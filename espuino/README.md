# ESPuino

This plugin allows to integrate nymea with [ESPunio](https://github.com/biologist79/ESPuino),
a software for Rfid-controlled music players running on ESP32 hardware.

ESPuino boxes can't be bought off the shelf, but there's a (mostly
German speaking) community in the [ESPuino forum](https://forum.espuino.de/)
that provides a lot of documentation and ideas for building your own custom
ESPuino.

## Usage

As a prerequisite, ESPuino has to be compiled with WiFi and MQTT
enabled and the ESPuino must have been set up to connect to your home
WiFi. For the mDNS based auto discovery to work, the hostname must start with
`espuino`.

Then the ESPuino can be added to nymea.

## Supported features

- Display current title and cover art
- Control volume
- Play/pause
- Lock hardware controls
- Browse SD-Card and start playback
- Control LED brightness
- Configure sleep or repeat modes
- Show battery status
