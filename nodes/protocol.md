# Protocol mesh and MQTT
Inspired by https://www.home-assistant.io/docs/mqtt/discovery/

## General Path
`devices/<device_type>/<id>`
* device_type: sensor, switch, ... 

## Config topic
* Path: `devices/<device_type>/<id>/config`
* Payload: 
````json
{
  "name": "",
  "device_class": "<device_type>",
  "state_topic": "see state topic"
}
````

## State topic
* Path: `devices/<device_type>/<id>/state`
* Payload, e.g.: 
`````json
{
  "temature": 22
}
`````

## To node
* Path `devices/<device_type>/<id>/to/`

## Gateway notification
The gateway publishes its id  to all nodes which are conected. This payload look like this
```json
{
  "gateway": {
    "node_id": "<id>",
  }
}
```