//
// Created by aykbo on 09.06.2020.
//

#ifndef BRIDGEAWAREMESH_H
#define BRIDGEAWAREMESH_H

#include <painlessMesh.h>

using namespace painlessmesh;

typedef std::function<void(String &msg)> receivedFromGatewayCallback_t;

typedef std::function<void()> shouldPublishCallback_t;

class BridgeAwareMesh : public painlessMesh {

public:
    BridgeAwareMesh();

    using painlessMesh::onReceive;
    void onReceive(receivedCallback_t onReceive);

    void onReceivedFromGateway(receivedFromGatewayCallback_t cb);

    void onShouldPublish(shouldPublishCallback_t cb);

    bool sendToGateway(String &msg);

protected:
    shouldPublishCallback_t shouldPublishCallback;
    receivedFromGatewayCallback_t receivedFromGatewayCallback;
    receivedCallback_t userReceivedCallback;

    uint32_t gatewayNodeId = 0;
};


#endif //BRIDGEAWAREMESH_H
