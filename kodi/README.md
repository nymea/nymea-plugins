# Kodi

This plugin allows to integrate nymea with the [Kodi media center](http://kodi.tv/). The minimum requred version of
Kodi is 13 (Gotham).

## Setup

Is is required to enable the following settings in Kodi:

Navigate to Settings -> Services -> Control and activate "Alow Remote control via HTTP".

If nymea and Kodi are installed on the same system, activate "Allow remote control from applications on this system" or if
kodi is installed on a different system in the same network, activate "Allow remote control from applications on other systems".

In addition, it is recommended to activate "Announce services to other systems" to allow nymea discovery the kodi setup automatically.

Once those settings are activated, the kodi system can be added to nymea.

Note: If ZeroConf cannot be used, the device can be added manually and at least the IP, Port and HTTP Port parameters must be given.
It is recommended to configure the Kodi system to a static IP if the manual setup with IP is used. When using discovery, nymea
will re-detect kodi when its IP address changes.
