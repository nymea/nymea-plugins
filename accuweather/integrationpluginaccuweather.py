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
import simplejson

from accuweather import (
    AccuWeather,
    ApiError,
    InvalidApiKeyError,
    InvalidCoordinatesError,
    RequestsExceededError,
)
from aiohttp import ClientError, ClientSession
from urllib.request import urlopen, Request


LATITUDE = 52.0677904
LONGITUDE = 19.4795644

apikey = ""
timers = {}
loop = asyncio.get_event_loop()
accuweather = None

def init():
    # AccuWeather lib needs asyncio, let's start the event loop for it.
    loop.run_forever();


async def initAccuWeather(latitude, longitude):
    # Need "global" here to be able to modify the global variable
    global accuweather
    logger.log("Initializing Accuweather, latitude", latitude, "longitude:", longitude)
    websession = ClientSession()
    accuweather = AccuWeather(apikey, websession, latitude, longitude)


def deinit():
    loop.stop()
    loop.close()

# Not needed for asyncio atm, so commented out atm...
#    for timer in timers:
#        timers[timer].cancel()


def discoverThings(info):
    request = Request("http://ip-api.com/json")
    data = simplejson.load(urlopen(request))
    descriptor = nymea.ThingDescriptor(weatherThingClassId, "%s - %s" % (data["city"], data["country"]), "%s - %s" % (data["lat"], data["lon"]))
    descriptor.params = [nymea.Param(weatherThingLatitudeParamTypeId, data["lat"]), nymea.Param(weatherThingLongitudeParamTypeId, data["lon"])]
    info.addDescriptor(descriptor)
    info.finish(nymea.ThingErrorNoError)


async def getCurrentConditions(thing):
    logger.log("running get current conditions")
    currentConditions = await accuweather.async_get_current_conditions()
    logger.log("after await", currentConditions)

    thing.setStateValue(weatherWeatherDescriptionStateId, currentConditions["WeatherText"]);

    thing.setStateValue(weatherTemperatureStateId, currentConditions["Temperature"]["Metric"]["Value"]);
    thing.setStateValue(weatherHumidityStateId, currentConditions["Humidity"]["Metric"]["Value"]);
    thing.setStateValue(weatherPressureStateId, currentConditions["Pressure"]["Metric"]["Value"]);

    thing.setStateValue(weatherWindSpeedStateId, currentConditions["Wind"]["Direction"]["Degrees"]);
    thing.setStateValue(weatherWindDirectionStateId, currentConditions["Wind"]["Speed"]);

    thing.setStateValue(weatherMinTemperatureStateId, currentConditions["TemperatureSummary"]["Past24HourRange"]["Minimum"]);
    thing.setStateValue(weatherMaxTemperatureStateId, currentConditions["TemperatureSummary"]["Past24HourRange"]["Maximum"]);

    thing.setStateValue(weatherVisibilityStateId, currentConditions["Visibility"]["Metric"]["Value"]);
    thing.setStateValue(cloudinessStateId, currentConditions["CloudCover"]);
    #descriptor = nymea.ThingDescriptor(weatherThingClassId, "Weather")
    #info.addDescriptor(descriptor)
    #info.finish(nymea.ThingErrorNoError)

# Haven't looked at anything below at all so far

def setupThing(info):
    logger.log("setupThing", info.thing.name, "Creating connection.")
    #timers[info.thing] = threading.Timer(30, getData, [info.thing])
    #timers[info.thing].start()

    latitude = info.thing.paramValue(weatherThingLatitudeParamTypeId)
    longitude = info.thing.paramValue(weatherThingLongitudeParamTypeId)

    if accuweather == None:
        asyncio.run_coroutine_threadsafe(initAccuWeather(latitude, longitude), loop);

    future = asyncio.run_coroutine_threadsafe(getCurrentConditions(info.thing), loop)
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
    lon = thing.paramValue(weatherThingLongitudeParamTypeId)

    logger.log("Current: {current_conditions}")
