import nymea
import time
import threading
from pybotvac import Account, Neato, OAuthSession, PasswordlessSession, PasswordSession, Vorwerk, Robot
import json

# pybotvac library: https://github.com/stianaske/pybotvac

oauthSessions = {}
accountsMap = {}
thingsAndRobots = {}

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
        logger.log("SetupThing for account:", info.thing.name)

        pluginStorage().beginGroup(info.thing.id)
        token = json.loads(pluginStorage().value("token"))
        logger.log("setup", token)
        pluginStorage().endGroup();

        try:
            oAuthSession = OAuthSession(token=token)
            # Login went well, finish the setup
            info.finish(nymea.ThingErrorNoError)
        except Exception as e:
            # Login error
            logger.warn("Error setting up neato account:", str(e))
            info.finish(nymea.ThingErrorAuthenticationFailure, str(e))
            return

        # Mark the account as logged-in and connected
        info.thing.setStateValue(accountLoggedInStateTypeId, True)
        info.thing.setStateValue(accountConnectedStateTypeId, True)

        # Create an account session on the session to get info about the login
        account = Account(oAuthSession)
        accountsMap[info.thing] = account

        # List all robots associated with account
        logger.log("account created. Robots:", account.robots);

        thingDescriptors = []
        for robot in account.robots:
            logger.log("Found new robot:", robot.serial)
            # Check if this robot is already added in nymea
            found = False
            for thing in myThings():
                logger.log("Comparing to existing robot:", thing.name)
                if thing.thingClassId == robotThingClassId and thing.paramValue(robotThingSerialParamTypeId) == robot.serial:
                    logger.log("Already have this robot in the system")
                    # Yep, already here... skip it
                    found = True
                    break
            if found:
                continue

            logger.log("Adding new robot to the system with parent", info.thing.id)
            thingDescriptor = nymea.ThingDescriptor(robotThingClassId, robot.name, parentId=info.thing.id)
            thingDescriptor.params = [
                nymea.Param(robotThingSerialParamTypeId, robot.serial),
                nymea.Param(robotThingSecretParamTypeId, robot.secret)
            ]
            thingDescriptors.append(thingDescriptor)

        # And let nymea know about all the users robots
        autoThingsAppeared(thingDescriptors)

        # If no poll timer is set up yet, start it now
        logger.log("Creating polltimer @ setupThing")
        global pollTimer
        if pollTimer is None:
            pollTimer = nymea.PluginTimer(30, pollService)
            logger.log("timer interval @ setupThing", pollTimer.interval)
        return

    # Setup for the robots
    if info.thing.thingClassId == robotThingClassId:
        logger.log("SetupThing for robot:", info.thing.name)

        serial = info.thing.paramValue(robotThingSerialParamTypeId)
        secret = info.thing.paramValue(robotThingSecretParamTypeId)
        try:
            robot = Robot(serial, secret, info.thing.name)
            thingsAndRobots[info.thing] = robot;
            refreshRobot(info.thing)
        except Exception as e:
            logger.warn("Error setting up robot:", e)
            info.finish(nymea.ThingErrorHardwareFailure, str(e))
            return

        info.thing.setStateValue(robotConnectedStateTypeId, True)
        # set up polling for robot status
        info.finish(nymea.ThingErrorNoError)
        return

