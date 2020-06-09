# Unitec

This plugin allows to controll RF 433 MHz actors an receive remote signals from Unitec devices.

## Usage

The unitec socket units have a teach function. If you plug in the switch, a red light will start blinking. That means the socket is now in teaching mode. This Unitec switch (48111) can now be added to nymea with the desired Channel (A,B,C or D).

To pair the socket the power ON button must be pressed, and the switch needs to be in the pairing mode. If the pairing was successful, the switch will turn on.

NOTE: If a switch loses power once, it needs to be repaired, the devices do not store the teached channel.

## Supported Things

* Unitec switch (48111)
    * Select channel
    * Power on/off
    * No internet connection required

## Requirements

* 433MHz RF transceiver
* The package “nymea-plugin-anel” must be installed

## More

Unitec: http://www.unitec-elektro.de
