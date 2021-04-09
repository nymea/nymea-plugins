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
        except:
            # Login error
            info.finish(nymea.ThingErrorAuthenticationFailure, "Login error")
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
        logger.log("Creating polltimer")
        global pollTimer
        if pollTimer is not None:
            pollTimer = threading.Timer(5, pollService)
            pollTimer.start()

        return

    # Setup for the robots
    if info.thing.thingClassId == robotThingClassId:
        logger.log("SetupThing for robot:", info.thing.name)

        serial = info.thing.paramValue(robotThingSerialParamTypeId)
        secret = info.thing.paramValue(robotThingSecretParamTypeId)
        robot = Robot(serial, secret, info.thing.name)
        thingsAndRobots[info.thing] = robot;
        try:
            refreshRobot(info.thing)
        except:
            logger.warn("Error getting robot state");
            info.finish(nymea.ThingErrorHardwareFailure, "Unable to connect to neato API.")
            return;

        logger.log(robot.get_robot_state())
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
    if rbtStateDetails['isDocked'] == True:
        thing.setStateValue(robotStateStateTypeId, "docked")
    elif rbtPauseAv == True:
        thing.setStateValue(robotStateStateTypeId, "cleaning")
    elif rbtResumeAv == True:
        thing.setStateValue(robotStateStateTypeId, "paused")
    elif rbtStartAv == True:
        thing.setStateValue(robotStateStateTypeId, "stopped")
    else:
        thing.setStateValue(robotStateStateTypeId, "error")


def pollService():
    # Poll all robots we know
    for thing in myThings():
        if thing.thingClassId == robotThingClassId:
            try:
                refreshRobot(thing)
            except:
                logger.warn("Error refreshing robot state")

    # restart the timer for next poll
    global pollTimer
    pollTimer = threading.Timer(60, pollService)
    pollTimer.start()


def executeAction(info):
    return; # Temporary, to not accidentally start it

    if info.actionTypeId == robotStartCleaningActionTypeId:
        # To do: add a parameter to the start action which takes a zone id
        thingsAndRobots[info.thing].start_cleaning()
        refreshRobot(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return

    if info.actionTypeId == robotPauseCleaningActionTypeId:
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
        boundaries = robot.get_map_boundaries(mapId)

        logger.log("boundaries", type(boundaries), boundaries.json())
        for boundary in boundaries.json()["data"]["boundaries"]:
#            if boundary["type"] == "polygon":
            logger.log("vertices:", json.dumps(boundary["vertices"]))
            browseResult.addItem(nymea.BrowserItem(boundary["id"], boundary["name"], json.dumps(boundary), executable=True, disabled=not boundary["enabled"], icon=nymea.BrowserIconFavorites))

        browseResult.finish(nymea.ThingErrorNoError)
        return


def executeBrowserItem(info):
    # TODO: An item in the browser has been clicked.
    logger.log("Browser item clicked:", info.itemId)
    info.finish(nymea.ThingErrorNoError)


def deinit():
    global pollTimer
    # If we started a poll timer, cancel it on shutdown.
    if pollTimer is not None:
        pollTimer.cancel()
