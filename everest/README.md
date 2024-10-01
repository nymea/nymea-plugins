# nymea-plugin-everest

This nymea integration plugin allows to connect to any EVerest instance in the network or on localhost with an API module and to add every connector configured on this instance.
Each connector has to be added manually using the discovery. Once added, the integration creates an EV charger within nymea which makes it available to the energy manager and the overall nymea eco system.

Known issues:

* There is currently no method available on the EVerest API module to switch phases
* There are no additional information which could be used to identify the instance, like serialnumber, manufacturer or model
* The action execution is not very clean implemented. There is no feedback if the execution was successfully
