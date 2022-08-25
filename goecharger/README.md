# go-eCharger

nymea plugin for go-eCharger smart wallbox for electic vehicles.

In order to integrate go-eChargers with nymea, please make sure the `API V2` is enabled in the go-eCharger app.

If you are using the original go-e App or other client services to communicate with the wallbox, disable MQTT during the setup in order to make 
sure all services are able to communicate with the wallbox.
Please note that that using the `MQTT interface` for connecting, may prevent other applications or services to connect to the go-eCharger wallbox. 

The preferred way of communicating would be MQTT (API V2), default is HTTP (API V1).

## Supported Things

* go-eCharger Home (Hardware V1 and V2 using `API V1`)
* go-eCharger Home (Hardware V3 using `API V2`)

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

