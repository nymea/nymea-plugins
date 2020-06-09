# DoorBird

This plugin integrates DoorBird video doorbells into nymea. All the communication between nymea and the DoorBird device happens locally and will work without internet connection.

## Supported Things

* All video doorbells
	* Auto discovery 
	* Doorbell presses
	* Motion events
	* Enable/disable IR light
	* Switching door relays
	* No internet connection required

NOTE: This plug-in does not handle any video- or audio stream.

## Requirements

* The DoorBird device must be in the same local area network as nymea.
* The router must not block ZeroConf/mDNS multicast messages.
* TCP Sockets on port 80 must not be blocked by the router.
* The user must have the permission to act as DoorBird API-operator.
	* You can check the permissions in the DoorBird app.
* The package "nymea-plugin-doorbird" must be installed.

## More

https://www.doorbird.com
