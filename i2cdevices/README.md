# I²C devices

This integration plugin adds support for some I²C devices.

Currently supported devices are:

* ADS1113/ADS1114/ADS1115
* Pi-16ADC
* SHT3x HT Sensor

## ADS1113/ADS1114/ADS1115

The ADS1113/1114/1115 is a 4 channel, I²C-compatible, 16-Bit ADC by Texas Instruments and supports
measuring input voltages from 0 to 6.144V.

### Usage

In order to use this device within nymea, it needs to be connected to the I²C bus. At least SDA, 
SCL, GND and VDD must be connected. VDD can be any desired voltage from 2V to 5.5V. Input channels
are to be connected at the according pins and will perform an analog to digital conversion of their
input voltage.

> IMPORTANT: The input voltage on the connected AINX channels must never exceed VDD + 0.3V.

The measured input value will be a floating point value from 0 to 1, depending on the selected input gain.
For instance, if the selected input gain is 4.096V, a voltage of 0V will be indicated in nymea as
0 while an input voltage of 4.096V will be represented as 1.

Setup can be done by performing a discovery for it in nymea. Please verify that the found results 
are matching with the address configuration of the device. If the address pin is not connected
at all, the I²C address will be 72 (0x48). It can be configured to another I²C address by connecting
the address pin as follows:

| Pin | Address (Hex) |
|:---:|:-------------:|
| GND | 72 (0x48)     |
| VDD | 73 (0x49)     |
| SDA | 74 (0x4a)     |
| SCL | 75 (0x4b)     |
-----------------------

By assigning different addresses, up to 4 such devices can be used on a single I²C bus.

> Note: At this point, this plugin does not support the devices dual channel mode.

## Pi-16ADC

The Pi-16ADC is a 16 channel analog/digital converter Raspberry Pi HAT by Alchemy Power and
supports measuring input voltage from 0 to 2.5V.

### Usage

In order to use this device within nymea, it needs to be plugged onto the Rasperry. The
analog input channels can be connected to to the respective channels. Unused channels
should be connected to GND in order to prevent them from oscillating.

The measured input value will be a floating point voltage value from 0V to 2.5V.

Setup can be done by performing a discovery for it in nymea. Please verify that the found results
are matching with the address configuration of the device. The default address is 118 (0x76).The 
jumpers on the board can be used to configure the I²C address of the board by setting the jumpers
to low (connect AX to GND), high (connect AX to +) or floating (remove jumper). The following 
table shows the possible address configurations:

| A2    | A1    | A0    | Address (Hex) |
|:-----:|:-----:|:-----:|:-------------:|
| Low   | Low   | Low   | 20 (0x14)     |
| Low   | Low   | High  | 22 (0x16)     |
| Low   | Low   | Float | 21 (0x15)     |
| Low   | High  | Low   | 38 (0x26)     |
| Low   | High  | High  | 52 (0x34)     |
| Low   | High  | Float | 39 (0x27)     |
| Low   | Float | Low   | 23 (0x17)     |
| Low   | Float | High  | 37 (0x25)     |
| Low   | Float | Float | 36 (0x24)     |
| High  | Low   | Low   | 86 (0x56)     |
| High  | Low   | High  | 100 (0x64)    |
| High  | Low   | Float | 87 (0x57)     |
| High  | High  | Low   | 116 (0x74)    |
| High  | High  | High  | 118 (0x76)    |
| High  | High  | Float | 117 (0x75)    |
| High  | Float | Low   | 101 (0x65)    |
| High  | Float | High  | 103 (0x67)    |
| High  | Float | Float | 102 (0x66)    |
| Float | Low   | Low   | 53 (0x35)     |
| Float | Low   | High  | 55 (0x37)     |
| Float | Low   | Float | 54 (0x36)     |
| Float | High  | Low   | 71 (0x47)     |
| Float | High  | High  | 85 (0x55)     |
| Float | High  | Float | 54 (0x54)     |
| Float | Float | Low   | 68 (0x44)     |
| Float | Float | High  | 70 (0x46)     |
| Float | Float | Float | 69 (0x45)     |
-----------------------------------------

By assigning different addresses, multiple such boards can be stacked on top of each other if
more than 16 channels are required.

> Note: At this point, this plugin does not support the devices dual channel mode.

Additional information ca be found at the devices users guide at 
[https://www.alchemy-power.com/wp-content/uploads/2017/03/Pi-16ADC-User-Guide.pdf](https://www.alchemy-power.com/wp-content/uploads/2017/03/Pi-16ADC-User-Guide.pdf).

## SHT3x HT Sensor

The SHT3x HT Sensor allows to measure the temperature and humidity and read the values using the I2C interface. Once connected to the I2C bus, 
the device can be discovered and added to the system.

Additional information can be found [here](https://www.sensirion.com/en/environmental-sensors/humidity-sensors/digital-humidity-sensors-for-various-applications/)
