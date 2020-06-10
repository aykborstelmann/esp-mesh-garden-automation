//
// Created by aykbo on 09.06.2020.
//

#include "BridgeAwareMesh.h"

using namespace painlessmesh;

void BridgeAwareMesh::onReceivedFromGateway(receivedFromGatewayCallback_t cb) {
    receivedFromGatewayCallback = cb;
}

bool BridgeAwareMesh::sendToGateway(String &msg) {
    if (gatewayNodeId != 0) {
        return sendSingle(gatewayNodeId, msg);
    }

    Serial.println("BRIDGE: Didn't find bridge, so far");
    //TODO Find gateway via broadcast, necessary?

    return false;
}

void BridgeAwareMesh::onShouldPublish(shouldPublishCallback_t cb) {
    shouldPublishCallback = cb;
}

void BridgeAwareMesh::onReceive(receivedCallback_t onReceive) {
    userReceivedCallback = onReceive;
}

BridgeAwareMesh::BridgeAwareMesh() {
    auto receiveCb = [this](uint32_t from, String &msg) {
        DynamicJsonDocument jsonDocument(256);
        deserializeJson(jsonDocument, msg);

        bool newGatewayPublished =
                jsonDocument.containsKey("gateway") && jsonDocument["gateway"].containsKey("node_id");
        if (newGatewayPublished) {
            uint32_t nodeId = jsonDocument["gateway"]["node_id"];

            if (painlessMesh::isConnected(nodeId)) {
                Serial.println("BRIDGE: New bridge detected");
                gatewayNodeId = nodeId;

                if (shouldPublishCallback) {
                    shouldPublishCallback();
                }
            } else {
                Serial.println("BRIDGE: Newly detected bridge was not found");
            }
        } else if (gatewayNodeId == from) {
            if (receivedFromGatewayCallback) {
                receivedFromGatewayCallback(msg);
            }
        } else {
            if (userReceivedCallback) {
                userReceivedCallback(from, msg);
            }
        }
    };
    painlessMesh::onReceive(receiveCb);
}
