import nymea
from yeelight import discover_bulbs
from yeelight import Bulb
import threading

timers = {}

def init():
    logger.log("Yeelight init")


def deinit():
    for timer in timers:
        timers[timer].cancel()

def discoverThings(info):
    logger.log("discoverThings", info.thingClassId, "Yeelight discovery test")

def setupThing(info):
    logger.log("setupThing", info.thing.name, "Setting up bulb")
    info.thing.nameChangedHandler = nameChanged
    if info.thingClassId == colorBulbThingClassId:
        bulb = Bulb("192.168.0.19", effect="smooth", duration=1000)

    timers[info.thing] = threading.Timer(10, runTestForever, [info.thing])
    timers[info.thing].start()
    info.finish(nymea.ThingErrorNoError)

def postSetupThing(thing):
    logger.log("postSetupThing", thing)


def nameChanged(thing):
    logger.log("Thing name changed:", thing.name)


def thingRemoved(thing):
    logger.log("thingRemoved:", thing.name)
    timers[thing].cancel()
    del timers[thing]


def executeAction(info):
    logger.log("executeAction")
    if info.actionTypeId == colorBulbPowerActionTypeId:
        info.finish(nymea.ThingErrorNoError)
        runTest(info.thing, duration=info.paramValue(speedtestRunTestActionDurationParamTypeId))
        bulb.turn_on()
        bulb.turn_off()


def runTest(thing, duration=10):
    logger.log("Running speed test for:", thing.name, "with maximum duration", duration)
    thing.setStateValue(speedtestTestRunningStateTypeId, True)
    result = fast_com(maxtime=duration, verbose=True)
    thing.setStateValue(speedtestLastResultStateTypeId, result)
    logger.log("Speed test result:", result)
    thing.setStateValue(speedtestTestRunningStateTypeId, False)
    param = nymea.Param(speedtestTestCompletedEventResultParamTypeId, result)
    params = [param]
    thing.emitEvent(speedtestTestCompletedEventTypeId, params)
    logger.log("all done")


def runTestForever(thing):
    del timers[thing]
    runTest(thing)
    logger.log("Running next test in %i minutes." % thing.settings[0].value)
    timers[thing] = threading.Timer(thing.settings[0].value * 60, runTestForever, [thing])
    timers[thing].start()
