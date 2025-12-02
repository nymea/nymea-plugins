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
import RPi.GPIO as GPIO

pluginTimer = None

def readadc(adcnum, clockpin, mosipin, misopin, cspin):
    if ((adcnum > 7) or (adcnum < 0)):
        return -1
    GPIO.output(cspin, True)

    GPIO.output(clockpin, False)  # start clock low
    GPIO.output(cspin, False)     # bring CS low

    commandout = adcnum
    commandout |= 0x18  # start bit + single-ended bit
    commandout <<= 3    # we only need to send 5 bits here
    for i in range(5):
        if (commandout & 0x80):
            GPIO.output(mosipin, True)
        else:
            GPIO.output(mosipin, False)
        commandout <<= 1
        GPIO.output(clockpin, True)
        GPIO.output(clockpin, False)

    adcout = 0
    # read in one empty bit, one null bit and 10 ADC bits
    for i in range(12):
        GPIO.output(clockpin, True)
        GPIO.output(clockpin, False)
        adcout <<= 1
        if (GPIO.input(misopin)):
            adcout |= 0x1

    GPIO.output(cspin, True)

    adcout >>= 1       # first bit is 'null' so drop it
    return adcout


def setupThing(info):
    logger.log("SetupThing for MCP3008:", info.thing.name)

    SPICLK = info.thing.paramValue(mcp3008ThingClkParamTypeId)
    SPIMISO = info.thing.paramValue(mcp3008ThingMisoParamTypeId)
    SPIMOSI = info.thing.paramValue(mcp3008ThingMosiParamTypeId)
    SPICS = info.thing.paramValue(mcp3008ThingCsParamTypeId)

    try:
        GPIO.setwarnings(False)
        GPIO.setmode(GPIO.BCM)    #to specify whilch pin numbering system
        # set up the SPI interface pins
        GPIO.setup(SPIMOSI, GPIO.OUT)
        GPIO.setup(SPIMISO, GPIO.IN)
        GPIO.setup(SPICLK, GPIO.OUT)
        GPIO.setup(SPICS, GPIO.OUT)

        logger.log("Reading channel 0")
        chan0Value = readadc(0, SPICLK, SPIMOSI, SPIMISO, SPICS)
        logger.log('Raw ADC Value: %s' % chan0Value)
        logger.log('ADC Voltage: %sV' % str("%.2f"%((chan0Value/1024.)*5)))

    except Exception as e:
        logger.warn("Unable to open SPI port", str(e))
        info.finish(nymea.ThingErrorHardwareFailure, "Unable to connect to the device. Please verify it is connected properly to the SPI interface.")
        return

    info.finish(nymea.ThingErrorNoError)

    global pluginTimer
    if pluginTimer is None:
        pluginTimer = nymea.PluginTimer(5, pollChannels)



def thingRemoved(thing):
    del devices[thing]
    if len(myThings()) == 0:
        GPIO.cleanup()



def pollChannels():
    for thing in myThings():        
        SPICLK = thing.paramValue(mcp3008ThingClkParamTypeId)
        SPIMISO = thing.paramValue(mcp3008ThingMisoParamTypeId)
        SPIMOSI = thing.paramValue(mcp3008ThingMosiParamTypeId)
        SPICS = thing.paramValue(mcp3008ThingCsParamTypeId)

        try:
            chan0Value = readadc(0, SPICLK, SPIMOSI, SPIMISO, SPICS)
            thing.setStateValue(mcp3008Channel1StateTypeId, str("%.2f"%((chan0Value/1024.)*5)))

            chan0Value = readadc(1, SPICLK, SPIMOSI, SPIMISO, SPICS)
            thing.setStateValue(mcp3008Channel2StateTypeId, str("%.2f"%((chan0Value/1024.)*5)))

            chan0Value = readadc(2, SPICLK, SPIMOSI, SPIMISO, SPICS)
            thing.setStateValue(mcp3008Channel3StateTypeId, str("%.2f"%((chan0Value/1024.)*5)))

            chan0Value = readadc(3, SPICLK, SPIMOSI, SPIMISO, SPICS)
            thing.setStateValue(mcp3008Channel4StateTypeId, str("%.2f"%((chan0Value/1024.)*5)))

            chan0Value = readadc(4, SPICLK, SPIMOSI, SPIMISO, SPICS)
            thing.setStateValue(mcp3008Channel5StateTypeId, str("%.2f"%((chan0Value/1024.)*5)))

            chan0Value = readadc(5, SPICLK, SPIMOSI, SPIMISO, SPICS)
            thing.setStateValue(mcp3008Channel6StateTypeId, str("%.2f"%((chan0Value/1024.)*5)))

            chan0Value = readadc(6, SPICLK, SPIMOSI, SPIMISO, SPICS)
            thing.setStateValue(mcp3008Channel7StateTypeId, str("%.2f"%((chan0Value/1024.)*5)))

            chan0Value = readadc(7, SPICLK, SPIMOSI, SPIMISO, SPICS)
            thing.setStateValue(mcp3008Channel8StateTypeId, str("%.2f"%((chan0Value/1024.)*5)))

        except:
            logger.warn("Failed to read values from", thing.name)
