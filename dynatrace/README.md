# Dynatrace

This plug-in enables nymea to control the Dynatrace Ufo.

## Supported Things

* Dynatrace UFO
	* Auto discovery setup
	* Light interface
	* Control color of each ring
	* Control logo color
	* Set morph and rotate effect

## Requirements

* The UFO device must be in the same local are network as nymea.
* TCP sockets on port 80 must not be blocked by the router.
* The package "nymea-plugin-dynatrace" must be installed.

An excerpt of the Ufo (Blog Post)[https://www.dynatrace.com/news/blog/using-dynatrace-devops-pipeline-state-ufo/]

* Plug it in!
* Press the little black dot on the top. The UFO starts blinking blue and now offers a WiFi hotspot with the name “ufo”
* Connect to that hotspot, browse to http://192.168.4.1
* Through the Web UI connect to your own WiFi. You can also do it via the REST API: http://192.168.4.1/api?ssid=<ssid>&pwd=<pwd>
* Remember: WPA2 works well where Enterprise WPA2 is currently not supported
* While it reboots itself it will blink yellow. Once it has its assigned IP Address it will start visualizing its IP Address through a special „blink code“ as explained in the Quick Start Guide!
* Remember: the UFO will also try to register its hostname as „ufo“ with your DHCP server. If that works you can simply browse to http://ufo


## More 

More about the Dynatrace Ufo: https://github.com/Dynatrace/ufo

