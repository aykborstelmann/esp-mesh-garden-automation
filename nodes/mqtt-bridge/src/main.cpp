#include <Arduino.h>
#include <painlessMesh.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

#include "config.h"

void receivedCallback(const uint32_t &from, const String &msg);

void mqttCallback(char *topic, byte *payload, unsigned int length);

IPAddress getLocalIP();

void publishGateway();

uint32_t parseTarget(char *topic);

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
            publishGateway();

            mqttClient.subscribe("devices/+/+/to/#");
        }
    }
}


void publishGateway() {
    String configTopic = String("devices/gateway/") + mesh.getNodeId() + "/config";
    mqttClient.publish(configTopic.c_str(), "{}", true);
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

    uint32_t target = parseTarget(topic);
    Serial.println(target);
    if (mesh.isConnected(target)) {
        mesh.sendSingle(target, msg);
    }
}

uint32_t parseTarget(char *topic) {
    String topicString(topic);

    int indexOfFirstSlash = topicString.indexOf("/");
    int indexOfSecondSlash = topicString.indexOf("/", indexOfFirstSlash + 1);
    int indexOfThirdSlash = topicString.indexOf("/", indexOfSecondSlash + 1);

    String targetString = topicString.substring(indexOfSecondSlash + 1, indexOfThirdSlash);
    return static_cast<uint32_t>(atoi(targetString.c_str()));
}


IPAddress getLocalIP() {
    return IPAddress(mesh.getStationIP());
}