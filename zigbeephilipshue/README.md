# Zigbee Philips Hue

This plugin allows to interact with Philips Hue ZigBee devices using a native ZigBee network controller in nymea.

## Supported Things

In order to bring a ZigBee device into the nymea ZigBee network, the network needs to be opened for joining before you perform the device pairing instructions. The joining process can take up to 30 seconds. If the device does not show up, please restart the pairing process.

### Hue motion sensor

The [Hue motion sensor](https://www.philips-hue.com/en-us/p/hue-motion-sensor/046677473389) is fully supported. The time period for the present state can be specied in the thing setting.

**Pairing instructions**: Open the ZigBee network for joining. Press the setup button for 5 seconds until the LED start blinking in different colors.



### Hue outdoor sensor
The [Hue outdoor sensor](https://www.philips-hue.com/en-us/p/hue-outdoor-sensor/046677541736) is fully supported. The time period for the present state can be specied in the thing setting.


### Hue dimmer switch

The [Hue dimmer switch](https://www.philips-hue.com/en-us/p/hue-dimmer-switch/046677473372) is fully supported.

**Pairing instructions**: Open the ZigBee network for joining. Press the setup button for 5 seconds until the LED start blinking in different colors.

### Hue lights

Most of the lights and lamps from Philips Hue should be handled in a generic way by the `nymea-plugin-zigbee-generic-lights` plugin. There are 2 methods for bringing a Hue light / bulb into the nymea ZigBee network.

1. Use a [Hue dimmer switch](https://www.philips-hue.com/en-us/p/hue-dimmer-switch/046677473372):

    * Open the ZigBee network for for allowing new devices to join the network.
    * Switch the lamp off and on again (take the power).
    * Take the Hue dimmer switch and press the `On` and the `Off` key at the same time
    * Hold the switch close to the lamp while keep pressing both buttons (< 5 cm).
    * Keep holding an pressing the buttons until the lamp starts blinking multiple times.
    * Once you release the buttons on the switch, the lamp should be resetted and start joining the nymea ZigBee network.

2. Remove the lamp from an existing Hue Bridge setup.

    * Open the ZigBee network in nymea for allowing new devices to join the network.
    * Remove the lamp from the Hue Bridge using the official Hue App or nymea.
    * The lamp should now be resetted and start joining the nymea ZigBee network.

The most reliable way is method 1.

## Requirements

* A compatible ZigBee controller and a running ZigBee network in nymea. You can find more information about supported controllers and ZigBee network configurations [here](https://nymea.io/documentation/users/usage/configuration#zigbee).
* Optional: [Hue dimmer switch](https://www.philips-hue.com/en-us/p/hue-dimmer-switch/046677473372) for resetting lights and bringing them into the nymea ZigBee network.

## Request support for a ZigBee device.

If your Hardware is not listed and you want to request support for your device, please let us know in the [nymea forum](https://forum.nymea.io/).

## More

 [Philips hue](http://www2.meethue.com/) 
