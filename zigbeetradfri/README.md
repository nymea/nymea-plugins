# ZigBee IKEA TRÅDFRI

This plugin allows to interact with IKEA TRÅDFRI ZigBee devices using a native ZigBee network controller in nymea.

## Supported Things

In order to bring a ZigBee device into the nymea ZigBee network, the network needs to be opened for joining before you perform the device pairing instructions. The joining process can take up to 30 seconds. If the device does not show up, please restart the pairing process.



### TRÅDFRI Wireless dimmer

The [Wireless dimmer](https://www.ikea.com/us/en/p/tradfri-wireless-dimmer-white-10408598/) requires the latest firmware version in order to work properly with nymea. If the thing shows up in nymea once paired, but no button events will be recongized, please update the device to the latest firmware version and re-add it to nymea.

> Known working device firmware version: `2.2.008`

**Pairing instructions**: Open the ZigBee network for joining. Click the connect button 4 times within 5 seconds.



### TRÅDFRI Remote control

The [Remote control](https://www.ikea.com/us/en/p/tradfri-remote-control-00443130/) requires the latest firmware version in order to work properly with nymea. If the thing shows up in nymea once paired, but no button events will be recongized, please update the device to the latest firmware version and re-add it to nymea.

> Known working device firmware version: `2.3.014`

**Pairing instructions**: Open the ZigBee network for joining. Click the connect button 4 times within 5 seconds.



### TRÅDFRI Symfonisk

The [Remote control](https://www.ikea.com/us/en/p/symfonisk-sound-remote-white-20370482/) is *not* supported properly yet, even if it shows up when pairing. This implementation of this device is still under construction.

**Pairing instructions**: Open the ZigBee network for joining. Click the connect button 4 times within 5 seconds.



### TRÅDFRI Motion sensor

The [Remote control](https://www.ikea.com/us/en/p/tradfri-wireless-motion-sensor-white-60377655/) is fully supported.

**Pairing instructions**: Open the ZigBee network for joining. Click the connect button 4 times within 5 seconds.



### TRÅDFRI Signal repeater

The [Signal repeater](https://www.ikea.com/us/en/p/tradfri-signal-repeater-30400407/) is fully supported.

**Pairing instructions**: Open the ZigBee network for joining. Press the setup button with a needle for 5 seconds.



#### Control outlet

The [Control outlet](https://www.ikea.com/us/en/p/tradfri-wireless-control-outlet-30356169/) should be handeld in a generic way by the `nymea-plugin-zigbee-generic` plugin.

**Pairing instructions**: Open the ZigBee network for joining. Press the setup button with a needle for 5 seconds. The power socket will switch quick on and off and start the pairing process.



### TRÅDFRI lights

Most of the lights and lamps from IKEA TRÅDFRI should be handled in a generic way by the `nymea-plugin-zigbee-generic-lights` plugin.

Tested and verified lights:

* TRADFRI bulb E27 CWS opal 600lm (color light)
* TRADFRI bulb E27 WS clear 806lm (color temperature light)


**Pairing instructions**: Open the ZigBee network for joining. Switch the light off and on 6 times in a 1 second rythm. Once the light start flashing/dimming, the pairing process has been started successfully and the lamp will join the nymea ZigBee network.



## Requirements

* A compatible ZigBee controller and a running ZigBee network in nymea. You can find more information about supported controllers and ZigBee network configurations [here](https://nymea.io/documentation/users/usage/configuration#zigbee).


## More

[IKEAD](https://www.ikea.com)
