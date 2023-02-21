# kostalpico
--------------------------------

Nymea plugin to read the Kostal Pico MP Plus. This can be extended in the future to read the Pico MP Plus as different Types of Inverters etc.

## Purpose

This integration provides the ability to read the [Kostal Pico MP Plus](https://www.kostal-solar-electric.com/de-de/produkte/solar-wechselrichter/piko-mp-plus/-/media/document-library-folder---kse/2021/02/17/15/19/ba-protocol_piko-mp-plus_en.pdf "Kostal Pico MP PLus"). The configuration allows via auto discovery adding a Router that forwards a xml file to read the kostal pico mp plus data(measurements.xml and yields.xml). This data is atm: CurrentPower and TotalEnergyProduced

| Integration name | Connection | Device types | Code status | Test stage | Repository link | Main dev |
|------------------|------------|--------------|-------------|------------|-----------------|----------|
| kostalpico | HTTP/XML | Smartmeter | 🟧 | 🟧 | [nymea-plugins/kostalpico](https://github.com/nymea/nymea-plugins/tree/master/kostalpico) | Consolinno Energy GmbH |

* **Status:**
  * Code: 🟧 Code is simple enough that there should be no problems. Tested as a standalone plugin, no idea if integration with ConEMS works.
  * Testing: 🟧 Passed limited internal testing.

## Compatibility

Device: Kostal Pico MP Plus. As a smartmeter Possible to extend with other devices.\

## Installation

Currently not yet available as a debian package. You need to compile it yourself and copy the .so manually to your Leaflet. \
See if

## Configuration

Configuration parameters are

- Name
- Mac address (usually detected by autodiscovery and selecting this one)