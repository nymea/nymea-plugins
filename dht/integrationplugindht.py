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
import board
import adafruit_dht

piPinMap = {
    "2": board.D2,
    "3": board.D3,
    "4": board.D4,
    "5": board.D5,
    "6": board.D6,
    "7": board.D7,
    "8": board.D8,
    "9": board.D9,
    "10": board.D10,
    "11": board.D11,
    "12": board.D12,
    "13": board.D13,
    "16": board.D16,
    "17": board.D17,
    "18": board.D18,
    "19": board.D19,
    "20": board.D20,
    "21": board.D21,
    "22": board.D22,
    "23": board.D23,
    "24": board.D24,
    "25": board.D25,
    "26": board.D26,
    "27": board.D27
}

pollTimer = None

def setupThing(info):
    logger.log("SetupThing for DHT11/22:", info.thing.name)

    try:
        refreshSensor(info.thing)
        logger.log("Refreshed sensor values successfully. Setup went well.")
        info.finish(nymea.ThingErrorNoError)
    except:
        logger.log("Error fetching sensor values. Setup failed.")
        info.finish(nymea.ThingErrorHardwareFailure, "Error reading sensor values. Please verify if the sensor is properly connected and you've selected the correct GPIO.")
        return

    global pollTimer
    if pollTimer is None:
        pollTimer = nymea.PluginTimer(5, pollSensors)


def thingRemoved(thing):
    if len(myThings()) is 0:
        global pollTimer
        del pollTimer
        pollTimer = None


def refreshSensor(thing):
    logger.log("Refreshing sensor values:", thing.name)

    if thing.thingClassId == dht11ThingClassId:
        gpio = thing.paramValue(dht11ThingRpiGpioParamTypeId)
        logger.log("GPIO pin:", gpio)
        dhtDevice = adafruit_dht.DHT11(piPinMap[gpio], use_pulseio=False)
        thing.setStateValue(dht11TemperatureStateTypeId, dhtDevice.temperature)
        thing.setStateValue(dht11HumidityStateTypeId, dhtDevice.humidity)

    elif thing.thingClassId == dht22ThingClassId:
        gpio = thing.paramValue(dht22ThingRpiGpioParamTypeId)
        logger.log("GPIO pin:", gpio)
        dhtDevice = adafruit_dht.DHT22(piPinMap[gpio], use_pulseio=False)
        thing.setStateValue(dht22TemperatureStateTypeId, dhtDevice.temperature)
        thing.setStateValue(dht22HumidityStateTypeId, dhtDevice.humidity)

    else:
        logger.crit("Unhandled thing class:", thing.thingClassId)


def pollSensors():
    for thing in myThings():
        try:
            refreshSensor(thing)
        except:
            logger.warn("Error refreshing sensor values for", thing.name)


