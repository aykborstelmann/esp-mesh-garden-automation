//
// Created by aykbo on 10.06.2020.
//

#ifndef MQTTMESH_H
#define MQTTMESH_H


#include "BridgeAwareMesh.h"

using namespace painlessmesh;

typedef std::function<void(String& topic, String& msg)> mqttReceivedCallback_t;


/**
 * API for the usage of a MQTT bridge inside the network
 */
class MqttMesh : public BridgeAwareMesh {

public:
    MqttMesh();

    /** Send message to the MQTT broker
     * @param topic The MQTT topic the message should be published to
     * @param msg The message which should be published
     * @return success of transmission
     */
    bool sendMqtt(String &topic, String &msg);


    /** Register callback for MQTT messages
     * @param receivedCallback callback which is called when a new MQTT message is received
     */
    virtual void onMqttReceived(mqttReceivedCallback_t receivedCallback);

    void onReceivedFromGateway(receivedFromGatewayCallback_t cb) override;
protected:
    receivedFromGatewayCallback_t userReceivedFromGatewayCallback;
    mqttReceivedCallback_t mqttReceivedCallback;
};


#endif //MQTTMESH_H
