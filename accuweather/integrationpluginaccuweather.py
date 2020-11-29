# Copyright 2013 - 2020, nymea GmbH
# Contact: contact@nymea.io
#
# This file is part of nymea.
# This project including source code and documentation is protected by
# copyright law, and remains the property of nymea GmbH. All rights, including
# reproduction, publication, editing and translation, are reserved. The use of
# this project is subject to the terms of a license agreement to be concluded
# with nymea GmbH in accordance with the terms of use of nymea GmbH, available
# under https://nymea.io/license
#
# GNU Lesser General Public License Usage
# Alternatively, this project may be redistributed and/or modified under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; version 3. This project is distributed in the hope that
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this project. If not, see <https://www.gnu.org/licenses/>.
#
# For any further details and any questions please contact us under
# contact@nymea.io or see our FAQ/Licensing Information on
# https://nymea.io/license/faq

import nymea
import threading
import asyncio
import threading

from accuweather import (
    AccuWeather,
    ApiError,
    InvalidApiKeyError,
    InvalidCoordinatesError,
    RequestsExceededError,
)
from aiohttp import ClientError, ClientSession

LATITUDE = 52.0677904
LONGITUDE = 19.4795644

apikey = ""
timers = {}

def init():
    logger.log("AccuWeather init")


def deinit():
    for timer in timers:
        timers[timer].cancel()

def discoverThings(info):
    logger.log("discoverThings")
    descriptor = nymea.ThingDescriptor(weatherThingClassId, "Weather")
    info.addDescriptor(descriptor)
    info.finish(nymea.ThingErrorNoError)


def setupThing(info):
    logger.log("setupThing", info.thing.name, "Creating connection.")
    timers[info.thing] = threading.Timer(30, getData, [info.thing])
    timers[info.thing].start()
    info.finish(nymea.ThingErrorNoError)


def postSetupThing(thing):
    logger.log("postSetupThing", thing.name)


def nameChanged(thing):
    logger.log("Thing name changed:", thing.name)


def thingRemoved(thing):
    logger.log("Thing removed:", thing.name)
    timers[thing].cancel()
    del timers[thing]


def executeAction(info):
    logger.log("executeAction")
    info.finish(nymea.ThingErrorNoError)


def configValueChanged(paramTypeId, value):
    if paramTypeId == accuWeatherPluginCustomApiKeyParamTypeId:
        apikey = value
        logger.log("API Key changed to", apikey)


def getData(thing):
    lat = thing.paramValue(weatherThingLatitudeParamTypeId)
    lon = thing.paramValue(weatherThingLatitudeParamTypeId)
    websession = ClientSession()
    accuweather = AccuWeather(
        apikey, websession, lat, lon
    )
    current_conditions = accuweather.async_get_current_conditions()
    logger.log("Current: {current_conditions}")
