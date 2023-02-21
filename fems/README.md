# fems
--------------------------------

Nymea plugin to read some data from a FEMS. This can be extended in the future easily by adding different "Natures" of OpenEMS etc.

## Purpose

This integration provides the ability to read the [FEMS.](https://docs.fenecon.de/de/_/latest/fems/apis.html#_fems_app_restjson_lese_und_schreibzugriff) The configuration adding  an IP Address, Port, username and password to access the FEMS and read data from it. Currently there is the support for the sum (represented by a meter), the battery and the FEMS State itself.

<table>
<tr>
<th>

<div>Integration name</div></th>
<th>

<div>Connection</div></th>
<th>

<div>Device types</div></th>
<th>

<div>Code status</div></th>
<th>

<div>Test stage</div></th>
<th>

<div>Repository link</div></th>
<th>

<div>Main dev</div></th>
</tr>
<tr>
<td>

<div>fems</div></td>
<td>

<div>REST</div></td>
<td>

<div>

Energystorage, energymeter,

Fems State

</div></td>
<td>

<div>🟧</div></td>
<td>

<div>🟧</div></td>
<td>

<div>

[nymea-plugins/fems](https://github.com/nymea/nymea-plugins/tree/master/fems)

</div></td>
<td>

<div>

Consolinno Energy Gmbh

</div></td>
</tr>
</table>

* **Status:**
  * Code: 🟧 Code is simple enough that there should be no problems. Tested as a standalone plugin.
  * Testing: 🟧 Passed limited internal testing.

## Compatibility

"Device": Not really a device, its a connection to a FEMS / OpenEMS System. You can read ANY data that the EMS holds. However I did implement only: The Battery, thinking its alway ess0 (documented here: [FEMS_App_REST_JSON_Benutzerhandbuch](https://docs.fenecon.de/de/_/latest/fems/apis.html#_fems_app_restjson_lese_und_schreibzugriff)) as well as sum. The device tries to find meter consumption/inverter by checking meter0,meter1 and meter2)

Additional Components are easy to implement, and the plugin is easy to expand. 

\
Leaflet: Probably only 1U0022.

## Installation

Since the device is in the main branch, it should be available as a debian package by installing the\
fems plugin

## Configuration

Configuration parameters are

* Name
* Host Address
* Port
* If the device is an Edge (e.g. not fems but openems directly)
* Username and Password