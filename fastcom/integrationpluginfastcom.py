# SPDX-License-Identifier: GPL-3.0-or-later

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#
# Copyright (C) 2013 - 2024, nymea GmbH
# Copyright (C) 2024 - 2025, chargebyte austria GmbH
#
# This file is part of nymea-plugins.
#
# nymea-plugins is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# nymea-plugins is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

import nymea
from fastdotcom import fast_com
import threading

timers = {}

def init():
    logger.log("fast.com init")


def deinit():
    for timer in timers:
        timers[timer].cancel()


def setupThing(info):
    logger.log("setupThing", info.thing.name, "Scheduling speed test in 10 seconds.")
    info.thing.nameChangedHandler = nameChanged
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
    if info.actionTypeId == speedtestRunTestActionTypeId:
        if info.thing.stateValue(speedtestTestRunningStateTypeId) == True:
            info.finish(nymea.ThingErrorThingInUse, "A speed test is already running")
            return
        info.finish(nymea.ThingErrorNoError)
        runTest(info.thing, duration=info.paramValue(speedtestRunTestActionDurationParamTypeId))


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
