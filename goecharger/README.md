# go-eCharger

nymea plugin for go-eCharger smart wallbox for electic vehicles.

If you are using other services like the original goe app to communicate with the wallbox, disable MQTT during the setup in order to make sure 
all services are able to communicate with the wallbox. There is no support for multiple MQTT clients on the device and therefore nymea defaults to HTTP
to prevent constant reconfiguration trough the clients.

The prefered way of communicating would be MQTT, default is HTTP. 

## Supported Things

* go-eCharger Home

## Requirements

* The package "nymea-plugin-goecharger" must be installed.
* The device must be in the same local area network as nymea.
* The Firmware version has to be at least `030.00`.

## Developer documentation

The documentation of the API can be found [here](https://github.com/goecharger/go-eCharger-API-v1). 

## More

https://go-e.co/

