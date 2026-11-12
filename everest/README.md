# nymea-plugin-everest

This nymea integration plugin allows to connect to any EVerest instance in the network or on localhost.

## Using the RpcApi module

By default the integration requires the use of the RpcApi module, which provides a JSON RPC interface based on a websocket communication to the EVerest core.

## Local AC charger with RFID

Install and enable `nymea-rfid-provider.service` on the charger before nymead
starts. The provider owns the RFID hardware; EVerest must not run its own reader
module against it. The first provider backend supports PN7160 through
EVerest's `libnfc-nci`, built with libgpiod 2.x support for named GPIO lines.

When creating a missing RPC charger child, the integration selects **EVerest AC
Charger with RFID** if the provider's compatible system D-Bus interface exists
and the connection is localhost, has exactly one EVSE with one connector, and
supports AC without DC. Provider readiness does not select the class: an
unavailable reader still belongs to an RFID-capable charger. Remote connections,
DC, MQTT and ambiguous connector topologies retain their existing behavior.

The provider probe completes before new charger descriptors are emitted.
`connected` reports RPC connectivity; `rfidAvailable` reports usable RFID
connectivity. Provider restarts restore subscriptions without recreating things.
Scans during outages are not replayed. More than one available local connection
disables RFID routing rather than assigning a scan to an arbitrary charger.

Existing charger things are preserved. To change an existing plain AC charger,
explicitly remove it and rediscover it by reconnecting/restarting its EVerest
connection after enabling the provider. The new thing has a new identity:
review thing-linked rules, energy settings and other references; historical data
remains associated with the old identity. There is no automatic migration.

The standard nymea RFID experience handles enrollment, authorization identity and
charging policy. The integration supplies its required identity states and
enrollment action. It forwards `tagDetected(code)` once per PN7160 presentation;
holding a tag does not repeatedly trigger authorization or stop charging.
Identifiers use uppercase UID hexadecimal in reader-library byte order with
leading zeroes and no separators. Arrival/removal notifications are available
only on the provider's D-Bus interface in this version.

`tagAccepted` and `tagRejected` currently log readable feedback, clear enrollment
mode and succeed. They do not call EVerest: the corresponding RPC feedback
methods are not yet available. This does not replace the RFID experience's
authorization decision.

### RFID regression tests

The standalone Qt 6 tests need Qt Core, DBus, Network, WebSockets, Test,
`dbus-run-session`, and the sibling nymea source checkout for headers.

```sh
cmake -S everest/tests -B build-everest-rfid-tests -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build build-everest-rfid-tests -j
ctest --test-dir build-everest-rfid-tests --output-on-failure
```

The tests use a private bus and fake provider; real PN7160 tag formats, physical
removal, charging enrollment/authorization and system boot ordering require
target acceptance testing.


## Using the MQTT based API module

If required, the MQTT based API module of everest can be used.

By default the MQTT API module will not be initialized unless explictily enabled by creating following file

    touch /etc/nymea/everest-mqtt

Each connector has to be added manually using the discovery. Once added, the integration creates an EV charger within nymea which makes it available to the energy manager and the overall nymea eco system.

Known issues:

* There is currently no method available on the EVerest API module to switch phases
* There are no additional information which could be used to identify the instance, like serialnumber, manufacturer or model
* The action execution is not very clean implemented. There is no feedback if the execution was successfully
