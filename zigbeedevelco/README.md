# Develco ZigBee

This plugin allows to interact with ZigBee devices from [Develco](https://www.develcoproducts.com/).

## Supported Things

In order to bring a ZigBee device into the nymea ZigBee network, the network needs to be opened for joining before you perform the device pairing instructions. The joining process can take up to 30 seconds. If the device does not show up, please restart the pairing process.

### IO Module

The [Develco IO Module](https://www.develcoproducts.com/products/smart-relays/io-module/) is fully supported.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the reset button of the IO module until the LED blinks constanly.

### Air Quality sensor

The [Develco Air Quality sensor](https://www.develcoproducts.com/products/sensors-and-alarms/air-quality-sensor/) is fully supported.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the reset button in the sensor casing until the LED blinks constanly.

## Requirements

* A compatible ZigBee controller and a running ZigBee network in nymea. You can find more information about supported controllers and ZigBee network configurations [here](https://nymea.io/documentation/users/usage/configuration#zigbee).

