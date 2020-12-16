# Generic ZigBee lights

This plugin allows to interact with ZigBee lights from many different manufacturers and different types using a native ZigBee network controller in nymea.

There is no specific list of supported manufacturers and models, because the plugin handles ZigBee certified lights in the most generic way possible. The manufacturer and model will be fetched from the ZigBee node and all requied information will be loaded dynamically from the ZigBee node according to the specifications.

* If a lamp supports native color temperature, the range of the color temperature will be fetched from the lamp.
* If a color lamp does not support color temperature native (on a ZigBee cluster level), the color temperature will be emulated using a color scale.

This plugin tries to catch all lightning devices based on the ZigBee profiles *Light Link* and *Home Automation*.

## Supported Things

In order to bring a ZigBee device into the nymea ZigBee network, the network needs to be opened for joining before you perform the device pairing instructions. The joining process can take up to 30 seconds. If the device does not show up, please restart the pairing process.

### On/Off lights

Lamps which support only beeing switched on and off.

**Pairing instructions**: Open the ZigBee network for joining. Follow the manufacturer specific reset/pairing instruction.

> Most lights can be resetted/paired by switching them off and on again 6 times. Some lights like Hue require a factory reset using a switch. Please check the manufacturer specific ZigBee plugins if available or read the user manual of your hardware how to bring the lamp into a network.

### Dimmable lights

Lamps which support only beeing switched on and off and dimmed.

**Pairing instructions**: Open the ZigBee network for joining. Follow the manufacturer specific reset/pairing instruction.

> Most lights can be resetted/paired by switching them off and on again 6 times. Some lights like Hue require a factory reset using a switch. Please check the manufacturer specific ZigBee plugins if available or read the user manual of your hardware how to bring the lamp into a network.

### Color temperature lights

Lamps which support beeing switched on/off, dimmed and allow setting the color temperature.

**Pairing instructions**: Open the ZigBee network for joining. Follow the manufacturer specific reset/pairing instruction.

> Most lights can be resetted/paired by switching them off and on again 6 times. Some lights like Hue require a factory reset using a switch. Please check the manufacturer specific ZigBee plugins if available or read the user manual of your hardware how to bring the lamp into a network.


### Color lights

Lamps which support beeing switched on/off, dimmed and allow setting the colors. The plugin tries to be as close as possible to the lamp capabilities and specifications like color temperature capabilities and ranges.

> Note: the lamp specific color gamut will currently not be fetched, but it is a pland feature for the future.

**Pairing instructions**: Open the ZigBee network for joining. Follow the manufacturer specific reset/pairing instruction.

> Most lights can be resetted/paired by switching them off and on again 6 times. Some lights like Hue require a factory reset using a switch. Please check the manufacturer specific ZigBee plugins if available or read the user manual of your hardware how to bring the lamp into a network.


## Requirements

* A compatible ZigBee controller and a running ZigBee network in nymea. You can find more information about supported controllers and ZigBee network configurations [here](https://nymea.io/documentation/users/usage/configuration#zigbee).

