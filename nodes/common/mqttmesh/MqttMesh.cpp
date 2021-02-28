//
// Created by aykbo on 10.06.2020.
//

#include "MqttMesh.h"

bool MqttMesh::sendMqtt(String &topic, String &msg) {
    DynamicJsonDocument jsonDocument(48 + topic.length() + msg.length());

    jsonDocument["topic"] = topic;
    jsonDocument["payload"] = msg;

    String buf;
    serializeJson(jsonDocument, buf);
    return BridgeAwareMesh::sendToGateway(buf);
}

void MqttMesh::onMqttReceived(mqttReceivedCallback_t receivedCallback) {
    this->mqttReceivedCallback = receivedCallback;
}

void MqttMesh::onReceivedFromGateway(receivedFromGatewayCallback_t cb) {
    this->userReceivedFromGatewayCallback = cb;
}

MqttMesh::MqttMesh() {
    auto cb = [this](String &msg) {
        DynamicJsonDocument jsonDocument(512);
        deserializeJson(jsonDocument, msg);

        if (jsonDocument.containsKey("topic") && jsonDocument.containsKey("payload")) {
            if (mqttReceivedCallback) {
                String topic = jsonDocument["topic"];
                String payload = jsonDocument["payload"];
                mqttReceivedCallback(topic, payload);
            }
        } else {
            if (userReceivedFromGatewayCallback) {
                userReceivedFromGatewayCallback(msg);
            }
        }
    };
    BridgeAwareMesh::onReceivedFromGateway(cb);
}
