#include <Arduino.h>
#include <painlessMesh.h>

#include "config.h"
#include "BridgeAwareMesh.h"

#define REPORT_STATE_INT 20
#define LIGHT_PIN D2

Scheduler userScheduler;
BridgeAwareMesh mesh;

void sendMessage();

Task taskSendMessage(TASK_SECOND * REPORT_STATE_INT, TASK_FOREVER, &sendMessage);

String constructTopic() {
    return String("devices/light/") + mesh.getNodeId();
}

String generateStatusString() {
    bool light_status = digitalRead(LIGHT_PIN);

    DynamicJsonDocument doc(256);
    doc["topic"] = constructTopic() + "/state";
    doc["payload"] = light_status ? "ON" : "OFF";

    String msg;
    serializeJson(doc, msg);
    return msg;
}

void sendMessage() {
    String msg = generateStatusString();
    mesh.sendToGateway(msg);
}

void receivedFromGatewayCallback(String &msg) {
    Serial.printf("Received from gateway - %s\n", msg.c_str());

    DynamicJsonDocument doc(256);
    deserializeJson(doc, msg);

    if (doc.containsKey("topic") && doc.containsKey("payload")) {
        String topic = doc["topic"].as<String>();

        if (topic.endsWith("set")) {
            if (doc["payload"] == "ON") {
                digitalWrite(LIGHT_PIN, HIGH);
            } else if (doc["payload"] == "OFF") {
                digitalWrite(LIGHT_PIN, LOW);
            } else {
                Serial.println("ERROR: Could not parse message");
            }
            sendMessage();
        }
    }
}

void receivedCallback(uint32_t from, String &msg) {
    Serial.printf("Received from %u - %s\n", from, msg.c_str());
}

void publishCallback() {
    Serial.println("MESH -> BRIDGE: Publishing node, got notification");

    DynamicJsonDocument jsonDocument(512);
    const String &ownTopic = constructTopic();

    jsonDocument["topic"] = ownTopic + "/config";
    JsonObject payload = jsonDocument.createNestedObject("payload");
    payload["~"] = ownTopic;
    payload["name"] = HR_NAME;
    payload["unique_id"] = String(mesh.getNodeId());
    payload["cmd_t"] = "~/to/set";
    payload["stat_t"] = "~/state";
    payload["schema"] = "json",
            payload["brightness"] = false;

    String configMessage;
    serializeJson(jsonDocument, configMessage);
    mesh.sendToGateway(configMessage);

    Serial.println("MESH -> BRIDGE: Config message: " + configMessage);

    taskSendMessage.enable();
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
    Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
    Serial.begin(115200);
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

    pinMode(LIGHT_PIN, OUTPUT);

    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.setContainsRoot(true);

    mesh.onReceive(&receivedCallback);
    mesh.onReceivedFromGateway(receivedFromGatewayCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onShouldPublish(&publishCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

    userScheduler.addTask(taskSendMessage);
}

void loop() {
    mesh.update();
}