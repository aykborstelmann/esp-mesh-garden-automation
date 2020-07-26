#include <Arduino.h>
#include <painlessMesh.h>
#include <MqttMesh.h>
#include <RunningMedian.h>
#include <cmath>

#include "config.h"

Scheduler userScheduler;
MqttMesh mesh;

// Task callbacks
void sleep();

bool turnSensorOn();

void measureMoistureAndSend();

Task taskMeasure(TASK_IMMEDIATE, TASK_ONCE, nullptr, &userScheduler, false, &turnSensorOn, &measureMoistureAndSend);

Task taskSleep(TASK_IMMEDIATE, TASK_ONCE, &sleep, &userScheduler);

String topicPath() {
    return String("devices/") + DEVICE_TYPE + "/" + mesh.getNodeId();
}

int readSensorValue() {
    return analogRead(SENSOR_PIN);
}

void turnSensorOff() {
    Serial.println(String("MEASUREMENT - Turn off Sensor ") + millis());
    digitalWrite(SENSOR_POWER_PIN, LOW);
}

double readAndNormalizeSensorValue() {
    Serial.println(String("MEASUREMENT - Start -----") + millis());

    RunningMedian median = RunningMedian(MEASUREMENTS);

    for (int i = 0; i < MEASUREMENTS; i++) {
        auto value = (float) readSensorValue();
        Serial.println(String("MEASUREMENT - Value ") + i + " " + value);
        median.add(value);
    }

    float average = median.getAverage(MEASUREMENTS - 2);
    Serial.println(String("MEASUREMENT - Average ") + average);

    const double percent = map((long) average, SENSORS_MIN, SENSORS_MAX, 100, 0);

    Serial.println(String("MEASUREMENT - Percent ") + percent);

    Serial.println("MEASUREMENT - End -----");

    turnSensorOff();

    return round(percent * 100) / 100;
}

void measureMoistureAndSend() {
    DynamicJsonDocument jsonDocument(128);
    jsonDocument["moisture"] = readAndNormalizeSensorValue();

    String statePayload;
    serializeJson(jsonDocument, statePayload);

    Serial.println("MESH -> MQTT: " + statePayload);

    mesh.sendMqtt(topicPath() + "/state", statePayload);

    taskSleep.enableDelayed(TASK_MILLISECOND * 150);
}

bool turnSensorOn() {
    Serial.println(String("MEASUREMENT - Turn on Sensor ") + millis());
    digitalWrite(SENSOR_POWER_PIN, HIGH);
    return true;
}

void sleep() {
    Serial.println("DEEP_SLEEP: Enable");
    mesh.stop();
    ESP.deepSleep(SLEEP_TIME * 1000000, WAKE_RF_DEFAULT);
}

void introduceSensor() {
    Serial.println("MESH -> BRIDGE: Publishing node, got notification");

    DynamicJsonDocument configJson(512);

    configJson["name"] = HR_NAME;
    configJson["unique_id"] = String(mesh.getNodeId());
    configJson["device_class"] = "humidity";
    configJson["stat_t"] = topicPath() + "/state";
    configJson["unit_of_measurement"] = "%";
    configJson["frc_upd"] = true;
    configJson["value_template"] = "{{ value_json.moisture}}";

    String configPayload;
    serializeJson(configJson, configPayload);

    mesh.sendMqtt(topicPath() + "/config", configPayload);

    Serial.println("MESH -> BRIDGE: Config message: " + configPayload);
}

void bridgeAvailableCallback() {
    // Turn of automatic sleep
    taskSleep.disable();
    introduceSensor();

    taskMeasure.enableDelayed(TASK_MILLISECOND * 1000);
}

void setup() {
    Serial.begin(115200);
    pinMode(SENSOR_POWER_PIN, OUTPUT);

    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.setContainsRoot(true);

    mesh.onBridgeAvailable(&bridgeAvailableCallback);

    // Automatically sleep after 2 minutes also if not connected
    taskSleep.enableDelayed(TASK_MINUTE * 2);
}

void loop() {
    mesh.update();
}
