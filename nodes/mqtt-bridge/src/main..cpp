#include <Arduino.h>
#include <painlessMesh.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

#define   STATION_SSID     "YourAP_SSID"
#define   STATION_PASSWORD "YourAP_PWD"

#define HOSTNAME "MQTT_Bridge"

// Prototypes
void receivedCallback( const uint32_t &from, const String &msg );
void mqttCallback(char* topic, byte* payload, unsigned int length);

IPAddress getlocalIP();

IPAddress myIP(0,0,0,0);
IPAddress mqttBroker(192, 168, 1, 1);

painlessMesh  mesh;
WiFiClient wifiClient;
PubSubClient mqttClient(mqttBroker, 1883, mqttCallback, wifiClient);

void setup() {
    Serial.begin(115200);

    mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages

    // Channel set to 6. Make sure to use the same channel for your mesh and for you other
    // network (STATION_SSID)
    mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6 );
    mesh.onReceive(&receivedCallback);

    mesh.stationManual(STATION_SSID, STATION_PASSWORD);
    mesh.setHostname(HOSTNAME);

    // Bridge node, should (in most cases) be a root node. See [the wiki](https://gitlab.com/painlessMesh/painlessMesh/wikis/Possible-challenges-in-mesh-formation) for some background
    mesh.setRoot(true);
    // This node and all other nodes should ideally know the mesh contains a root, so call this on all nodes
    mesh.setContainsRoot(true);
}

void loop() {
    mesh.update();
    mqttClient.loop();

    if(myIP != getlocalIP()){
        myIP = getlocalIP();
        Serial.println("My IP is " + myIP.toString());

        if (mqttClient.connect("painlessMeshClient")) {
            mqttClient.publish("painlessMesh/from/gateway","Ready!");
            mqttClient.subscribe("painlessMesh/to/#");
        }
    }
}

void receivedCallback( const uint32_t &from, const String &msg ) {
    Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());
    String topic = "painlessMesh/from/" + String(from);
    mqttClient.publish(topic.c_str(), msg.c_str());
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
    char* cleanPayload = (char*)malloc(length+1);
    payload[length] = '\0';
    memcpy(cleanPayload, payload, length+1);
    String msg = String(cleanPayload);
    free(cleanPayload);

    String targetStr = String(topic).substring(16);

    if(targetStr == "gateway")
    {
        if(msg == "getNodes")
        {
            auto nodes = mesh.getNodeList(true);
            String str;
            for (auto &&id : nodes)
                str += String(id) + String(" ");
            mqttClient.publish("painlessMesh/from/gateway", str.c_str());
        }
    }
    else if(targetStr == "broadcast")
    {
        mesh.sendBroadcast(msg);
    }
    else
    {
        uint32_t target = strtoul(targetStr.c_str(), NULL, 10);
        if(mesh.isConnected(target))
        {
            mesh.sendSingle(target, msg);
        }
        else
        {
            mqttClient.publish("painlessMesh/from/gateway", "Client not connected!");
        }
    }
}

IPAddress getlocalIP() {
    return IPAddress(mesh.getStationIP());
}