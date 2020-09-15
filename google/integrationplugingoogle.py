# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
#
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
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

import nymea
import datetime
import pickle
import os.path
from googleapiclient.discovery import build
from google_auth_oauthlib.flow import InstalledAppFlow
from google.auth.transport.requests import Request

timers = {}

def init():
    logger.log("google init")


def deinit():
    for timer in timers:
        timers[timer].cancel()

def discoverThings(info):
    info.finish(nymea.ThingErrorNoError)

def setupThing(info):
    logger.log("setupThing", info.thing.name)
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
        info.finish(nymea.ThingErrorNoError)
        runTest(info.thing, duration=info.paramValue(speedtestRunTestActionDurationParamTypeId))
