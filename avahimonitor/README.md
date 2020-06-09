# Avahi Monitor

This integration plug-in allows nymea to discover and monitor zeroconf services in the local area network. 
Avahi is a free zeroconf implementation and is utilised by nymea. In nymea:app an added Avahi thing will appear as a presence sensor.

## Supported Things

* Avahi Monitor
    * List all zeroconf services in the local area network during device setup
    * Displays the presence of the zeroconf service
    * No internet or cloud connection required

## Requirements

* The zeroconf service must be in the same local area network as nymea.
* The router must not block avahi/zeroconf multicast messages
* The package “nymea-plugin-avahimonitor” must be installed.

## More
See http://www.zeroconf.org for more information about the Zeroconf discovery protocol, or see https://www.avahi.org for more information about avahi.
