# One wire

This plugin allows to add one wire devices through the one wire file system.


## One wire interface device

This device initializes OWFS, during the device setup you can set OWFS init arguments.
Default arguments are "--i2c=ALL:ALL" to scan for one-wire devices on all I2C interfaces.

You can simulate one-wire device with following init argument: "--fake=10,22,28,05"

More about init arguments here: https://www.owfs.org

## Supported one-wire devices

* Family Code 10 - Temperature Sensors
..* DS18S20
..* DS1820
..* DS18S20-PAR
..* DS1920
* Family Code 22 - Temperature Sensors
..* DS1822
..* DS1822-PAR
* Family Code 28 - Temperature Sensors
..* DS18B20
..* DS18B20-PAR
..* DS18B20X
* Family Code 3B - Temperature Sensors
..* DS1825
* Family Code 05 - Single channel switch
..* DS2405
* Family Code 12 - Dual channel switch
..* DS2406
..* DS2407
* Family Code 3A - Dual channel switch
..* DS2413
* Family Code 29 - Eight channel switch
..* DS2408
