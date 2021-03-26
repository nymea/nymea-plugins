import nymea
import time
import threading
from pybotvac import Account, Neato, OAuthSession, PasswordlessSession, PasswordSession, Vorwerk, Robot
import json

# pybotvac library: https://github.com/stianaske/pybotvac

thingsAndRobots = {}
oauthSessions = {}

pollTimer = None

def startPairing(info):
    # Start OAuth2 session
    apiKey = apiKeyStorage().requestKey("neato")
    oauthSession = OAuthSession(client_id=apiKey.data("clientId"), client_secret=apiKey.data("clientSecret"), redirect_uri="https://127.0.0.1:8888", vendor=Neato())
    oauthSessions[info.transactionId] = oauthSession;
    authorizationUrl = oauthSession.get_authorization_url()
    info.oAuthUrl = authorizationUrl
    info.finish(nymea.ThingErrorNoError)


def confirmPairing(info, username, secret):
    # The user has successfully logged in at neato. Obtain the token from the OAuth session
    token = oauthSessions[info.transactionId].fetch_token(secret)
    pluginStorage().beginGroup(info.thingId)
    pluginStorage().setValue("token", json.dumps(token))
    pluginStorage().endGroup();
    del oauthSessions[info.transactionId]
    info.finish(nymea.ThingErrorNoError)

def setupThing(info):
    # Setup for the account
    if info.thing.thingClassId == accountThingClassId:
        pluginStorage().beginGroup(info.thing.id)
        token = json.loads(pluginStorage().value("token"))
        logger.log("setup", token)
        pluginStorage().endGroup();

        try:
            oAuthSession = OAuthSession(token=token)
            # Login went well, finish the setup
            info.finish(nymea.ThingErrorNoError)
        except:
            # Login error
            info.finish(nymea.ThingErrorAuthenticationFailure, "Login error")
            return

        # Mark the account as logged-in and connected
        info.thing.setStateValue(accountLoggedInStateTypeId, True)
        info.thing.setStateValue(accountConnectedStateTypeId, True)

        # Create an account session on the session to get info about the login
        account = Account(oAuthSession)

        # List all robots associated with account
        logger.log("account created. Robots:", account.robots);

        logger.log("Persistent maps: ", account.persistent_maps)
        mapDict = account.persistent_maps
        mapKeys = mapDict.keys()
        logger.log("Keys: ", mapKeys)
        mapValues = mapDict.values()
        logger.log("Values: ", mapValues)

        thingDescriptors = []
        for robot in account.robots:
            logger.log(robot)
            # Check if this robot is already added in nymea
            found = False
            for thing in myThings():
                if thing.paramValue(robotThingSerialParamTypeId) == robot.serial:
                    # Yep, already here... skip it
                    found = True
                    continue
            if found:
                continue

            thingDescriptor = nymea.ThingDescriptor(robotThingClassId, robot.name)
            logger.log("MapID for Serial: ", robot.serial, mapDict[robot.serial])
            mapInfo = mapDict[robot.serial]
            logger.log("Type mapInfo: ", type(mapInfo))
            logger.log("Contents mapInfo: ", mapInfo)
            mapInfo2 = mapInfo[0]
            logger.log("MapInfo2 type: ", type(mapInfo2), " MapInfo2 contents: ", mapInfo2)
            mapId = mapInfo2['id']
            logger.log("MapId type: ", type(mapId), " MapId contents: ", mapId)
            mapName = mapInfo2['name']
            logger.log("MapName type: ", type(mapName), " MapName contents: ", mapName)
            rbtMapBound = robot.get_map_boundaries(mapId)
            logger.log("rbtMapBound Type: ", type(rbtMapBound), "rbtMapBound Contents: ", rbtMapBound)
            rbtBoundData = rbtMapBound.text
            logger.log("rbtBoundData Type: ", type(rbtBoundData), "rbtBoundData Contents: ", rbtBoundData)
            thingDescriptor.params = [
                nymea.Param(robotThingSerialParamTypeId, robot.serial),
                nymea.Param(robotThingSecretParamTypeId, robot.secret),
                nymea.Param(robotThingMapIdParamTypeId, mapId)
            ]
            thingDescriptors.append(thingDescriptor)

        # And let nymea know about all the users robots
        autoThingsAppeared(thingDescriptors)
        # return

        # If no poll timer is set up yet, start it now
        logger.log("Creating polltimer")
        global pollTimer
        pollTimer = threading.Timer(5, pollService)
        pollTimer.start()
        return


    # Setup for the robots
    if info.thing.thingClassId == robotThingClassId:

        serial = info.thing.paramValue(robotThingSerialParamTypeId)
        secret = info.thing.paramValue(robotThingSecretParamTypeId)
        robot = Robot(serial, secret, info.thing.name)
        thingsAndRobots[info.thing] = robot;
        logger.log(robot.get_robot_state())
        # set up polling for robot status
        info.finish(nymea.ThingErrorNoError)
        return



