# Anel-Elektronik AG

This integration allows nymea to control ANEL Elektronik NET-PwrCtrl power sockets.
Once a NET-PwrCtl device is added, the associated sockets appear as child devices. Deleting the NET-PwerCtl device will also remove the associated sockets. This means each socket can be named and will appear as an individual socket inside the nymea:app.

## Supported Things

* NET-PwrCtl HOME
* NET-PwrCtl PRO
    * Includes temperature sensor
* NET-PwrCtl ADV
    * Includes temperature sensor
* NET-PwrCtl HUT
    * Includes temperature sensor
    * Includes humidity sensor
    * Includes light intensity sensor

Each of the mentioned device types supports following features:

* Auto discovery setup
* Auto rediscovery on IP address change
* Secure connection with username and password
* Get and set the state of each socket
* No internet or cloud connection required

## Requirements

* The NET-PwrCtrl device must be in the same local area network as nymea.
* UDP multicast on Port 30303 must not be blocked by the router.
* TCP Sockets on port 80 must not be blocked by the router.
* Access to the NET-PwrCtl device login credentials.
* The package “nymea-plugin-anel” must be installed

## More

See https://anel-elektronik.de for a detailed description of the devices.
