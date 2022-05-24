# go-eCharger

nymea plugin for go-eCharger smart wallbox for electic vehicles.

In order to make nymea work with go-e, please make sure you have enable `API V2` in the official app.

If you are using the original go-e App or other client services to communicate with the wallbox, disable MQTT during the setup in order to make sure all services are able to communicate with the wallbox. There is no support for multiple MQTT clients on go-e devices, thus nymea defaults to HTTP to prevent constant reconfiguration trough the clients.

The preferred way of communicating would be MQTT (API V2), default is HTTP (API V2).

## Supported Things

* go-eCharger Home

## Requirements

* The package "nymea-plugin-goecharger" must be installed.
* The device must be in the same local area network as nymea.
* The Firmware version has to be at least `030.0` (API V1).
* The Firmware version has to be at least `051.1` (API V2).

## Developer documentation

The documentation of the API V1 can be found [here](https://github.com/goecharger/go-eCharger-API-v1).
The documentation of the API v2 can be found [here](https://github.com/goecharger/go-eCharger-API-v2).

## More

https://go-e.co/