def pollService():
    logger.log("pollService!!!")

    # Poll all robots we know
    for thing in myThings():
        if thing.thingClassId == robotThingClassId:
            robot = thingsAndRobots[thing]
            logger.log("polling robot:", robot)

            # Get robot state
            rbtState = thingsAndRobots[thing].get_robot_state()
            rbtStateJson = rbtState.json()

            # Set robot docked/charging state
            rbtStateDetails = rbtStateJson['details']
            rbtCharging = rbtStateDetails['isCharging']
            rbtDocked = rbtStateDetails['isDocked']
            rbtStateOfCharge = rbtStateDetails['charge']
            logger.log("Updating thing", thing.name, "Charging", rbtCharging)
            thing.setStateValue(robotChargingStateTypeId, rbtCharging)
            logger.log("Updating thing", thing.name, "Docked", rbtDocked)
            thing.setStateValue(robotDockedStateTypeId, rbtDocked)
            logger.log("Updating thing", thing.name, "Battery Charge Level", rbtStateOfCharge)
            thing.setStateValue(robotBatteryLevelStateTypeId, rbtStateOfCharge)

            # Set robot cleaning/paused state
            rbtStateCommands = rbtStateJson['availableCommands']
            rbtStartAv = rbtStateCommands['start']
            rbtPauseAv = rbtStateCommands['pause']
            rbtResumeAv = rbtStateCommands['resume']
            if rbtStartAv == True:
                logger.log("Updating thing", thing.name, "Cleaning: False")
                thing.setStateValue(robotCleaningStateTypeId, False)
                thing.setStateValue(robotPausedStateTypeId, False)
            elif rbtPauseAv == True:
                logger.log("Updating thing", thing.name, "Cleaning: True")
                thing.setStateValue(robotCleaningStateTypeId, True)
                thing.setStateValue(robotPausedStateTypeId, False)
            elif rbtResumeAv == True:
                logger.log("Updating thing", thing.name, "Paused: True")
                thing.setStateValue(robotCleaningStateTypeId, True)
                thing.setStateValue(robotPausedStateTypeId, True)

    # restart the timer for next poll
    global pollTimer
    pollTimer = threading.Timer(60, pollService)
    pollTimer.start()


def executeAction(info):
    if info.actionTypeId == robotStartCleaningActionTypeId:
        rbtState = thingsAndRobots[info.thing].get_robot_state()
        rbtStateJson = rbtState.json()
        rbtStateCommands = rbtStateJson['availableCommands']
        rbtStartAv = rbtStateCommands['start']
        rbtPauseAv = rbtStateCommands['pause']
        rbtResumeAv = rbtStateCommands['resume']
        if rbtStartAv == True:
            logger.log("Start cleaning")
            thingsAndRobots[info.thing].start_cleaning()
        elif rbtPauseAv == True:
            logger.log("Pause cleaning")
            thingsAndRobots[info.thing].pause_cleaning()
        elif rbtResumeAv == True:
            thingsAndRobots[info.thing].resume_cleaning()
        threading.Timer(5, pollService).start()
        info.finish(nymea.ThingErrorNoError)
        return

    if info.actionTypeId == robotGoToBaseActionTypeId:
        thingsAndRobots[info.thing].send_to_base()
        threading.Timer(5, pollService).start()
        info.finish(nymea.ThingErrorNoError)

    if info.actionTypeId == robotStopCleaningActionTypeId:
        thingsAndRobots[info.thing].stop_cleaning()
        threading.Timer(5, pollService).start()
        info.finish(nymea.ThingErrorNoError)

    # To do: get available boundaries to use with start_cleaning action
    if info.actionTypeId == robotGetBoundariesActionTypeId:
        mapIDparam = info.thing.paramValue(robotThingMapIdParamTypeId)
        rbtMapBound = thingsAndRobots[info.thing].get_map_boundaries(mapIDparam)
        logger.log("rbtMapBound Type: ", type(rbtMapBound), "rbtMapBound Contents: ", rbtMapBound)
        rbtBoundData = rbtMapBound.text
        logger.log("rbtBoundData Type: ", type(rbtBoundData), "rbtBoundData Contents: ", rbtBoundData)
        threading.Timer(5, pollService).start()
        info.finish(nymea.ThingErrorNoError)


def deinit():
    global pollTimer
    # If we started a poll timer, cancel it on shutdown.
    if pollTimer is not None:
        pollTimer.cancel()
