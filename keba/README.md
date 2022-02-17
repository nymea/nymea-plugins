# Keba Wallbox

This plugin allows to control Keba KeContact EV-Charging stations. 

## Supported Things

* KeContact Wallbox
	* P20 (certain models)
	* P30 (certain models)
	* BMW (certain models)

Please make sure that your model supports communication through the UDP protocol.
Only `c-series` and `x-series` have this ability (by Feb 2022). You can check the [product overview](https://www.keba.com/download/x/21634787f7/kecontact-p30_productoverview_en.pdf) 
on KEBA home page to verify your models capabilities.

## Requirments

* nymea and the wallbox are required to be in the same network. 
* UDP Port 7090 must not be blocked by a firewall or router.
* The package "nymea-plugin-keba" must be installed.
* KeContact P20 Charging station with network connection (LSA+  socket). Firmware version: `2.5` or higher.
* KeContact P30 Charging station or BMW  wallbox. Firmware version `3.05` of higher.
* **Enabled UDP function with DIP-switch `DWS1.3 = ON`.**

## More information

https://www.keba.com/en/emobility/products/product-overview/product_overview
