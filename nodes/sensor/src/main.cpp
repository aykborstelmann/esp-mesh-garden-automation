#include <Arduino.h>
#include <painlessMesh.h>
#include <MqttMesh.h>
#include <RunningMedian.h>
#include <math.h>

#include "config.h"

#define REPORT_STATE_INT 3
#define DEVICE_TYPE "sensor"


#define SENSOR_PIN A0
#define SENSOR_POWER_PIN D1

#define SENSORS_MAX 800.0
#define SENSORS_MIN 312.0

Scheduler userScheduler;
MqttMesh mesh;

void measure();

Task taskMeasure(TASK_SECOND * REPORT_STATE_INT, TASK_FOREVER, &measure);

String constructTopic() {
    return String("devices/") + DEVICE_TYPE + "/" + mesh.getNodeId();
}

int readMoistureOnce() {
    return analogRead(SENSOR_PIN);
}

double readMoisture() {
    Serial.println("MEASUREMENT - Start -----");

    const int measurements = 5;
    RunningMedian median = RunningMedian(measurements);

    for (int i = 0; i < measurements; i++) {
        auto value = (float) readMoistureOnce();
        Serial.println(String("MEASUREMENT - Value ") + i + " " + value);
        median.add(value);
    }


    float average = median.getAverage(measurements - 2);
    Serial.println(String("MEASUREMENT - Average ") + average);


    const double percent = ((average - SENSORS_MAX) / (SENSORS_MIN - SENSORS_MAX)) * 100.0;

    Serial.println(String("MEASUREMENT - Percent ") + percent);

    Serial.println("MEASUREMENT - End -----");
    return round(percent * 100) / 100;
}

void measure() {
    DynamicJsonDocument jsonDocument(128);
    jsonDocument["moisture"] = readMoisture();

    String statePayload;
    serializeJson(jsonDocument, statePayload);

    Serial.println("MESH -> MQTT: " + statePayload);

    mesh.sendMqtt(constructTopic() + "/state", statePayload);
}

void mqttReceivedCallback(String &topic, String &payload) {
    Serial.printf("Received from MQTT - %s - %s", topic.c_str(), payload.c_str());
}

void publishCallback() {
    Serial.println("MESH -> BRIDGE: Publishing node, got notification");

    DynamicJsonDocument configJson(512);

    configJson["name"] = HR_NAME;
    configJson["unique_id"] = String(mesh.getNodeId());
    configJson["device_class"] = "humidity";
    configJson["stat_t"] = constructTopic() + "/state";
    configJson["unit_of_measurement"] = "%";
    configJson["value_template"] = "{{ value_json.moisture}}";

    String configPayload;
    serializeJson(configJson, configPayload);

    mesh.sendMqtt(constructTopic() + "/config", configPayload);

    Serial.println("MESH -> BRIDGE: Config message: " + configPayload);
    taskMeasure.enable();
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

    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.setContainsRoot(true);

    mesh.onBridgeAvailable(&publishCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onMqttReceived(&mqttReceivedCallback);
    mesh.onReceivedFromGateway([](String &msg) {
        Serial.println(msg);
    });
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

    userScheduler.addTask(taskMeasure);
}

void loop() {
    mesh.update();
}
