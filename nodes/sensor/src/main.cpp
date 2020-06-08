#include <Arduino.h>
#include <painlessMesh.h>

#include "config.h"

#define REPORT_STATE_INT 20
#define LIGHT_PIN D2

Scheduler userScheduler;
painlessMesh mesh;

bool light_status = false;

void sendMessage();

String generateStatusString();

Task taskSendMessage(TASK_SECOND * REPORT_STATE_INT, TASK_FOREVER, &sendMessage);

void sendMessage() {
    String msg = generateStatusString();
    mesh.sendBroadcast(msg);
}

String generateStatusString() {
    DynamicJsonDocument doc(256);
    doc["topic"] = String("devices/switch/") + mesh.getNodeId() + "/state";
    JsonObject payload = doc.createNestedObject("payload");
    payload["light"] = light_status;

    String msg;
    serializeJson(doc, msg);
    return msg;
}

void receivedCallback(uint32_t from, String &msg) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, msg);

    if (doc.containsKey("topic") && doc.containsKey("payload")) {
        String topic = doc["topic"].as<String>();

        if (topic.endsWith("set")) {
            if (doc["payload"].containsKey("light")) {
                bool newLightState = doc["payload"]["light"].as<bool>();
                digitalWrite(LIGHT_PIN, newLightState);
            }
        }
    }

    Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
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
    taskSendMessage.enable();
}

void loop() {
    mesh.update();
}