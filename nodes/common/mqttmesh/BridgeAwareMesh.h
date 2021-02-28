//
// Created by aykbo on 09.06.2020.
//

#ifndef BRIDGEAWAREMESH_H
#define BRIDGEAWAREMESH_H

#include <painlessMesh.h>

using namespace painlessmesh;

typedef std::function<void(String &msg)> receivedFromGatewayCallback_t;

typedef std::function<void()> bridgeAvailableCallback_t;

/**
 * Easy API for usage with bridges in the mesh
 */
class BridgeAwareMesh : public painlessMesh {

public:
    BridgeAwareMesh();

    using painlessMesh::onReceive;
    void onReceive(receivedCallback_t onReceive);

    /** Callback when a message from the gateway is received
     *
     * @param fromGatewayCallback
     */
    virtual void onReceivedFromGateway(receivedFromGatewayCallback_t fromGatewayCallback);

    /** Callback when bridge is available
     * Callback gets called when the Bridge reveals itself
     * Possibly: the node should then register itself and/or should start sending messages
     * @param bridgeAvailableCallback
     */
    void onBridgeAvailable(bridgeAvailableCallback_t bridgeAvailableCallback);

    /** Send message to gateway
     * Transmit message to gateway
     * @param msg
     * @return success of transmission and false if the gateway is not registrated
     */
    bool sendToGateway(String &msg);

protected:
    bridgeAvailableCallback_t bridgeAvailableCallback;
    receivedFromGatewayCallback_t receivedFromGatewayCallback;
    receivedCallback_t userReceivedCallback;

    uint32_t gatewayNodeId = 0;
};


#endif //BRIDGEAWAREMESH_H
