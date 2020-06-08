#include <Arduino.h>
#include <painlessMesh.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

#include "config.h"

void receivedCallback(const uint32_t &from, const String &msg);

void mqttCallback(char *topic, byte *payload, unsigned int length);

IPAddress getLocalIP();

String putTopicTogether(const String &component, uint32_t nodeId, const String &messageType);

void publishDevice(const char *component, uint32_t id, const char *payload);

IPAddress myIP(0, 0, 0, 0);
IPAddress mqttBroker(192, 168, 176, 70);

painlessMesh mesh;
WiFiClient wifiClient;
PubSubClient mqttClient(mqttBroker, 1883, mqttCallback, wifiClient);

void setup() {
    Serial.begin(115200);
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

    mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, CHANNEL);
    mesh.onReceive(&receivedCallback);

    mesh.stationManual(STATION_SSID, STATION_PASSWORD);
    mesh.setHostname(HOSTNAME);

    mesh.setRoot(true);
    mesh.setContainsRoot(true);
}

void loop() {
    mesh.update();
    mqttClient.loop();

    if (myIP != getLocalIP()) {
        myIP = getLocalIP();
        Serial.println("My IP is " + myIP.toString());

        if (mqttClient.connect("painlessMeshClient")) {
            Serial.println("Connected to MQTT Broker");
            publishDevice("gateway", mesh.getNodeId(), "{}");

            mqttClient.subscribe("devices/+/+/to/#");
        }
    }
}

void publishDevice(const char *component, uint32_t id, const char *payload) {
    String configTopic = putTopicTogether(component, id, "config");
    mqttClient.publish(configTopic.c_str(), payload, true);
}

String putTopicTogether(const String &component, uint32_t nodeId, const String &messageType) {
    String topic = "devices/" + component + "/" + nodeId + "/" + messageType;
    return topic;
}

void receivedCallback(const uint32_t &from, const String &msg) {
    DynamicJsonDocument jsonDocument(256);
    deserializeJson(jsonDocument, msg);

    Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());

    if (jsonDocument.containsKey("topic") && jsonDocument.containsKey("payload")) {
        const char *topic = jsonDocument["topic"].as<char *>();
        const char *payload = jsonDocument["payload"].as<char *>();
        mqttClient.publish(topic, payload, true);
    }
}

void mqttCallback(char *topic, uint8_t *payload, unsigned int length) {
    char *cleanPayload = (char *) malloc(length + 1);
    payload[length] = '\0';
    memcpy(cleanPayload, payload, length + 1);

    Serial.printf("%s %s\n", topic, payload);
    String msg = String(cleanPayload);
    free(cleanPayload);

    String targetStr = String(topic).substring(16);
}


IPAddress getLocalIP() {
    return IPAddress(mesh.getStationIP());
}