def refreshRobot(thing):
    robot = thingsAndRobots[thing]
    logger.log("Refreshing robot:", robot)

    # Get robot state
    rbtState = thingsAndRobots[thing].get_robot_state()
    rbtStateJson = rbtState.json()

    logger.log("Robot state for %s: %s" % (thing.name, rbtStateJson))

    # Set robot docked/charging state
    rbtStateDetails = rbtStateJson['details']
    thing.setStateValue(robotChargingStateTypeId, rbtStateDetails['isCharging'])
    thing.setStateValue(robotBatteryLevelStateTypeId, rbtStateDetails['charge'])

    # Set robot cleaning/paused state
    rbtStateCommands = rbtStateJson['availableCommands']
    rbtStartAv = rbtStateCommands['start']
    rbtPauseAv = rbtStateCommands['pause']
    rbtResumeAv = rbtStateCommands['resume']
    if rbtStateJson['error'] == None:
        rbtError = "no error"
    else:
        rbtError = rbtStateJson['error']
    # alert state hasn't been implemented yet (not sure what would trigger an alert, haven't seen one yet)
    if rbtStateJson['alert'] == None:
        rbtAlert = "no alert"
    else:
        rbtAlert = rbtStateJson['alert']
    logger.log("error: %s: -- alert: %s" % (rbtError, rbtAlert))
    if rbtStateDetails['isDocked'] == True:
        thing.setStateValue(robotRobotStateStateTypeId, "docked")
    elif rbtPauseAv == True:
        thing.setStateValue(robotRobotStateStateTypeId, "cleaning")
    elif rbtResumeAv == True:
        thing.setStateValue(robotRobotStateStateTypeId, "paused")
    elif rbtStartAv == True:
        thing.setStateValue(robotRobotStateStateTypeId, "stopped")
    else:
        thing.setStateValue(robotRobotStateStateTypeId, "error")
    thing.setStateValue(robotErrorMessageStateTypeId, rbtError)

def pollService():
    logger.log("pollTimer triggered")
    # Poll all robots we know
    for thing in myThings():
        if thing.thingClassId == robotThingClassId:
            try:
                refreshRobot(thing)
            except:
                logger.warn("Error refreshing robot state")

