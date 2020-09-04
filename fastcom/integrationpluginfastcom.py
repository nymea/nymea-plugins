# Copyright 2013 - 2020, nymea GmbH
# Contact: contact@nymea.io
#
# This file is part of nymea.
# This project including source code and documentation is protected by
# copyright law, and remains the property of nymea GmbH. All rights, including
# reproduction, publication, editing and translation, are reserved. The use of
# this project is subject to the terms of a license agreement to be concluded
# with nymea GmbH in accordance with the terms of use of nymea GmbH, available
# under https://nymea.io/license
#
# GNU Lesser General Public License Usage
# Alternatively, this project may be redistributed and/or modified under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; version 3. This project is distributed in the hope that
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this project. If not, see <https://www.gnu.org/licenses/>.
#
# For any further details and any questions please contact us under
# contact@nymea.io or see our FAQ/Licensing Information on
# https://nymea.io/license/faq

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
