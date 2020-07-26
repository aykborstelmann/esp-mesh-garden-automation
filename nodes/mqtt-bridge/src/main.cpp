#include <Arduino.h>
#include <painlessMesh.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

#include "config.h"

void receivedCallback(const uint32_t &from, const String &msg);

void mqttCallback(char *topic, byte *payload, unsigned int length);

void newConnectionCallback(const uint32_t &id);

IPAddress getLocalIP();

uint32_t parseTarget(char *topic);

String constructGatewayPublication();

IPAddress myIP(0, 0, 0, 0);
IPAddress mqttBroker(192, 168, 176, 6);

painlessMesh mesh;
WiFiClient wifiClient;
PubSubClient mqttClient(mqttBroker, 1883, mqttCallback, wifiClient);

void setup() {
    Serial.begin(115200);
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

    mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, CHANNEL);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);

    mesh.stationManual(STATION_SSID, STATION_PASSWORD);
    mesh.setHostname(HOSTNAME);

    mesh.setRoot(true);
    mesh.setContainsRoot(true);
}

void publishGatewayToMesh() {
    String configTopic = String("devices/gateway/") + mesh.getNodeId() + "/config";
    mqttClient.publish(configTopic.c_str(), "{}", true);

    String publishGatewayMessage = constructGatewayPublication();
    mesh.sendBroadcast(publishGatewayMessage.c_str());
}

void connectMqttClient() {
    if (mqttClient.connect("painlessMeshClient")) {
        Serial.println("CONNECTION: Connected to MQTT Broker");
        publishGatewayToMesh();

        mqttClient.subscribe("devices/+/+/to/#");
    }
}


void loop() {
    mesh.update();
    mqttClient.loop();

    if (myIP != getLocalIP()) {
        myIP = getLocalIP();
        Serial.println("CONNECTION: IP is " + myIP.toString());

        connectMqttClient();
    }

    if (!mqttClient.connected()) {
        connectMqttClient();
    }
}

void newConnectionCallback(const uint32_t &id) {
    if (mqttClient.connected()) {
        Serial.println("MESH: Publishing ");
        String publicationMessage = constructGatewayPublication();
        mesh.sendSingle(id, publicationMessage.c_str());
    }
}

String constructGatewayPublication() {
    String publicationMessage;
    DynamicJsonDocument jsonDocument(256);
    JsonObject gateway = jsonDocument.createNestedObject("gateway");
    gateway["node_id"] = mesh.getNodeId();
    serializeJson(jsonDocument, publicationMessage);
    return publicationMessage;
}

void receivedCallback(const uint32_t &from, const String &msg) {
    DynamicJsonDocument jsonDocument(1024);
    deserializeJson(jsonDocument, msg);

    Serial.printf("MESH: Received message from %u - %s\n", from, msg.c_str());
    if (mqttClient.connected() && jsonDocument.containsKey("topic") && jsonDocument.containsKey("payload")) {
        String topic = jsonDocument["topic"].as<String>();
        String payload = jsonDocument["payload"].as<String>();

        Serial.println("MESH -> MQTT: Forward message to MQTT broker, to " + topic + " - " + payload);
        mqttClient.publish(topic.c_str(), payload.c_str(), true);
    }
}

void mqttCallback(char *topic, uint8_t *payload, unsigned int length) {
    char *cleanPayload = (char *) malloc(length + 1);
    payload[length] = '\0';
    memcpy(cleanPayload, payload, length + 1);

    String msg = String(cleanPayload);
    free(cleanPayload);

    uint32_t target = parseTarget(topic);
    Serial.printf("MQTT: Received notification for %s - %s and target %ul\n", topic, payload, target);
    if (mesh.isConnected(target)) {
        DynamicJsonDocument doc(512);
        doc["topic"] = String(topic);
        doc["payload"] = msg;

        String buf;
        serializeJson(doc, buf);

        Serial.println(String("MQTT -> Mesh: Forward message to node ") + target + " with payload " + buf);
        mesh.sendSingle(target, buf.c_str());
    }
}


uint32_t parseTarget(char *topic) {
    String topicString(topic);

    int indexOfFirstSlash = topicString.indexOf("/");
    int indexOfSecondSlash = topicString.indexOf("/", indexOfFirstSlash + 1);
    int indexOfThirdSlash = topicString.indexOf("/", indexOfSecondSlash + 1);

    String targetString = topicString.substring(indexOfSecondSlash + 1, indexOfThirdSlash);
    return strtoul(targetString.c_str(), nullptr, 0);
}

IPAddress getLocalIP() {
    return IPAddress(mesh.getStationIP());
}
