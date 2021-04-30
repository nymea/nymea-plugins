import nymea
from pyairctrl.http_client import HTTPAirClient
from pyairctrl.coap_client import CoAPAirClient

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
            nymea.Param(purifierThingIpParamTypeId, device["ip"]),
            nymea.Param(purifierThingProtocolParamTypeId, "HTTP")
        ]

        info.addDescriptor(thingDescriptor)

    logger.log("Discovery finished.")
    info.finish(nymea.ThingErrorNoError)


def setupThing(info):
    ip = info.thing.paramValue(purifierThingIpParamTypeId)
    protocol = info.thing.paramValue(purifierThingProtocolParamTypeId)
    purifier = None
    try:
        if protocol == "CoAP":
            purifier = CoAPAirClient(ip, 5683, debug=True)
        elif protocol == "HTTP":
            purifier = HTTPAirClient(ip, debug=True)
    except Exception as e:
        logger.warn("Error connecting to Philips air purifier", str(e))
        info.finish(nymea.ThingErrorHardwareNotAvailable, "Unable to connect to the Philips Air Purifier. Please check if IP and Protocol are set correctly and the purifier is turned on.")
        return

    status = self._client.get_status(debug)
    if status is None:
        logger.warn("Error reading status from Philips air purifier")
        info.finish(nymea.ThingErrorHardwareNotAvailable, "Unable to read status from the Philips Air Purifier.")
        return

    logger.log("Philips air purifier status:")
    logger.log(json.dumps(status))

    purifiersMap[info.thing] = purifier
    info.finish(nymea.ThingErrorNoError)
    return


