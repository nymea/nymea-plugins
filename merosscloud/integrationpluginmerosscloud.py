import nymea
import asyncio

from meross_iot.http_api import MerossHttpClient
from meross_iot.manager import MerossManager

loop = asyncio.get_event_loop()

managers = {} # <ThingId, manager>

pollTimer = None

# nymeas plugin api is already threaded but doesn't use asyncio.
# The meross_iot library however, only has an asyncio API.
# In order to await async coroutines we'll need to wrap them, run
# them in the asyncio event loop and wait for them to finish
def runBlocking(coro):
    global loop
    future = asyncio.run_coroutine_threadsafe(coro, loop)
    future.result()


def init():
    loop.run_forever()

def deinit():
    loop.close()


def startPairing(info):
    info.finish(nymea.ThingErrorNoError, "Log in to meross cloud")


def confirmPairing(info, username, secret):
    runBlocking(confirmPairingAsync(info, username, secret))

async def confirmPairingAsync(info, username, secret):
    try:
        await MerossHttpClient.async_from_user_password(email=username, password=secret)
        pluginStorage().beginGroup(info.thingId)
        pluginStorage().setValue("username", username)
        pluginStorage().setValue("secret", secret)
        pluginStorage().endGroup();
        logger.info("Logged in to meross cloud as %s" % username)
        info.finish(nymea.ThingErrorNoError)
    except:
        logger.warn("Error loggin in to meross cloud as %s" % username)
        info.finish(nymea.ThingErrorAuthenticationFailure)


def setupThing(info):
    if info.thing.thingClassId == accountThingClassId:
        runBlocking(setupAccount(info))
    elif info.thing.thingClassId == plugThingClassId:
        runBlocking(setupPlug(info))
    else:
        logger.warn("Unhandled thing class: %s", info.thing.thingClassId)

async def setupAccount(info):
    logger.debug("Setting up meross account")
    pluginStorage().beginGroup(info.thing.id)
    username = pluginStorage().value("username")
    secret = pluginStorage().value("secret")
    pluginStorage().endGroup();

    try:
        httpClient = await MerossHttpClient.async_from_user_password(email=username, password=secret)
        logger.info("Logged in to meross account as %s" % username)

        manager = MerossManager(http_client=httpClient)

        global managers;
        managers[info.thing.id] = manager

        info.finish(nymea.ThingErrorNoError)

        info.thing.setStateValue(accountLoggedInStateTypeId, True)
        info.thing.setStateValue(accountConnectedStateTypeId, True)

        await manager.async_device_discovery()
        devs = manager.find_devices()

        logger.log("found %s plugs" % len(devs))

        thingDescriptors = []

        for dev in devs:
            # Check if this plug is already added in nymea
            found = False
            for thing in myThings():
                logger.log("Comparing to existing device:", thing.name)
                if thing.thingClassId == plugThingClassId and thing.paramValue(plugThingIdParamTypeId) == dev.uuid:
                    logger.log("Already have this device in the system")
                    # Yep, already here... skip it
                    found = True
                    break
            if found:
                continue

            if dev.type == "mss310":
                logger.log("Adding new plug: %s" % dev.uuid)
                thingDescriptor = nymea.ThingDescriptor(plugThingClassId, dev.name, parentId=info.thing.id)
                thingDescriptor.params = [
                    nymea.Param(plugThingIdParamTypeId, dev.uuid)
                ]
                thingDescriptors.append(thingDescriptor)
            else:
                logger.warn("Unhandled meross device: %s" % dev)

        autoThingsAppeared(thingDescriptors)

    except:
        logger.warn("Login error")
        info.finish(nymea.ThingErrorAuthenticationFailure)

    global pollTimer
    if pollTimer is None:
        pollTimer = nymea.PluginTimer(5, pollService)
        logger.log("timer interval @ setupThing", pollTimer.interval)


async def setupPlug(info):
    logger.debug("Setting up meross plug")
    info.finish(nymea.ThingErrorNoError)


def executeAction(info):
    runBlocking(executeActionAsync(info))

async def executeActionAsync(info):
    manager = managers[info.thing.parentId]
    devs = manager.find_devices(device_uuids=(info.thing.paramValue(plugThingIdParamTypeId),))
    if len(devs) < 1:
        info.finish(nymea.ThingErrorHardwareNotAvailable)
        return

    plug = devs[0]
    if info.actionTypeId == plugPowerActionTypeId:
        power = info.paramValue(plugPowerActionPowerParamTypeId)
        if power == True:
            logger.log("Turning on")
            await plug.async_turn_on(channel=0)
            info.thing.setStateValue(plugPowerStateTypeId, True)
            info.finish(nymea.ThingErrorNoError)
        else:
            logger.log("Turning off")
            await plug.async_turn_off(channel=0)
            info.thing.setStateValue(plugPowerStateTypeId, False)
            info.finish(nymea.ThingErrorNoError)


def pollService():
    # Poll all robots we know
    for thing in myThings():
        if thing.thingClassId == plugThingClassId:
            runBlocking(refreshPlug(thing))


async def refreshPlug(thing):
    manager = managers[thing.parentId]
    devs = manager.find_devices(device_uuids=(thing.paramValue(plugThingIdParamTypeId),))
    if len(devs) < 1:
        info.finish(nymea.ThingErrorHardwareUnavailable)
        return

    plug = devs[0]
    await plug.async_update()
    thing.setStateValue(plugPowerStateTypeId, plug.is_on(channel=0))
    powerInfo = await plug.async_get_instant_metrics(channel=0)
    thing.setStateValue(plugCurrentPowerStateTypeId, powerInfo.power)

    totalEnergyConsumed = thing.stateValue(plugTotalEnergyConsumedStateTypeId)
    totalPowerInfo = await plug.async_get_daily_power_consumption(channel=0)

    logger.debug("energy: %s, total energy: %s" % (powerInfo, totalPowerInfo))

    # The lib only gives us the last 5 days consumption totals
    # In order to calculate an all time total we'll memorize that list
    # and update our total counter with changes to that list
    pluginStorage().beginGroup(thing.id)
    pluginStorage().beginGroup("consumptionLogs")

    timestamps = []
    for entry in totalPowerInfo:
        date = entry["date"]
        kwh = float(entry["total_consumption_kwh"])
        timestamps.append(str(date.timestamp()))
        loggedKwh = 0.0
        if pluginStorage().value(str(date.timestamp())) != None:
            loggedKwh = float(pluginStorage().value(str(date.timestamp())))

        if loggedKwh != kwh:
            totalEnergyConsumed -= loggedKwh
            totalEnergyConsumed += kwh
            pluginStorage().setValue(str(date.timestamp()), kwh)

    # Cleaning up old entries from pluginStorage
    for entry in pluginStorage().childKeys():
        if entry not in timestamps:
            pluginStorage().remove(entry)

    pluginStorage().endGroup()
    pluginStorage().endGroup()


    logger.log("currentPower: %s, calculated total energy consumed: %s" % (powerInfo.power, totalEnergyConsumed))
    thing.setStateValue(plugTotalEnergyConsumedStateTypeId, totalEnergyConsumed)
