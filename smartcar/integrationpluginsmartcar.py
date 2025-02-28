import nymea
import smartcar
import urllib.parse as urlparse
from urllib.parse import parse_qs
import threading
import time
import json
import datetime

smartcarClient = None
defaultPollTimerInterval = 20 # will be read from settings
pollTimers = {}
oauthSessions = {}

def init():
    logger.log("Smartcar init")    

def deinit():    
    # If we started any poll timers, cancel at shutdown
    for timerId in pollTimers:
        if timerId in pollTimers and pollTimers[timerId] is not None:
            pollTimers[timerId].cancel()

def startPairing(info):
    logger.log("Start pairing")
    
    global smartcarClient
    global oauthSessions
    
    if info.thingClassId == smartcarAccountThingClassId:
        logger.log("Starting pairing: ", smartcarAccountThingClassId)
        smartcarClient = createSmartcarClient()
        logger.log("Auth URL: ", smartcarClient.get_auth_url())
        oauthSessions[info.transactionId] = smartcarClient
        info.oAuthUrl = smartcarClient.get_auth_url()
        info.finish(nymea.ThingErrorNoError)
    else:
        logger.log("Unhandled pairing method")
        info.finish(nymea.ThingErrorCreationMethodNotSupported)

def confirmPairing(info, user, secret):
    logger.log("Confirm pairing...")
    if info.thingClassId == smartcarAccountThingClassId:                
        parsed = urlparse.urlparse(secret)
        code = parse_qs(parsed.query)['code'][0]        
        accessToken = smartcarClient.exchange_code(code)

        saveToken(info.thingId, accessToken)
        del oauthSessions[info.transactionId]
        info.finish(nymea.ThingErrorNoError)
    else:
        info.finish(nymea.ThingErrorCreationMethodNotSupported)

def setupThing(info):
    logger.log("Setting up thing:", info.thing.name)    

    # setup the account (login and discover vehicles)
    if info.thing.thingClassId == smartcarAccountThingClassId:
        logger.log("Setting up the account")
        # deal with the connection / auth
        try:
            client = createSmartcarClient()
            accessToken = getAccessToken(client, info.thing.id)
            info.finish(nymea.ThingErrorNoError)
        except Exception as e:
            logger.error("Error setting up smartcar account: ", str(e))
            info.finish(nymea.ThingErrorAuthenticationFailure, str(e))
            return

        info.thing.setStateValue(smartcarAccountLoggedInStateTypeId, True)
        info.thing.setStateValue(smartcarAccountConnectedStateTypeId, True)
        vehicle_ids = smartcar.get_vehicle_ids(accessToken)['vehicles']

        thingDescriptors = []
        for raw_vehicle in vehicle_ids:
            found = False
            vehicle = smartcar.Vehicle(raw_vehicle, accessToken)
            logger.log(vehicle.info())
            logger.log("-----------------")
            for thing in myThings():
                if thing.thingClassId == vehicleThingClassId and thing.paramValue(vehicleThingVehicleidParamTypeId) == vehicle.info()['id']:
                    logger.log("Vehicle already added: ", vehicle.info()['id'])
                    found = True
                    break
            if found:
                continue

            logger.log("Adding new vehicle to the system: ", vehicle.info()['id'], " parent id: ", info.thing.id)
            thingDescriptor = nymea.ThingDescriptor(vehicleThingClassId, vehicle.info()['model'], parentId=info.thing.id)
            thingDescriptor.params = [
                nymea.Param(vehicleThingVehicleidParamTypeId, vehicle.info()['id']),
                nymea.Param(vehicleThingMakeParamTypeId, vehicle.info()['make'])
            ]
            thingDescriptors.append(thingDescriptor)

        logger.log("New vehicles appeared")
        autoThingsAppeared(thingDescriptors)
        createPollTimer(info.thing)
        info.thing.settingChangedHandler = socRefreshRateChanged
        return
    
    # setup individual vehicles
    if info.thing.thingClassId == vehicleThingClassId:
        logger.log("Should setup vehicle here: ", info.thing.name)
        info.finish(nymea.ThingErrorNoError)


def postSetupThing(thing):
    logger.log("postSetupThing")

def thingRemoved(thing):
    logger.log("thingRemoved:", thing.name)

