# Solarman

This integration plugin allows to add Solarman compatible inverters to nymea.

Every inverter which is compatible with the Solarman app should be possible to use with this
plugin, provided the Modbus registers for the inverter are defined in the registermappings.json.

## Current supported devices

* Bosswerk MI-300
* Bosswerk MI-600

## Requirements

The solar inverter needs to be connected to the same network as nymea. Once powered on, the
inverter will open a wireless hotspot named AP_XXXXXXXXXX where XXXXXXXXXX is the serial number
written on the casing. The default password for this WiFi is 12345678. After connecting,
open [http://10.10.100.254](http://10.10.100.254) with the browser and use the web interface
on the inverter to connect it to a WiFi with nymea in it.

Once done so, set up the inverter in nymea as any other thing.
