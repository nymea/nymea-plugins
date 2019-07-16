# usbrelay
--------------------------------

This plugin supports the usb HID based relay switch. The device can be discovered and added to the system once connected. The plugin uses the `libudev1` and `libhidapi-hidraw` for communicating with the device. Currently only the USBRelay2 is supported, but the plugin is prepared to be extended for all versions (4, 6, 8 channels) of the interface. The impuse was added since it got requested and does genenerate a 250 ms high impulse on the relay.

![2 Channel usb relay](https://raw.githubusercontent.com/guh/nymea-plugins/master/usbrelay/docs/images/5v-220v-2-channel-relay-shield-control-interface-usb-front.jpg "2 channle usb relay front")

![2 Channel usb relay](https://raw.githubusercontent.com/guh/nymea-plugins/master/usbrelay/docs/images/5v-220v-2-channel-relay-shield-control-interface-usb.jpg "2 channle usb relay back")


