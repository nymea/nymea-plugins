# One-Wire

This integration plugin allows to integrate one-wire devices like temperature sensors and switches.

## Supported Things

* OWFS Interface
    * Gateway device
    * Initializes OWFS
* Temperature Sensors
    * Family Code 10
        * DS18S20
        * DS1820
        * DS18S20-PAR
        * DS1920
    * Family Code 22
        * DS1822
        * DS1822-PAR
    * Family Code 28
        * DS18B20
        * DS18B20-PAR
        * DS18B20X
    * Family Code 3B
        * DS1825
* Switches
    * Family Code 05 
        * Single channel switch
        * DS2405
    * Family Code 12
        * Dual channel switch
        * DS2406
        * DS2407
    * Family Code 3A 
        * Dual channel switch
        * DS2413
    * Family Code 29
        * Eight channel switch 
        * DS2408
        
## Usage
    
First step is to setup the "One wire interface". During the device setup it is required to enter the OWFS init arguments. Default arguments are "--i2c=ALL:ALL", means OWFS is about to scan for one-wire bus masters on all I2C interfaces.

You can simulate one-wire device with following init argument: "--fake=10,22,28,05"

More about init arguments here: https://www.owfs.org

The "One wire interface" thing has the toggle button to "Auto add one wire devices". Is this activated one-wire devices that get connected to the bus will appear in nymea automatically. 

NOTE: As long as the "Auto add one wire devices" feature is activated you won't be able to manually discover devices.

## Requirements

* The package “nymea-plugin-onewire” must be installed.

## More

This plug-in uses "OWFS" the one-wire file system: https://owfs.org


