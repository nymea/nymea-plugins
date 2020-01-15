
# Tasmota

This plugin allows to make use of Tasmota devices via the nymea internal MQTT broker. There is no external MQTT broker needed.

Note that Sonoff devices must be flashed with the Tasmota sofware and connected to the WiFi network in order to work with this plugin.

See the [Tasmota wiki](https://tasmota.github.io/docs/#/Home) for a list of all supported devices and instructions on how to
install Tasmota on those. Virtually any ESP8266 or ESP8285 can be flashed.

After flashing Tasmota to a Sonoff device and connecting it to WiFi, it can be added to nymea. The only required
thing is the IP address to the device. This plugin will create a new isoloated MQTT channel on the nymea internal
MQTT broker and provision login details to the Tasmota device via HTTP. Once that is successful, the Tasmota device
will connect to the MQTT broker and appear as connected in nymea.

## Plugin properties
When adding a Tasmota device it will add a new Gateway type device representing the Tasmota device itself. In addition
to that a power switch device will appear which can be used to control the switches in the Tasmota device. Upon
device setup, the user can optionally select the type of the connected hardware, (e.g. a light, roller shutter or blind) which
causes this plugin to create an additional device in the system which also controls the switches inside the Tasmota device and nicely
integrates with the nymea:ux for the given device type.
