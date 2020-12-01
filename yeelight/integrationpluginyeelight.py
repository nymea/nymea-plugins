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
from yeelight import discover_bulbs
from yeelight import Bulb
import threading

bulbs = {}

def init():
    logger.log("Yeelight init")


def deinit():
    logger.log("Yeelight deinit")


def discoverThings(info):
    logger.log("discoverThings", info.thingClassId, "Yeelight discovery")
    discover_bulbs()


def setupThing(info):
    logger.log("setupThing", info.thing.name, "Setting up bulb")
    info.thing.nameChangedHandler = nameChanged
    if info.thingClassId == colorBulbThingClassId:
        address = info.thing.paramValue(colorBulbThingHostParamTypeId)
        logger.log("Setting up color bulb with address", address)
        bulb = Bulb(address, effect="smooth", duration=1000)
        thread = threading.Thread(target=bulb.listen, args=(bulbCallback,))
        thread.start()
        bulbs[info.thing.id] = bulb
        info.finish(nymea.ThingErrorNoError)
    else if info.thingClassId == colorBulbThingClassId:
        address = info.thing.paramValue(dimmableBulbThingHostParamTypeId)
        logger.log("Setting up dimmable bulb with address", address)
        info.finish(nymea.ThingErrorNoError)
    else:
        logger.log("Setup thing: Thing class id not found", info.thingClassId)
        info.finish(nymea.ThingErrorThingClassNotFound)


def postSetupThing(thing):
    logger.log("postSetupThing", thing)


def nameChanged(thing):
    logger.log("Thing name changed:", thing.name)


def thingRemoved(thing):
    logger.log("thingRemoved:", thing.name)

    if thing.thingClassId == colorBulbThingClassId || info.thingClassId == dimmableBulbThingClassId:
        bulb = bulbs.get(thing.id)
        bulbs.remove(thing.id)
        bulb.stop_listening()


def executeAction(info):
    logger.log("executeAction")
    thing = info.thing
    action = info.action

    if thing.thingClassId == colorBulbThingClassId:
        bulb = bulbs.get(thing.id)
        if action.actionTypeId == colorBulbPowerActionTypeId:
            state = action.param(colorBulbPowerActionPowerParamTypeId).value
            if state:
                bulb.turn_on()
            else:
                bulb.turn_off()
        else if action.actionTypeId == colorBulbColorTemperatureActionTypeId:
            temperature = action.param(colorBulbColorTemperatureActionColorTemperatureParamTypeId).value
            bulb.set_color_temp(temperature)
        else if action.actionTypeId == colorBulbColorActionTypeId:
            color = action.param(colorBulbColorActionColorParamTypeId).value
            bulb.set_rgb(hex_color_to_rgb(color))
        else if action.actionTypeId == colorBulbBrightnessActionTypeId:
            bulb.set_brightness(10)
        else if action.actionTypeId == colorBulbEffectActionTypeId:
            bulb.turn_on(effect="sudden")
        else if action.actionTypeId == colorBulbColorModeActionTypeId:
            bulb.set_color_mode()
        else:
            logger.log("Action type Id not found", action.actionTypeId)
    else if thing.thingClassId == dimmableBulbThingClassId:
        bulb = bulbs.get(thing.id)
        if action.actionTypeId == dimmableBulbPowerActionTypeId:
        else if action.actionTypeId == colorBulbBrightnessActionTypeId:

        else:
            logger.log("Action type Id not found", action.actionTypeId)
    else:
        logger.log("Execute action, thingClass not found", thing.thingClassId)
        info.finish(nymea.ThingErrorThingClassNotAvailable)


def hex_color_to_rgb(color):
    color = color.strip("#")
    try:
        red, green, blue = tuple(int(color[i : i + 2], 16) for i in (0, 2, 4))
    except (TypeError, ValueError):
        logger.log("Unrecognized color, changing to red...", file=sys.stderr)
        red, green, blue = (255, 0, 0)
    return red, green, blue


def bulbCallback:
    logger.log("bulb callback)
