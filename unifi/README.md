# UniFi

This plugin adds support to connect nymea to a UniFi network controller and monitor devices for presence.<br />
UniFi distinguishs between Controller running on a UniFi Console and Controller running on a Windows, macOS or Linux machine.

## Usage

In order to monitor network devices via a UniFi controller, it is required to configure the UniFi controller.<br />
The IP, as well as username and password for the UniFi controller must be provided. The port is optional.<br /> 
For Controller running on a UniFi console the port must be 443. Network controller running on Windows, macOS or Linux machines <br />
usually use port 8443. This is also the default value if empty.<br />
The login-user must have a local access only for the controller, a user with cloud access will not work.

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
