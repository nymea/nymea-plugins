# SPDX-License-Identifier: GPL-3.0-or-later

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#
# Copyright (C) 2013 - 2024, nymea GmbH
# Copyright (C) 2024 - 2025, chargebyte austria GmbH
#
# This file is part of nymea-plugins.
#
# nymea-plugins is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# nymea-plugins is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

import nymea
from bimmer_connected.account import ConnectedDriveAccount
from bimmer_connected.country_selector import Regions
from bimmer_connected.vehicle_status import ChargingState

accountsMap = {}
pollTimer = None

regions = {
    "Rest of world": Regions.REST_OF_WORLD,
    "North America": Regions.NORTH_AMERICA,
    "China": Regions.CHINA,
}


def findByParam(cls, param, value):
    for thing in myThings():
        if thing.thingClassId == cls and thing.paramValue(param) == value:
            return thing


def init():
    logger.log("Initializing Bimmerconnected plugin")


def startPairing(info):
    info.finish(nymea.ThingErrorNoError)


def confirmPairing(info, username, secret):
    try:
        region = regions[info.paramValue(accountThingRegionParamTypeId)]
        account = ConnectedDriveAccount(username, secret, region)
        account.update_vehicle_states()
        info.finish(nymea.ThingErrorNoError)
        pluginStorage().beginGroup(info.thingId)
        pluginStorage().setValue("username", username)
        pluginStorage().setValue("password", secret)
        pluginStorage().endGroup()
        del account

    except Exception as e:
        logger.warn(f"Error setting up BMW account: {str(e)}")
        info.finish(nymea.ThingErrorAuthenticationFailure)


def setupThing(info):
    # Setup for the account
    if info.thing.thingClassId == accountThingClassId:
        logger.log("SetupThing for account:", info.thing.name)

        region = regions[info.thing.paramValue(accountThingRegionParamTypeId)]
        pluginStorage().beginGroup(info.thing.id)
        username = pluginStorage().value("username")
        password = pluginStorage().value("password")
        pluginStorage().endGroup()

        try:
            account = ConnectedDriveAccount(username, password, region)
            account.update_vehicle_states()

            accountsMap[info.thing.id] = account
        except Exception as e:
            # Login error
            logger.warn(f"Error setting up BMW account: {str(e)}")
            info.finish(nymea.ThingErrorAuthenticationFailure, str(e))
            return

        # Mark the account as logged-in and connected
        info.thing.setStateValue(accountLoggedInStateTypeId, True)
        info.thing.setStateValue(accountConnectedStateTypeId, True)

        # Login went well, finish the setup
        info.finish(nymea.ThingErrorNoError)

        logger.log(
            f"Found {len(account.vehicles)} vehicles: {', '.join([v.name for v in account.vehicles])}"
        )

        thingDescriptors = []
        for vehicle in account.vehicles:
            if any(
                thing.thingClassId == vehicleThingClassId
                and thing.paramValue(vehicleThingVinParamTypeId) == vehicle.vin
                for thing in myThings()
            ):
                continue

            if not vehicle.has_hv_battery:
                logger.log(
                    f"Ignoring combustion vehicle {vehicle.name} ({vehicle.vin[-7:]})"
                )
                continue

            logger.log(
                f"Adding new vehicle {vehicle.name} ({vehicle.vin[-7:]}) to the system with parent {info.thing.id}"
            )
            thingDescriptor = nymea.ThingDescriptor(
                vehicleThingClassId,
                "BMW {} ({})".format(vehicle.name, vehicle.vin[-7:]),
                parentId=info.thing.id,
            )
            thingDescriptor.params = [
                nymea.Param(vehicleThingVinParamTypeId, vehicle.vin)
            ]
            thingDescriptors.append(thingDescriptor)

        autoThingsAppeared(thingDescriptors)

        # If no poll timer is set up yet, start it now
        logger.log("Creating polltimer @ setupThing")
        global pollTimer
        if pollTimer is None:
            pollTimer = nymea.PluginTimer(60 * 5, pollService)
            logger.log("timer interval @ setupThing", pollTimer.interval)

    # Setup for the vehicles
    if info.thing.thingClassId == vehicleThingClassId:
        info.finish(nymea.ThingErrorNoError)


def postSetupThing(thing):
    if thing.thingClassId == accountThingClassId:
        pollService()


def pollService():
    logger.log("Polling BMW Connect")
    for thing in myThings():
        if thing.thingClassId != accountThingClassId:
            continue

        account = accountsMap[thing.id]
        try:
            account.update_vehicle_states()
        except:
            logger.warn(f"Error refreshing vehicle states for account {thing.name}")

        for vehicle in account.vehicles:
            logger.log(f"Updating vehicle with VIN {vehicle.vin}")

            thing = findByParam(
                vehicleThingClassId, vehicleThingVinParamTypeId, vehicle.vin
            )

            thing.setStateValue(
                vehicleBatteryLevelStateTypeId,
                vehicle.status.charging_level_hv,
            )
            thing.setStateValue(
                vehiclePluggedInStateTypeId,
                vehicle.status.connection_status == "CONNECTED",
            )
            thing.setStateValue(
                vehicleChargingStateStateTypeId,
                "charging"
                if vehicle.status.charging_status == ChargingState.CHARGING
                else "idle",
            )


def executeAction(info):
    if info.actionTypeId == vehicleCapacityActionTypeId:
        info.thing.setStateValue(
            vehicleCapacityStateTypeId,
            info.paramValue(vehicleCapacityActionCapacityParamTypeId),
        )
        info.finish(nymea.ThingErrorNoError)
    else:
        logger.error(f"Unhandled action: {info.action.id}")


def thingRemoved(thing):
    global pollTimer

    if thing.thingClassId == accountThingClassId:
        del accountsMap[thing.id]

    if len(myThings()) == 0 and pollTimer is not None:
        pollTimer = None
