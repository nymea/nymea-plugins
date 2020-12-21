# Home Connect

Connects your Home Connect home appliances to nymea.

## Supported Things

Home connected home appliances, like:
* Oven
* Dishwasher
* Coffee maker
* Dryer
* Fridge
* Washer
* Cook Top
* Hood

## Requirements

* The package “nymea-plugin-homeconnect” must be installed
* Internet connection
* Home connect account

## Plug-In Settings

**Simulation Mode**
HomeConnect has a simulation server, where the home appliances are simulated. This feature is useful for demonstration and development.

**Control**
The control of the oven for example is only allowed with an authorized API key. The nymea community
API credentials does not have this permission. Enabling this without the permission will result in failing login attempts.

**Custom client key and secret**
You can register as developer on https://developer.home-connect.com, where you can obtain you own client credentials. The default client credentials are made available through the nymea community API key provider.

## More

Home Connect developer documentation:
https://developer.home-connect.com/
