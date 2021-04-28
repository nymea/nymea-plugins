import nymea
from pyairctrl.http_client import HTTPAirClient

# py-air-control library: https://github.com/rgerganov/py-air-control/blob/master/pyairctrl/airctrl.py

# Map nymea things to library devices
purifiersMap = {}


def discoverThings(info):
    logger.log("Discovering Philips air purifiers...")
    try:
        devices = HTTPAirClient.ssdp(timeout=1, repeats=3)
    except Exception as e:
        logger.warn("Discovery failed:", str(e))
        info.finish(nymea.ThingErrorHardwareNotAvailable, "No air purifiers found. Make sure it is connected to your WiFi.");
        return

    for device in devices:
        logger.log("Found Philips air purifier:", device["ip"])
        thingDescriptor = nymea.ThingDescriptor(purifierThingClassId, device["modelName"], device["modelNumber"], device["friendlyName"])
        # TODO: Store serial number instead of IP in params and re-discover in setupThing
        thingDescriptor.params = [
            nymea.Param(purifierThingIpParamTypeId, device["ip"])
        ]

        info.addDescriptor(thingDescriptor)

    logger.log("Discovery finished.")
    info.finish(nymea.ThingErrorNoError)


def setupThing(info):
    ip = info.paramValue(purifierThingIpParamTypeId)
    purifier = HTTPAirClient(ip, debug=True)
    purifiersMap[info.thing] = purifier
    info.finish(nymea.ThingErrorNoError)
    return


