#include <Arduino.h>
#include <painlessMesh.h>

#include "config.h"

#define REPORT_STATE_INT 20
#define LIGHT_PIN D2

Scheduler userScheduler;
painlessMesh mesh;

uint32_t gateway = 0;

void sendMessage();

String generateStatusString();

void publishNode();

String constructTopic();

Task taskSendMessage(TASK_SECOND * REPORT_STATE_INT, TASK_FOREVER, &sendMessage);

void sendMessage() {
    if (gateway != 0) {
        String msg = generateStatusString();
        mesh.sendSingle(gateway, msg);
    }
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

void receivedCallback(uint32_t from, String &msg) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, msg);

    if (doc.containsKey("gateway") && doc["gateway"].containsKey("node_id")) {
        gateway = doc["gateway"]["node_id"];
        publishNode();
        taskSendMessage.enable();
    } else if (doc.containsKey("topic") && doc.containsKey("payload")) {
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

    Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
}

void publishNode() {
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

    String publicationMessage;
    serializeJson(jsonDocument, publicationMessage);
    mesh.sendSingle(gateway, publicationMessage.c_str());
}

String constructTopic() {
    return String("devices/light/") + mesh.getNodeId();
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
    mesh.setDebugMsgTypes(ERROR | STARTUP);

    pinMode(LIGHT_PIN, OUTPUT);

    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.setContainsRoot(true);

    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

    userScheduler.addTask(taskSendMessage);
}

void loop() {
    mesh.update();
}