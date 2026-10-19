# SENEC.Home

Integrates SENEC.Home energy storages into the system.

## Local network (SENEC.Home V2 / V3)

The storage is discovered in the local network and polled directly. Optionally the grid meter of the storage can be added using the "Add meter" setting.

## SENEC.Connect (SENEC.Home 4, P4, E4)

V4 systems do not provide a local API and are integrated using the official read-only SENEC.Connect cloud API.

1. Sign in on https://developer.senec.com with your SENEC account.
2. Subscribe to the SENEC.Connect product (SENEC.Data 45000).
3. Copy the subscription key from your profile and enter it when adding the SENEC.Connect thing.

All systems registered to the account will appear automatically, each with a grid meter and a PV inverter. The data is updated once per minute due to the request limit of the API. The energy totals of the meter and the inverter are calculated from the power values.

## More

https://senec.com
