# Keba Wallbox

This plugin allows to control Keba KeContact EV-Charging stations. 

## Supported Things

* KeContact Wallbox
	* P20 (certain models)
	* P30
		* c-series
		* x-series
	* BMW (certain models)
	* [Keba Deutschland Edition](https://a.storyblok.com/f/40131/x/fc59dc7bf7/datenblatt_deutschland_edition.pdf) (DE440)
(by March 2022)

Please make sure that your model supports communication through the UDP protocol.
The [product overview](https://www.keba.com/download/x/21634787f7/kecontact-p30_productoverview_en.pdf) helps to verify about your models capabilities.

## Requirements

* nymea and the wallbox are required to be in the same network. 
* UDP Port 7090 must not be blocked by a firewall or router.
* The package "nymea-plugin-keba" must be installed.
* KeContact P20 Charging station with network connection (LSA+  socket). Firmware version: `2.5` or higher.
* KeContact P30 Charging station or BMW  wallbox. Firmware version `3.05` of higher.
* **Enabled UDP function with DIP-switch `DSW1.3 = ON`.**

## More information

https://www.keba.com/en/emobility/products/product-overview/product_overview
