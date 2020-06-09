# UniFi

This plugin adds support to connect nymea to a UniFi network controller and monitor devices for presence.

## Usage

In order to monitor network devices via a UniFi controller, it is required to configure the UniFi controller.
The IP, as well as username and password for the UniFi controller must be provided.

Once the controller is added to the system, additional Wi-Fi devices may be added by starting a discovery of
UniFi clients. After a client is addded, it will appear as presence sensor in the system.
Client devices, by default have a one minute grace period before they are marked as offline. This value can
be changed in the device settings. A value of 0 will immediately mark a device as offline.

## Supported Things

* UniFi Controller
    * 
* Client

## Requirements

* The package “nymea-plugin-unifi” must be installed

## More

https://www.ui.com/software/