def createSmartcarClient():
    apiKey = apiKeyStorage().requestKey("smartcar")
    clientId = apiKey.data("clientId")    
    clientSecret = apiKey.data("clientSecret")
    redirectUri = "https://127.0.0.1:8888"
    
    testMode = configValue(smartcarPluginTestModeParamTypeId)
    logger.log("Test mode enabled: ", testMode)    
    
    smartcarClient = smartcar.AuthClient(
        client_id=clientId,
        client_secret=clientSecret,
        redirect_uri=redirectUri,
        scope=['required:read_vehicle_info', 'required:read_battery', 'required:read_charge'],
        test_mode=testMode
    )    
    return smartcarClient

    
def socRefreshRateChanged(thing, paramTypeId, value):
    if paramTypeId == smartcarAccountSettingsSocRefreshPeriodParamTypeId:
        logger.log("Refresh rate changed for ", thing.id, " , new value: ", value)        
        
        if (thing.id in pollTimers) and (pollTimers[thing.id] is not None):
            logger.log("Timer already exists, cancelling it first")
            pollTimers[thing.id].cancel()

        pollTimers[thing.id] = threading.Timer(value, pollService, [thing.id])
        pollTimers[thing.id].start()
    
def createPollTimer(thing):    
    timerId = thing.id
    if (timerId not in pollTimers) or (pollTimers[timerId] is None):
        pollTimers[timerId] = threading.Timer(5, pollService, [thing.id])
        pollTimers[timerId].start()

def pollService(parentThingId):
    socRefreshSettingValue = defaultPollTimerInterval
    for thing in myThings():
        if thing.parentId == parentThingId and thing.thingClassId == vehicleThingClassId:
            try:
                refreshVehicleSOC(createSmartcarClient(), thing)
            except:
                logger.error("Error refreshing vehicle SOC")
        if thing.id == parentThingId:
            socRefreshSettingValue = thing.setting(smartcarAccountSettingsSocRefreshPeriodParamTypeId)
        
    # pollTimerInterval can be modified in settings to refresh the SoC faster
    # when needed for a demo. When not needed, this value should be higher (e.g. 120 sec)
    # because the demo account has an API call quota whose limit can be easily reahed
    # when using high refresh rates.     
    pollTimerInterval = socRefreshSettingValue or defaultPollTimerInterval    
    pollTimers[parentThingId] = threading.Timer(pollTimerInterval, pollService, [parentThingId])
    pollTimers[parentThingId].start()

def refreshVehicleSOC(client, thing):    
    vid = thing.paramValue(vehicleThingVehicleidParamTypeId)
    logger.log("Refreshing SOC for vehicle: ", vid)    
    vehicle = smartcar.Vehicle(vid, getAccessToken(client, thing.parentId))
    
    # update the state of charge (battery level)
    soc = vehicle.battery()
    logger.log(soc)
    percentage = soc["data"]["percentRemaining"]    
    thing.setStateValue(vehicleBatteryLevelStateTypeId, percentage * 100)
    if percentage * 100 < 10:
        thing.setStateValue(vehicleBatteryCriticalStateTypeId, True)
    else:
        thing.setStateValue(vehicleBatteryCriticalStateTypeId, False)

    # update charing status
    chargingState = vehicle.charge()
    logger.log(chargingState)
    thing.setStateValue(vehiclePluggedInStateTypeId, chargingState["data"]["isPluggedIn"])
    thing.setStateValue(vehicleChargingStateTypeId, chargingState["data"]["state"] == "CHARGING")


def saveToken(thingId, token):    
    pluginStorage().beginGroup(thingId)
    pluginStorage().setValue("token", json.dumps(token, default=str))
    pluginStorage().endGroup()

def getAccessToken(client, thingId):    
    pluginStorage().beginGroup(thingId)
    token = json.loads(pluginStorage().value("token"))
    pluginStorage().endGroup()

    logger.log("Retrieved token from pluginStorage: ", token)    
    expiration_date_time_obj = datetime.datetime.strptime(token['expiration'], '%Y-%m-%d %H:%M:%S.%f')
    if smartcar.is_expired(expiration_date_time_obj):
        token = client.exchange_refresh_token(token['refresh_token'])
        saveToken(thingId, token)
    return token['access_token']


def configValueChanged(paramTypeId, value):    
    if paramTypeId == smartcarPluginTestModeParamTypeId:
        logger.log("Test mode enabled changed to: ", value)
        #should retrigger a setup somehow here
    