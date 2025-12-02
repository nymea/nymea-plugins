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
from sunposition import sunpos
from datetime import datetime
from urllib.request import urlopen, Request
import simplejson
import time

loopRunning = False

def init():
    global loopRunning
    loopRunning = True

    while loopRunning:
        time.sleep(5)
        now = datetime.utcnow()
        for thing in myThings():
            lat = thing.paramValue(sunPositionThingLatitudeParamTypeId)
            lon = thing.paramValue(sunPositionThingLongitudeParamTypeId)
            az,zen = sunpos(now, lat, lon, 0)[:2]
            angle = 90 - zen.item()
            logger.log("Updating thing", thing.name, "Angle:", angle)
            thing.setStateValue(sunPositionAngleStateTypeId, angle)


def deinit():
    global loopRunning
    loopRunning = False


def discoverThings(info):
    request = Request("http://ip-api.com/json")
    data = simplejson.load(urlopen(request))
    descriptor = nymea.ThingDescriptor(sunPositionThingClassId, "%s - %s" % (data["city"], data["country"]), "%s - %s" % (data["lat"], data["lon"]))
    descriptor.params = [nymea.Param(sunPositionThingLatitudeParamTypeId, data["lat"]), nymea.Param(sunPositionThingLongitudeParamTypeId, data["lon"])]
    info.addDescriptor(descriptor)
    info.finish(nymea.ThingErrorNoError)


def setupThing(info):
    info.finish(nymea.ThingErrorNoError)

