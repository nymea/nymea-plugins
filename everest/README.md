# nymea-plugin-everest

This nymea integration plugin allows to connect to any EVerest instance in the network or on localhost.

## Using the RpcApi module

By default the integration requires the use of the RpcApi module, which provides a JSON RPC interface based on a websocket communication to the EVerest core.


## Using the MQTT based API module

If required, the MQTT based API module of everest can be used.

By default the MQTT API module will not be initialized unless explictily enabled by creating following file

    touch /etc/nymea/everest-mqtt

Each connector has to be added manually using the discovery. Once added, the integration creates an EV charger within nymea which makes it available to the energy manager and the overall nymea eco system.

Known issues:

* There is currently no method available on the EVerest API module to switch phases
* There are no additional information which could be used to identify the instance, like serialnumber, manufacturer or model
* The action execution is not very clean implemented. There is no feedback if the execution was successfully
