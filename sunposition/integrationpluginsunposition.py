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

