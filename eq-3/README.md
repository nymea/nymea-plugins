# eQ-3

This plugin allows to find and control radiation equipment by EQ-3.

Currently supported devices are the [EQ-3 Max! Cube LAN Gateway](http://www.eq-3.de/max-heizungssteuerung-produktdetail/items/bc-lgw-o-tw.html)
as well as [Eqiva Bluetooth Smart Radiator Thermostat](https://www.eq-3.com/products/eqiva/bluetooth-smart-radiator-thermostat.html).

To use this plugin, you need either a Max! Cube LAN Gateway with radiator or wall thermostats connected, or one or more
Eqiva Bluetooth Smart Radiator Thermostats within the range of the Bluetooth signal from your nymea system.

### Eqiva Bluetooth Smart Radiator Thermostat

In order to use the [Eqiva Bluetooth Smart Radiator Thermostat](https://www.eq-3.com/products/eqiva/bluetooth-smart-radiator-thermostat.html)
with nymea, the nymea system is required to have Bluetooth support and the nymea system and the Thermostat need to be located within
the range of Bluetooth between each other. Once the thermostat is connected to the radiator and calibrated for it (refer to the
Eqiva Smart Bluetooth Radiator Thermostat Manual) it can be added to nymea through the device setup.

Note: It is recommended to set the Radiator Thermostat mode to "Manual" if nymea should be controlling the target temperature. When the
radiator thermostat's mode is set to "Auto", it will manage temperature settings on its own and disregard nymea's commands.

### Max! Cube LAN Gateway

Once the cube is connected to the same network as the nymea system, it can be added to nymea through. Having connected multiple
Max! Cubes to the same network is also supported. Thermostats connected to the cube will be automatically detected and added
to the nymea system. If using the Max! Cube with nymea, it is recommended to keep the Cube's settings at factory defaults as
schedules set up in the Cube will be conflicting with schedules set through nymea.

### Max! Wall Thermostat

In order to use a [MAX! Wall Thermostat](http://www.eq-3.de/max-raumloesung-produktdetail/items/bc-tc-c-wm.html) with nymea a
Max! Cube LAN Gateway is required. Once the thermostat is connected to the Max! Cube, it will automatically appear in nymea,
provided the Cube has been set up in nymea.

### Max! Radiator Thermostat

In order to use a [MAX! Radiator Thermostat](http://www.eq-3.de/max-heizungssteuerung-produktdetail/items/bc-rt-trx-cyg.html)
with nymea a Max! Cube LAN Gateway is required. Once the thermostat is connected to the Max! Cube, it will automatically appear in nymea,
provided the Cube has been set up in nymea.
