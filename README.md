# ESP Mesh MQTT
Homeautomation setup especially for gardening. Using ESPs (ESP8266 or ESP32) as nodes and a Raspberry Pi as Server

## Structure
This project uses a Raspberry Pi with a `docker-compose` to run [Home Assistent](https://www.home-assistant.io/) and a [MQTT broker](https://mosquitto.org/).
The broker is the connection between the mesh and the Home Assistant. The mesh is based on [painlessMesh](https://gitlab.com/painlessMesh/painlessMesh) 
to archive a simple manageable mesh structure with JSON package communication. One node serves as an MQTT bridge since the mesh network itself can't communicate 
with the broker. The bridge redirects messages from inside the mesh to the MQTT broker. 

## Purpose 
This project should serve as an automation system for garden watering. It uses the mesh structure to function outside of the home's wifi radius. 
Home Assistant is the view for the project which doesn't need much setup or programming. Each node can be connected to a sensor or a valve and sends its messages
to the MQTT broker which can be controlled and analysed by Home Assistant.

## Setup
If you want to contribute or just use this code, make sure to copy the `example-config.h` to `config.h` and fill in your own values.