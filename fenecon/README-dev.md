# Fenecon
## FEMS
--------------------------------

Nymea plugin to read some data from a FEMS. This can be extended in the future easily by adding different "Natures" of OpenEMS etc.

## How to expand the FEMS plugin

### Prerequisite

Whenever you want to expand the FEMS plugin, have a look at OpenEMS-Edge
[on Github](https://github.com/OpenEMS/openems)
See if the device you want to read is implemented yet. If so check the Nature (Interface) of the Component and look into the *ChannelIds*. As well as the default "id" (Config file).

The conjunction of the Component *id* and the *ChannelId* create the ChannelAddress.
The Address you want to read from (e.g. *ess0/Capacity* ess0 == the Component Id and Capacity == one of many ChannelIds).

------

### Implementation of the new FEMS device

Just add your device from the FEMS system into the integrationsplugin file.
Each of your state you want to update, should correspon to a FEMS ChannelAddress.
Those ChannelAddresses remain static, therefore put those ChannelAddresses into the
*constFemsPath.h*
like *static const QString ESS_ACTIVE_POWER_L3 = "_sum/EssActivePowerL3";*

After that, update your device/discover it in the *refreshConnection* method.
emit the *authothingsappeared* method, and update your device with states later on.
Use the existing code to see what my intention was.


## Documentation of the FEMS

[FEMS_App_REST_JSON_Benutzerhandbuch](https://docs.fenecon.de/de/_/latest/fems/apis.html#_fems_app_restjson_lese_und_schreibzugriff)
