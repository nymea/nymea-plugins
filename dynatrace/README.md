# Dynatrace

This plug-in enables nymea to control the Dynatrace Ufo. The Dynatrace UFO is a sophisticated status light by Dynatrace. 

This nymea integration supports auto discovering UFOs in the network and set them up as a color light in nymea.
Each ring can as well as the top logo can be controlled individually and morph and rotate effects can be enabled.

## Usage 

* Plug it in!
* Press the little black dot on the top. The UFO starts blinking blue and now offers a WiFi hotspot with the name “ufo”
* Connect to that hotspot, browse to http://192.168.4.1
* Through the Web UI connect to your own WiFi. You can also do it via the REST API: http://192.168.4.1/api?ssid=<ssid>&pwd=<pwd>
* Remember: WPA2 works well where Enterprise WPA2 is currently not supported
* While it reboots itself it will blink yellow. Once it has its assigned IP Address it will start visualizing its IP Address through a special „blink code“ as explained in the Quick Start Guide!
* Remember: the UFO will also try to register its hostname as „ufo“ with your DHCP server. If that works you can simply browse to http://ufo

## Supported Things

* Dynatrace UFO
    * Network discovery
    * Color light appearance
        * Set power
        * Set color
        * Set brightness
        * Set color temperature
    * Set color for each ring.
    * Set effect "Morph" or "Whirl" for each ring
    * Set color for logo

## Requirements

* The UFO device must be in the same local are network as nymea.
* TCP sockets on port 80 must not be blocked by the router.
* The package "nymea-plugin-dynatrace" must be installed.

## More

More information about the the UFO can be found
(here)[https://www.dynatrace.com/news/blog/using-dynatrace-devops-pipeline-state-ufo/]. 3D print layouts and
build instructions for your own UFO are available at this (github page)[https://github.com/Dynatrace/ufo].
