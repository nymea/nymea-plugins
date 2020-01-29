# Shelly

The Shelly plugin adds support for Shelly devices (https://shelly.cloud).

The currently supported devices are:
* Shelly 1
* Shelly 1PM
* Shelly 2.5
* Shelly Plug / PlugS
* Shelly RGBW2
* Shelly Dimmer

## Requirements
Shelly devices communicate with via MQTT. This means, in order to add Shelly devices to nymea, the nymea instance is required
to have the MQTT broker enabled in the nymea settings and the Shelly device needs to be connected to the same WiFi as nymea is
in. New Shelly devices will open a WiFi named with their name as SSID. For instance, a Shelly 1 would appear as "shelly1-XXXXXX".
Connect to this WiFi and open the webpage that will pop up. From there, it can be configured it to connect to the same
network where the nymea system is located. No other options need to be set as they can be configured using nymea later on.


## Setting up devices
Once the Shelly is connected to the WiFi, a device discovery in nymea can be performed and will list the Shelly device.
For some Shelly devices, the connected device can be configured during setup. If the Shelly is connected to e.g. a light bulb,
choose "Light" here. Optionally, a username and password can be set. If the Shelly device is already configured to require
authentication, the username and password here must match the ones set on the Shelly. NOTE: If the Shelly is not configured
to require a login yet, but credentials are entered during setup, the Shelly device will be configured to require authentication
from now on and this login will be required also for the web interface of the Shelly device.

## Plugin properties
When adding a Shelly device that is meant to be installed in walls and has connectors to switches, a new Gateway type device 
representing the Shelly device itself will be added. The gateway device allow basic monitoring (such as the connected state)
and interaction (e.g. reboot the Shelly device). In addition to that, a power switch device will appear which will reflect
presses on the Shelly's SW input. This power switch device also offers the possiblity to configure the used switch (e.g. 
toggle, momentary, edge or detached from the Shelly's output). If a connected device has been selected during setup, an
additional device, e.g. the light will appear in the system and can be used to control the power output of the Shelly,
e.g. turning on or off the connected light.
