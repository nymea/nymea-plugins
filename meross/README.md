# Meross

This integration plugin allows nymea to control meross power sockets with energy metering.

## Supported devices

Currently, only the meross smart plug with energy metering MSS310 is supported.

## Requirements

The meross smart plug needs to be set up with the meross app and connected to the same network
as the nymea system. The device can be discovered by nymea in the local network and will communicate
via the local REST API of the device. However, given that the devices require a signing key to respond
to API calls, the plugin will require the user to log into meross cloud during setup and will
obtain the key from the cloud API. No other calls will be made to the meross cloud and the
user credentials will not be stored (thus need to be re-entered when setting up multiple devices).

The device can be used in nymea along with the meross app, or, if desired, also disconnected from
the meross cloud (e.g. by blocking internet access via a firewall) without impairing functionality
within nymea.
