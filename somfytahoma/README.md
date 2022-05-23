# Somfy TaHoma

This plugin adds support for Somfy smarthome devices through the local
Somfy TaHoma API offered by Somfy Gateways with 'Developer Mode'
enabled.

See <https://developer.somfy.com/developer-mode> and <https://github.com/Somfy-Developer/Somfy-TaHoma-Developer-Mode>
for more information.

## Prerequisites

This plugin requires a Somfy TaHoma gateway to which your Somfy devices
are connected. The gateway needs to be registered to the Somfy API.
Follow the user guide of the gateway for detailed instructions.

## Usage

In order to interact with your Somfy devices, add your TaHoma gateway as new
'Thing' to nymea. All supported devices will show up automatically after
entering your personal username + password for the Somfy TaHoma API.

## Supported devices

Currently this plugin supports all roller shutters, blinds, garage
door, awning drives, lights and smoke detectors that are connectable to the
TaHoma gateway. These are Somfy iO devices as well as RTS devices.
