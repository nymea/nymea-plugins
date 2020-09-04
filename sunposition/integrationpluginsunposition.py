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

