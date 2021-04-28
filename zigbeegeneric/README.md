# Generic ZigBee

This plugin allows to interact with ZigBee from many different manufacturers and different types using a native ZigBee network controller in nymea.

There is no specific list of supported manufacturers and models, because the plugin handles ZigBee certified devices in the most generic way possible. The manufacturer and model will be fetched from the ZigBee node and all requied information will be loaded dynamically from the ZigBee node according to the specifications.

If no manufacturer specific plugin handles a new joined ZigBee node, this plugin will try to support the device. If the device can be handled in a generic way, this plugin will create an approriate thing in nymea.

## Supported Things

In order to bring a ZigBee device into the nymea ZigBee network, the network needs to be opened for joining before you perform the device pairing instructions. The joining process can take up to 30 seconds. If the device does not show up, please restart the pairing process.

### On/off power sockets

Simple on/off power sockets.

**Pairing instructions**: Open the ZigBee network for joining. Follow the manufacturer specific reset/pairing instruction.

> Most power sockets have a pairing button. Once the device is powered, it can be resetted / paired by clicking the button multiple times of keeping it pressed for several seconds.

### Radiator thermostats

Radiator thermostats that follow the ZigBee specification.

### Door/window sensors

Door/window that follow the ZigBee IAS Zone specification.

## Requirements

* A compatible ZigBee controller and a running ZigBee network in nymea. You can find more information about supported controllers and ZigBee network configurations [here](https://nymea.io/documentation/users/usage/configuration#zigbee).

