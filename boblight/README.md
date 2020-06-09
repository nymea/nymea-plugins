# Boblight

This plugin allows to communicate with a [Boblight server](https://code.google.com/p/boblight/)

The boblight server needs to be installed and running. In nymea it is required to setup the Boblight server connection first, during the setup you can configure how many channels you want to add. Each channel will appear auotamtically as an own device insided nymea. Default configuration for the Boblight server is `localhost:19333`, you can change the parameters during device setup.

Once the Boblight devices are added you can control them like any other color light in nymea.


## Supported Things

* Boblight Server
	* Gateway device
	* Define channel count
* Boblight Channel
	* Color light
	* Set color
	* Set color temperature
	* Set brightness
	* Set power

**Generall**

* No internet connection required

## Requirements

* Boblight server running on `localhost:19333` or any other reachable IP address.
* Boblight device connected to the Boblight server.
* The package "nymea-plugin-boblight" must be installed.

## More 

For more information regarding the project please visit the [project site](https://sites.google.com/site/wikikrautbox/krautbox/hardware/boblight).