def executeAction(info):
    if info.actionTypeId == robotStartCleaningActionTypeId:
        refreshRobot(info.thing)
        if info.thing.stateValue(robotRobotStateStateTypeId) == "paused":
            thingsAndRobots[info.thing].resume_cleaning()
        else:
            cleanWithRobot(info.thing, None, None)

        refreshRobot(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return

    if info.actionTypeId == robotPauseCleaningActionTypeId:
        refreshRobot(info.thing)
        if info.thing.stateValue(robotRobotStateStateTypeId) == "paused":
            thingsAndRobots[info.thing].resume_cleaning()
        else:
            thingsAndRobots[info.thing].pause_cleaning()
        refreshRobot(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return

    if info.actionTypeId == robotReturnToBaseActionTypeId:
        thingsAndRobots[info.thing].send_to_base()
        refreshRobot(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return

    if info.actionTypeId == robotStopCleaningActionTypeId:
        thingsAndRobots[info.thing].stop_cleaning()
        refreshRobot(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return

def cleanWithRobot(robotThing, mapID, boundaryID):
    # To do: add a parameter to the start action which takes a zone id --> this should now be represented by mapID & boundaryID
    robot = thingsAndRobots[robotThing]
    logger.log("Cleaning with robot:", robot, robotThing)
    boolEco = robotThing.setting(robotSettingsEcoParamTypeId)
    boolCare = robotThing.setting(robotSettingsCareParamTypeId)
    boolNogo = robotThing.setting(robotSettingsNoGoLinesParamTypeId)
    if boolEco == False:
        intEco = 2
    else:
        intEco = 1
    if boolCare == False:
        intCare = 1
    else:
        intCare = 2
    if boolNogo == False:
        intNogo = 2
    else:
        intNogo = 4
    logger.log("Settings: Eco:", boolEco, "Care:", boolCare, "Enable nogo:", boolNogo, "mapID:", mapID, "boundaryID:", boundaryID)
    thingsAndRobots[robotThing].start_cleaning(mode=intEco, navigation_mode=intCare, category=intNogo, boundary_id=boundaryID, map_id=mapID)
    refreshRobot(robotThing)

def cleanWithRobot(robotThing, mapID, boundaryID):
    # To do: add a parameter to the start action which takes a zone id --> this should now be represented by mapID & boundaryID
    robot = thingsAndRobots[robotThing]
    logger.log("Cleaning with robot:", robot, robotThing, mapID, boundaryID)
    boolEco = robotThing.setting(robotSettingsEcoParamTypeId)
    boolCare = robotThing.setting(robotSettingsCareParamTypeId)
    boolNogo = robotThing.setting(robotSettingsNoGoLinesParamTypeId)
    if boolEco == False:
        intEco = 2
    else:
        intEco = 1
    if boolCare == False:
        intCare = 1
    else:
        intCare = 2
    if boolNogo == False:
        intNogo = 2
    else:
        intNogo = 4
    logger.log("Settings: Eco:", boolEco, "Care:", boolCare, "Enable nogo:", boolNogo, "mapID:", mapID, "boundaryID:", boundaryID)
    thingsAndRobots[robotThing].start_cleaning(mode=intEco, navigation_mode=intCare, category=intNogo, boundary_id=boundaryID, map_id=mapID)

def browseThing(browseResult):
    robotThing = browseResult.thing
    robot = thingsAndRobots[browseResult.thing]
    account = None
    for thing in myThings():
        logger.log("checking thing", thing.name, thing.id, robotThing.parentId)
        if thing.id == robotThing.parentId:
            account = accountsMap[thing]
            break
    if account is None:
        logger.warn("Cannot find account for robot", robotThing.name)
        browseResult.finish(nymea.ThingErrorAuthenticationFailure)
        return;

    # Top level entries -> return maps
    if browseResult.itemId == "" or browseResult.itemId == "maps":
        maps = account.persistent_maps

        logger.log("maps", type(maps), maps)
        for map in maps[robot.serial]:
            logger.log("Type mapInfo: ", type(map))
            logger.log("map:", map)
            browseResult.addItem(nymea.BrowserItem("map-" + map["id"], map["name"], browsable=True, thumbnail=map["url"]))

        browseResult.finish(nymea.ThingErrorNoError)
        return

    # browsing boundaries for a map
    if browseResult.itemId.startswith("map-"):
        mapId = browseResult.itemId[4:]

        try:
            boundaries = robot.get_map_boundaries(mapId)
        except Exception as e:
            logger.warn("Error fetching boundaries from robot:", e)
            info.finish(nymea.ThingErrorHardwareFailure, "Unable to fetch boundaries from robot.")

        logger.log("boundaries", type(boundaries), boundaries.json())
        for boundary in boundaries.json()["data"]["boundaries"]:
            if boundary["type"] == "polygon":
                logger.log("vertices:", boundary)
                browseResult.addItem(nymea.BrowserItem("boundary-" + mapId + ";" + boundary["id"], boundary["name"], json.dumps(boundary), executable=True, disabled=not boundary["enabled"], icon=nymea.BrowserIconFavorites))

        browseResult.finish(nymea.ThingErrorNoError)
        return

def executeBrowserItem(info):
    logger.log("Browser item clicked:", info.itemId)
    if info.itemId.startswith("boundary-"):
        ids = info.itemId[9:];
        logger.log("IDS:", ids)
        mapId = ids.split(";")[0]
        boundaryId = ids.split(";")[1]
        logger.log("Cleaning boundary:", mapId, boundaryId)
        cleanWithRobot(info.thing, mapId, boundaryId)
        refreshRobot(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return

    logger.warn("Can't execute browser item:", info.itemId)
    info.finish(nymea.ThingErrorItemNotExecutable)

def deinit():
    global pollTimer
    # If we started a poll timer, cancel it on shutdown.
    if pollTimer is not None:
        pollTimer = None

def thingRemoved(thing):
    global pollTimer
    logger.log("removeThing called for", thing.name)
    # Clean up all data related to this thing
    logger.log("len myThings", len(myThings()))
    if len(myThings()) == 0 and pollTimer is not None:
        logger.log("cancelling plugintimer")
        pollTimer = None
