# SENEC LAN PV inverter extension

Based on upstream `nymea/nymea-plugins`, branch `add-senec-integration`.

## Implementation

The existing LAN client already obtains `ENERGY.GUI_INVERTER_POWER` from the
SENEC `/lala.cgi` API and exposes it through
`SenecStorageLan::inverterPower()`.

The plugin now:
- automatically creates one `SENEC PV inverter` child of the LAN storage,
  reusing the existing `senecInverter` thing class and `solarinverter` interface;
- publishes `currentPower` in watts and estimates `totalEnergyProduced`
  using the plugin's existing sample integration;
- propagates the LAN storage connection state to the inverter;
- retains the previous SENEC.Connect cloud behavior and optional LAN meter.

**Power sign:** The cloud implementation sends negative values for generated PV
power. This extension assumes positive LAN `GUI_INVERTER_POWER` represents PV
generation and negates it to match that existing convention. Validate the sign
on a real SENEC device under daylight and at night before merging upstream.
If the LAN field is signed differently on other devices, adjust this mapping.

**Energy total:** This is calculated from samples, not a lifetime counter from
SENEC. The existing `updateEnergy` method skips first samples and intervals
with gaps above 300 seconds. After installation it cannot reconstruct past
PV generation and may undercount during interruptions.

## Verification on a test system

1. Build against the target nymea/Qt toolchain and deploy to a test instance.
2. Verify the existing SENEC LAN storage and meter still work.
3. Confirm one automatically created `SENEC PV inverter` child appears.
4. Compare `currentPower` with the SENEC PV output during daytime and at night.
5. Confirm `totalEnergyProduced` increases only when PV is generating and
   remains stable across a plugin restart.
6. Verify `connected` follows LAN connectivity and cloud integration is unchanged.

Not yet built or tested on the user's Raspberry Pi. No running installation
has been modified.
