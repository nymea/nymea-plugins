# go-eCharger

nymea plugin for go-eCharger smart wallbox for electic vehicles.

Once you connect to the go-eCharger, nymea will configure the wallbox to use MQTT and send information to nymea.
Please make sure no other service is using the custom MQTT server in the local network, otherwise they will exclude each other, depending who comes first.

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

