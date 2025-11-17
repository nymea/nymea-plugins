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
import time
import threading
import json
import requests
import random
from xml.sax.saxutils import unescape
from zeroconfbrowser import ZeroconfDevice, ZeroconfListener, discover

pollTimer = None
pollFrequency = 30

def discoverThings(info):
    if info.thingClassId == receiverThingClassId:
        logger.log("Discovery started for", info.thingClassId)
        discoveredIps = findIps()
        
        for i in range(0, len(discoveredIps)):
            deviceIp = discoveredIps[i]
            rUrl = 'http://' + deviceIp + ':80/YamahaRemoteControl/ctrl'
            body = '<YAMAHA_AV cmd="GET"><System><Config>GetParam</Config></System></YAMAHA_AV>'
            headers = {'Content-Type': 'text/xml', 'Accept': '*/*'}
            rr = requests.post(rUrl, headers=headers, data=body)
            pollResponse = rr.text
            if rr.status_code == requests.codes.ok:
                logger.log("Device with IP " + deviceIp + " is a supported Yamaha AVR.")
                # get device info
                stringIndex1 = pollResponse.find("<System_ID>")
                stringIndex2 = pollResponse.find("</System_ID>")
                responseExtract = pollResponse[stringIndex1+11:stringIndex2]
                systemId = responseExtract
                logger.log("System ID:", systemId)
                stringIndex1 = pollResponse.find("<Model_Name>")
                stringIndex2 = pollResponse.find("</Model_Name>")
                responseExtract = pollResponse[stringIndex1+12:stringIndex2]
                modelType = "Yamaha " + responseExtract
                # check if device already known
                exists = False
                for thing in myThings():
                    logger.log("Comparing to existing receivers: is %s a receiver?" % (thing.name))
                    if thing.thingClassId == receiverThingClassId:
                        logger.log("Yes, %s is a receiver." % (thing.name))
                        if thing.paramValue(receiverThingSerialParamTypeId) == systemId:
                            logger.log("Already have receiver with serial number %s in the system: %s" % (systemId, thing.name))
                            exists = True
                        else:
                            logger.log("Thing %s doesn't match with found receiver with serial number %s" % (thing.name, systemId))
                if exists == False: # Receiver doesn't exist yet, so add it
                    thingDescriptor = nymea.ThingDescriptor(receiverThingClassId, modelType)
                    thingDescriptor.params = [
                        nymea.Param(receiverThingSerialParamTypeId, systemId)
                    ]
                    info.addDescriptor(thingDescriptor)
                else: # Receiver already exists, so show it to allow reconfiguration
                    thingDescriptor = nymea.ThingDescriptor(receiverThingClassId, modelType, thingId=thing.id)
                    thingDescriptor.params = [
                        nymea.Param(receiverThingSerialParamTypeId, systemId)
                    ]
                    info.addDescriptor(thingDescriptor)
            else:
                logger.log("Device with IP " + deviceIp + " does not appear to be a supported Yamaha AVR.")
        info.finish(nymea.ThingErrorNoError)
        return

    if info.thingClassId == zoneThingClassId:
        logger.log("Discovery started for", info.thingClassId)
        
        for possibleReceiver in myThings():
            logger.log("Looking for existing receivers to add zones: is %s a receiver?" % (possibleReceiver.name))
            if possibleReceiver.thingClassId == receiverThingClassId:
                receiver = possibleReceiver
                deviceIp = receiver.stateValue(receiverUrlStateTypeId)
                logger.log("Yes, %s with IP address %s is a receiver, looking for zones." % (receiver.name, deviceIp))
                rUrl = 'http://' + deviceIp + ':80/YamahaRemoteControl/ctrl'
                body = '<YAMAHA_AV cmd="GET"><System><Config>GetParam</Config></System></YAMAHA_AV>'
                headers = {'Content-Type': 'text/xml', 'Accept': '*/*'}
                rr = requests.post(rUrl, headers=headers, data=body)
                pollResponse = rr.text
                possibleZones = list(("Zone_2", "Zone_3", "Zone_4"))
                            
                for zone in possibleZones:
                    stringIndex1 = pollResponse.find("<" + zone + ">")
                    stringIndex2 = pollResponse.find("</" + zone + ">")
                    zoneFound = int(pollResponse[stringIndex1+8:stringIndex2])
                    zoneNbr = int(zone[5:6])
                    stringIndex1 = pollResponse.find("<System_ID>")
                    stringIndex2 = pollResponse.find("</System_ID>")
                    systemId = pollResponse[stringIndex1+11:stringIndex2]
                    if zoneFound == 1:
                        logger.log("Additional zone with number %s found." % (str(zoneNbr)))
                        # test if zone already exists
                        exists = False
                        for possibleZone in myThings():
                            logger.log("Comparing to existing zones: is %s a zone?" % (possibleZone.name))
                            if possibleZone.thingClassId == zoneThingClassId:
                                zone = possibleZone
                                logger.log("Yes, %s is a zone." % (possibleZone.name))
                                if zone.paramValue(zoneThingSerialParamTypeId) == systemId and zone.paramValue(zoneThingZoneIdParamTypeId) == zoneNbr:
                                    logger.log("Already have zone with number %s in the system" % (str(zoneNbr)))
                                    exists = True
                                else:
                                    logger.log("Thing %s doesn't match with found zone with number %s" % (possibleZone.name, str(zoneNbr)))
                            elif possibleZone.thingClassId == receiverThingClassId:
                                logger.log("Yes, %s is a main zone." % (possibleZone.name))
                            else:
                                logger.log("No, %s is not a zone." % (possibleZone.name))
                        if exists == False: # Zone doesn't exist yet, so add it
                            zoneName = receiver.name + " Zone " + str(zoneNbr)
                            logger.log("Found new additional zone:", zone, zoneNbr)
                            logger.log("Adding %s to the system with parent:" % (zoneName), receiver.name, receiver.id)
                            thingDescriptor = nymea.ThingDescriptor(zoneThingClassId, zoneName, parentId=receiver.id)
                            thingDescriptor.params = [
                                nymea.Param(zoneThingSerialParamTypeId, systemId),
                                nymea.Param(zoneThingZoneIdParamTypeId, zoneNbr)
                            ]
                            info.addDescriptor(thingDescriptor)
                        else: # Zone already exists, so show it to allow reconfiguration
                            zoneName = receiver.name + " Zone " + str(zoneNbr)
                            thingDescriptor = nymea.ThingDescriptor(zoneThingClassId, zoneName, thingId=zone.id, parentId=receiver.id)
                            thingDescriptor.params = [
                                nymea.Param(zoneThingSerialParamTypeId, systemId),
                                nymea.Param(zoneThingZoneIdParamTypeId, zoneNbr)
                            ]
                            info.addDescriptor(thingDescriptor)
        info.finish(nymea.ThingErrorNoError)
        return

def findIps():
    # To do: in future use nymea capabilities:
    # no need of any external libraries, you can just call "serviceBrowser = hardwareManager.zeroconf.registerServiceBrowser()"
    # and can then loop over "serviceBrowser.entries"# serviceBrowser = hardwareManager.zeroconf.registerServiceBrowser()
    # for i in range(0, len(serviceBrowser.entries)):
    #     logger.log(serviceBrowser.entries[i])
    
    # foreach (const ZeroConfServiceEntry &entry, m_serviceBrowser->serviceEntries()) {
    #     if (entry.hostAddress().protocol() == QAbstractSocket::IPv6Protocol && entry.hostAddress().toString().startsWith("fe80")) {
    #         // We don't support link-local ipv6 addresses yet. skip those entries
    #         continue;
    #     }
    #     QString uuid;
    #     foreach (const QString &txt, entry.txt()) {
    #         if (txt.startsWith("uuid")) {
    #             uuid = txt.split("=").last();
    #             break;
    #         }
    #     }
    #     if (QUuid(uuid) == kodiUuid) {
    #         ipString = entry.hostAddress().toString();
    #         port = entry.port();
    #         break;
    #     }
    # }
    # for now we use zeroconf (def discover & classes ZeroconfDevice & ZeroconfListener) as borrowed from pyvizio

    ipList = discover("_http._tcp.local.", 5)
    logger.log(ipList)

    discoveredIps = []
    for i in range(0, len(ipList)):
        deviceInfo = ipList[i]
        if "Yamaha" in deviceInfo.name:
            discoveredIps.append(deviceInfo.ip)
    return discoveredIps
    
def setupThing(info):
    if info.thing.thingClassId == receiverThingClassId:
        searchSystemId = info.thing.paramValue(receiverThingSerialParamTypeId)
        logger.log("setupThing called for", info.thing.name, searchSystemId)

        discoveredIps = findIps()
        found = False
        
        for i in range(0, len(discoveredIps)):
            deviceIp = discoveredIps[i]
            rUrl = 'http://' + deviceIp + ':80/YamahaRemoteControl/ctrl'
            body = '<YAMAHA_AV cmd="GET"><System><Config>GetParam</Config></System></YAMAHA_AV>'
            headers = {'Content-Type': 'text/xml', 'Accept': '*/*'}
            rr = requests.post(rUrl, headers=headers, data=body)
            pollResponse = rr.text
            if rr.status_code == requests.codes.ok:
                logger.log("Device with IP " + deviceIp + " is a supported Yamaha AVR.")
                # get device info
                stringIndex1 = pollResponse.find("<System_ID>")
                stringIndex2 = pollResponse.find("</System_ID>")
                responseExtract = pollResponse[stringIndex1+11:stringIndex2]
                systemId = responseExtract
                logger.log("System ID:", systemId)
                # check if this is the device with the serial number we're looking for
                if systemId == searchSystemId:
                    logger.log("Device with IP " + deviceIp + " is the existing device.")
                    found = True
                    info.thing.setStateValue(receiverUrlStateTypeId, deviceIp)
                    rr2 = rr
            else:
                logger.log("Device with IP " + deviceIp + " does not appear to be a supported Yamaha AVR.")
        if found == True:
            info.thing.setStateValue(receiverConnectedStateTypeId, True)
            pollReceiver(info.thing)
            info.finish(nymea.ThingErrorNoError)
        else:
            info.thing.setStateValue(receiverConnectedStateTypeId, False)
            info.finish(nymea.ThingErrorHardwareFailure, "Error connecting to the device in the network.")
        
        logger.log("Receiver added:", info.thing.name)

        # If no poll timer is set up yet, start it now
        logger.log("Creating pollService")
        global pollTimer
        global pollFrequency
        if pollTimer == None:
            logger.log("Starting timer @ setupThing")
            pollTimer = nymea.PluginTimer(pollFrequency, pollService)
            logger.log("timer interval @ setupThing", pollTimer.interval)
        else:
            logger.log("Timer already exists @ setupThing")
        
        info.finish(nymea.ThingErrorNoError)
        return

    # Setup for the zone
    if info.thing.thingClassId == zoneThingClassId:
        logger.log("SetupThing for zone:", info.thing.name)
        # get parent receiver thing, needed to get deviceIp
        for possibleParent in myThings():
            if possibleParent.id == info.thing.parentId:
                parentReceiver = possibleParent
        deviceIp = parentReceiver.stateValue(receiverUrlStateTypeId)
        zoneId = info.thing.paramValue(zoneThingZoneIdParamTypeId)
        zone = "Zone_" + str(zoneId)
        try:
            pollReceiver(info.thing)
            logger.log(zone + " added.")
            info.thing.setStateValue(zoneConnectedStateTypeId, True)
        except:
            logger.warn("Error getting zone state");
            info.finish(nymea.ThingErrorHardwareFailure, "Unable to set up zone.")
            info.thing.setStateValue(zoneConnectedStateTypeId, False)
            return;

        # set up polling for zone status
        info.finish(nymea.ThingErrorNoError)
        return

def pollReceiver(info):
    global pollFrequency
    if info.thingClassId == zoneThingClassId:
        # get parent receiver thing, needed to get deviceIp
        for possibleParent in myThings():
            if possibleParent.id == info.parentId:
                parentReceiver = possibleParent
        deviceIp = parentReceiver.stateValue(receiverUrlStateTypeId)
        zoneId = info.paramValue(zoneThingZoneIdParamTypeId)
        logger.log("polling zone", deviceIp, info.name)
        bodyStart = '<YAMAHA_AV cmd="GET"><Zone_' + str(zoneId) + '>'
        bodyEnd = '</Zone_' + str(zoneId) + '></YAMAHA_AV>'
    elif info.thingClassId == receiverThingClassId:
        deviceIp = info.stateValue(receiverUrlStateTypeId)
        logger.log("polling receiver", deviceIp, info.name)
        bodyStart = '<YAMAHA_AV cmd="GET"><Main_Zone>'
        bodyEnd = '</Main_Zone></YAMAHA_AV>'
    rUrl = 'http://' + deviceIp + ':80/YamahaRemoteControl/ctrl'
    body = bodyStart + '<Basic_Status>GetParam</Basic_Status>' + bodyEnd
    headers = {'Content-Type': 'text/xml', 'Accept': '*/*'}
    try:
        pr = requests.post(rUrl, headers=headers, data=body)
        polled = True
    except:
        logger.log("Device didn't respond to http request to get basic status", deviceIp, info.name)
        polled = False
        logger.log("Temporarily reducing pollfrequency to give the device some rest")
        pollFrequency = 120
    if polled == True:
        pollResponse = pr.text
        # add distinction between receiver & zone
        if info.thingClassId == receiverThingClassId:
            receiver = info
            if pr.status_code == requests.codes.ok:
                receiver.setStateValue(receiverConnectedStateTypeId, True)
                # Get power state
                if pollResponse.find("<Power>Standby</Power>") != -1:
                    receiver.setStateValue(receiverPowerStateTypeId, False)
                    powerState = False
                elif pollResponse.find("<Power>On</Power>") != -1:
                    receiver.setStateValue(receiverPowerStateTypeId, True)
                    powerState = True
                else:
                    logger.log("Power state not found!")
                # Get sleep state
                stringIndex1 = pollResponse.find("<Sleep>")
                stringIndex2 = pollResponse.find("</Sleep>")
                responseExtract = pollResponse[stringIndex1+7:stringIndex2]
                receiver.setStateValue(receiverSleepStateTypeId, responseExtract)
                # Get mute state
                if pollResponse.find("<Mute>Off</Mute>") != -1:
                    receiver.setStateValue(receiverMuteStateTypeId, False)
                elif pollResponse.find("<Mute>On</Mute>") != -1:
                    receiver.setStateValue(receiverMuteStateTypeId, True)
                else:
                    logger.log("Mute state not found!")
                # Get pure direct state
                if pollResponse.find("<Pure_Direct><Mode>Off</Mode></Pure_Direct>") != -1:
                    receiver.setStateValue(receiverPureDirectStateTypeId, False)
                elif pollResponse.find("<Pure_Direct><Mode>On</Mode></Pure_Direct>") != -1:
                    receiver.setStateValue(receiverPureDirectStateTypeId, True)
                else:
                    logger.log("Pure Direct state not found!")
                # Get enhancer state
                if pollResponse.find("<Enhancer>Off</Enhancer>") != -1:
                    receiver.setStateValue(receiverEnhancerStateTypeId, False)
                elif pollResponse.find("<Enhancer>On</Enhancer>") != -1:
                    receiver.setStateValue(receiverEnhancerStateTypeId, True)
                else:
                    logger.log("Enhancer state not found!")
                # Get input
                stringIndex1 = pollResponse.find("<Input><Input_Sel>")
                stringIndex2 = pollResponse.find("</Input_Sel>")
                inputSource = pollResponse[stringIndex1+18:stringIndex2]
                receiver.setStateValue(receiverInputSourceStateTypeId, inputSource)
                videoSources = ["HDMI1","HDMI2","HDMI3","HDMI4","HDMI5","AV1","AV2","AV3","AV4","AV5","AV6","V-AUX"]
                if inputSource in videoSources:
                    receiver.setStateValue(receiverPlayerTypeStateTypeId, "video")
                else:
                    receiver.setStateValue(receiverPlayerTypeStateTypeId, "audio")
                # Get sound program
                stringIndex1 = pollResponse.find("<Sound_Program>")
                stringIndex2 = pollResponse.find("</Sound_Program>")
                responseExtract = pollResponse[stringIndex1+15:stringIndex2]
                receiver.setStateValue(receiverSurroundModeStateTypeId, responseExtract)
                # Get Cinema DSP 3D state
                stringIndex1 = pollResponse.find("<_3D_Cinema_DSP>")
                stringIndex2 = pollResponse.find("</_3D_Cinema_DSP>")
                responseExtract = pollResponse[stringIndex1+16:stringIndex2]
                receiver.setStateValue(receiverCinemaDSP3DStateTypeId, responseExtract)
                # Get Adaptive DRC state
                stringIndex1 = pollResponse.find("<Adaptive_DRC>")
                stringIndex2 = pollResponse.find("</Adaptive_DRC>")
                responseExtract = pollResponse[stringIndex1+14:stringIndex2]
                receiver.setStateValue(receiverAdaptiveDRCStateTypeId, responseExtract)
                # Get volume - volume is represented by int in Yamaha API, but shown as double = int/10 in Yamaha UI - this is ignored here as nymea wants volume to be an int
                stringIndex1 = pollResponse.find("<Volume><Lvl><Val>")
                responseExtract = pollResponse[stringIndex1+18:stringIndex1+30]
                stringIndex2 = responseExtract.find("</Val>")
                responseExtract = responseExtract[0:stringIndex2]
                volume = int(responseExtract)
                receiver.setStateValue(receiverVolumeStateTypeId, volume)
                # Get bass
                stringIndex1 = pollResponse.find("<Bass><Val>")
                responseExtract = pollResponse[stringIndex1+11:stringIndex1+30]
                stringIndex2 = responseExtract.find("</Val>")
                responseExtract = responseExtract[0:stringIndex2]
                bass = int(responseExtract)
                receiver.setStateValue(receiverBassStateTypeId, bass)
                # Get treble
                stringIndex1 = pollResponse.find("<Treble><Val>")
                responseExtract = pollResponse[stringIndex1+13:stringIndex1+30]
                stringIndex2 = responseExtract.find("</Val>")
                responseExtract = responseExtract[0:stringIndex2]
                treble = int(responseExtract)
                receiver.setStateValue(receiverTrebleStateTypeId, treble)
                # Get dialogue level
                stringIndex1 = pollResponse.find("<Dialogue_Lvl>")
                stringIndex2 = pollResponse.find("</Dialogue_Lvl>")
                responseExtract = pollResponse[stringIndex1+14:stringIndex2]
                dialogueLvl = int(responseExtract)
                receiver.setStateValue(receiverDialogueLevelStateTypeId, dialogueLvl)
                # Get dialogue lift
                stringIndex1 = pollResponse.find("<Dialogue_Lift>")
                stringIndex2 = pollResponse.find("</Dialogue_Lift>")
                responseExtract = pollResponse[stringIndex1+15:stringIndex2]
                dialogueLift = int(responseExtract)
                receiver.setStateValue(receiverDialogueLiftStateTypeId, dialogueLift)
                # Get subwoofer trim
                stringIndex1 = pollResponse.find("<Subwoofer_Trim><Val>")
                responseExtract = pollResponse[stringIndex1+21:stringIndex1+30]
                stringIndex2 = responseExtract.find("</Val>")
                responseExtract = responseExtract[0:stringIndex2]
                subTrim = int(responseExtract)
                receiver.setStateValue(receiverSubwooferTrimStateTypeId, subTrim)
                # Get player info
                body = '<YAMAHA_AV cmd="GET"><' + inputSource + '><Play_Info>GetParam</Play_Info></' + inputSource + '></YAMAHA_AV>'
                headers = {'Content-Type': 'text/xml', 'Accept': '*/*'}
                try:
                    plr = requests.post(rUrl, headers=headers, data=body)
                    polled = True
                except:
                    logger.log("Device didn't respond to http request to get player status", deviceIp, info.name)
                    polled = False
                    logger.log("Temporarily reducing pollfrequency to give the device some rest")
                    pollFrequency = 120
                if polled == True:
                    if plr.status_code == requests.codes.ok and powerState == True:
                        playerResponse = plr.text
                        # Get repeat state
                        stringIndex1 = playerResponse.find("<Repeat>")
                        stringIndex2 = playerResponse.find("</Repeat>")
                        responseExtract = playerResponse[stringIndex1+8:stringIndex2]
                        if responseExtract not in ["None", "One", "All"]:
                            responseExtract = "None"
                        receiver.setStateValue(receiverRepeatStateTypeId, responseExtract)
                        # Get shuffle state
                        stringIndex1 = playerResponse.find("<Shuffle>")
                        stringIndex2 = playerResponse.find("</Shuffle>")
                        responseExtract = playerResponse[stringIndex1+9:stringIndex2]
                        if responseExtract == "On":
                            shuffleStatus = True
                        else:
                            shuffleStatus = False
                        receiver.setStateValue(receiverShuffleStateTypeId, shuffleStatus)
                        # Get playback state
                        stringIndex1 = playerResponse.find("<Playback_Info>")
                        stringIndex2 = playerResponse.find("</Playback_Info>")
                        responseExtract = playerResponse[stringIndex1+15:stringIndex2]
                        if responseExtract == "Play":
                            playStatus = "Playing"
                            pollFrequency = min(10, pollFrequency)
                        elif responseExtract == "Pause":
                            playStatus = "Paused"
                            pollFrequency = min(10, pollFrequency)
                        else:
                            playStatus = "Stopped"
                            pollFrequency = min(30, pollFrequency)
                        receiver.setStateValue(receiverPlaybackStatusStateTypeId, playStatus)
                        # Get meta info
                        stringIndex1 = playerResponse.find("<Artist>")
                        stringIndex2 = playerResponse.find("</Artist>")
                        responseExtract = playerResponse[stringIndex1+8:stringIndex2]
                        receiver.setStateValue(receiverArtistStateTypeId, unescape(responseExtract, {"&amp;": "&", "&apos;": "'", "&quot;": '"'}))
                        stringIndex1 = playerResponse.find("<Album>")
                        stringIndex2 = playerResponse.find("</Album>")
                        responseExtract = playerResponse[stringIndex1+7:stringIndex2]
                        receiver.setStateValue(receiverCollectionStateTypeId, unescape(responseExtract, {"&amp;": "&", "&apos;": "'", "&quot;": '"'}))
                        stringIndex1 = playerResponse.find("<Song>")
                        stringIndex2 = playerResponse.find("</Song>")
                        responseExtract = playerResponse[stringIndex1+6:stringIndex2]
                        receiver.setStateValue(receiverTitleStateTypeId, unescape(responseExtract, {"&amp;": "&", "&apos;": "'", "&quot;": '"'}))
                        # Get artwork --> Yamaha artwork file type isn't recognized by nymea: browse for external cover art?
                        stringIndex1 = playerResponse.find("<URL>")
                        stringIndex2 = playerResponse.find("</URL>")
                        responseExtract = playerResponse[stringIndex1+5:stringIndex2]
                        artURL = 'http://' + deviceIp + ':80' + responseExtract
                        receiver.setStateValue(receiverArtworkStateTypeId, artURL)
                    else:
                        # Playing from external source so no info available 
                        receiver.setStateValue(receiverRepeatStateTypeId, "None")
                        receiver.setStateValue(receiverShuffleStateTypeId, False)
                        receiver.setStateValue(receiverPlaybackStatusStateTypeId, "Stopped")
                        receiver.setStateValue(receiverArtistStateTypeId, "")
                        receiver.setStateValue(receiverCollectionStateTypeId, "")
                        receiver.setStateValue(receiverTitleStateTypeId, "")
                        receiver.setStateValue(receiverArtworkStateTypeId, "")
            else:
                receiver.setStateValue(receiverConnectedStateTypeId, False)
        elif info.thingClassId == zoneThingClassId:
            zone = info
            if pr.status_code == requests.codes.ok:
                zone.setStateValue(zoneConnectedStateTypeId, True)
                # Get power state
                if pollResponse.find("<Power>Standby</Power>") != -1:
                    zone.setStateValue(zonePowerStateTypeId, False)
                    powerState = False
                elif pollResponse.find("<Power>On</Power>") != -1:
                    zone.setStateValue(zonePowerStateTypeId, True)
                    powerState = True
                else:
                    logger.log("Power state not found!")
                # Get sleep state
                stringIndex1 = pollResponse.find("<Sleep>")
                stringIndex2 = pollResponse.find("</Sleep>")
                responseExtract = pollResponse[stringIndex1+7:stringIndex2]
                zone.setStateValue(zoneSleepStateTypeId, responseExtract)
                # Get mute state
                if pollResponse.find("<Mute>Off</Mute>") != -1:
                    zone.setStateValue(zoneMuteStateTypeId, False)
                elif pollResponse.find("<Mute>On</Mute>") != -1:
                    zone.setStateValue(zoneMuteStateTypeId, True)
                else:
                    logger.log("Mute state not found!")
                # Get input
                stringIndex1 = pollResponse.find("<Input><Input_Sel>")
                stringIndex2 = pollResponse.find("</Input_Sel>")
                inputSource = pollResponse[stringIndex1+18:stringIndex2]
                zone.setStateValue(zoneInputSourceStateTypeId, inputSource)
                videoSources = ["HDMI1","HDMI2","HDMI3","HDMI4","HDMI5","AV1","AV2","AV3","AV4","AV5","AV6","V-AUX"]
                if inputSource in videoSources:
                    zone.setStateValue(zonePlayerTypeStateTypeId, "video")
                else:
                    zone.setStateValue(zonePlayerTypeStateTypeId, "audio")
                # Get volume
                stringIndex1 = pollResponse.find("<Volume><Lvl><Val>")
                responseExtract = pollResponse[stringIndex1+18:stringIndex1+30]
                stringIndex2 = responseExtract.find("</Val>")
                responseExtract = responseExtract[0:stringIndex2]
                volume = int(responseExtract)
                zone.setStateValue(zoneVolumeStateTypeId, volume)
                # Get player info
                body = '<YAMAHA_AV cmd="GET"><' + inputSource + '><Play_Info>GetParam</Play_Info></' + inputSource + '></YAMAHA_AV>'
                headers = {'Content-Type': 'text/xml', 'Accept': '*/*'}
                try:
                    plr = requests.post(rUrl, headers=headers, data=body)
                    polled = True
                except:
                    logger.log("Device didn't respond to http request to get player status", deviceIp, info.name)
                    polled = False
                    logger.log("Temporarily reducing pollfrequency to give the device some rest")
                    pollFrequency = 120
                if polled == True:
                    if plr.status_code == requests.codes.ok and powerState == True:
                        playerResponse = plr.text
                        # Get repeat state
                        stringIndex1 = playerResponse.find("<Repeat>")
                        stringIndex2 = playerResponse.find("</Repeat>")
                        responseExtract = playerResponse[stringIndex1+8:stringIndex2]
                        if responseExtract not in ["None", "One", "All"]:
                            responseExtract = "None"
                        zone.setStateValue(zoneRepeatStateTypeId, responseExtract)
                        # Get shuffle state
                        stringIndex1 = playerResponse.find("<Shuffle>")
                        stringIndex2 = playerResponse.find("</Shuffle>")
                        responseExtract = playerResponse[stringIndex1+9:stringIndex2]
                        if responseExtract == "On":
                            shuffleStatus = True
                        else:
                            shuffleStatus = False
                        zone.setStateValue(zoneShuffleStateTypeId, shuffleStatus)
                        # Get playback state
                        stringIndex1 = playerResponse.find("<Playback_Info>")
                        stringIndex2 = playerResponse.find("</Playback_Info>")
                        responseExtract = playerResponse[stringIndex1+15:stringIndex2]
                        if responseExtract == "Play":
                            playStatus = "Playing"
                            pollFrequency = min(10, pollFrequency)
                        elif responseExtract == "Pause":
                            playStatus = "Paused"
                            pollFrequency = min(10, pollFrequency)
                        else:
                            playStatus = "Stopped"
                            pollFrequency = min(30, pollFrequency)
                        zone.setStateValue(zonePlaybackStatusStateTypeId, playStatus)
                        # Get meta info
                        stringIndex1 = playerResponse.find("<Artist>")
                        stringIndex2 = playerResponse.find("</Artist>")
                        responseExtract = playerResponse[stringIndex1+8:stringIndex2]
                        zone.setStateValue(zoneArtistStateTypeId, responseExtract)
                        stringIndex1 = playerResponse.find("<Album>")
                        stringIndex2 = playerResponse.find("</Album>")
                        responseExtract = playerResponse[stringIndex1+7:stringIndex2]
                        zone.setStateValue(zoneCollectionStateTypeId, responseExtract)
                        stringIndex1 = playerResponse.find("<Song>")
                        stringIndex2 = playerResponse.find("</Song>")
                        responseExtract = playerResponse[stringIndex1+6:stringIndex2]
                        zone.setStateValue(zoneTitleStateTypeId, responseExtract)
                        stringIndex1 = playerResponse.find("<URL>")
                        stringIndex2 = playerResponse.find("</URL>")
                        responseExtract = playerResponse[stringIndex1+5:stringIndex2]
                        artURL = 'http://' + deviceIp + ':80' + responseExtract
                        zone.setStateValue(zoneArtworkStateTypeId, artURL)
                    else:
                        # Playing from external source so no info available 
                        zone.setStateValue(zoneRepeatStateTypeId, "None")
                        zone.setStateValue(zoneShuffleStateTypeId, False)
                        zone.setStateValue(zonePlaybackStatusStateTypeId, "Stopped")
                        zone.setStateValue(zoneArtistStateTypeId, "")
                        zone.setStateValue(zoneCollectionStateTypeId, "")
                        zone.setStateValue(zoneTitleStateTypeId, "")
                        zone.setStateValue(zoneArtworkStateTypeId, "")
            else:
                zone.setStateValue(zoneConnectedStateTypeId, False)

def pollService():
    logger.log("pollTimer triggered")
    global pollTimer
    global pollFrequency
    logger.log("adjusting timer interval")
    pollTimer.interval = pollFrequency
    logger.log("timer interval @ pollService", pollTimer.interval)
    pollFrequency = 30
    # Poll all receivers we know
    for thing in myThings():
        if thing.thingClassId == receiverThingClassId or thing.thingClassId == zoneThingClassId:
            pollReceiver(thing)

def executeAction(info):
    pollReceiver(info.thing)
    if info.thing.thingClassId == zoneThingClassId:
        # get parent receiver thing, needed to get deviceIp
        for possibleParent in myThings():
            if possibleParent.id == info.thing.parentId:
                parentReceiver = possibleParent
        deviceIp = parentReceiver.stateValue(receiverUrlStateTypeId)
        zoneId = info.thing.paramValue(zoneThingZoneIdParamTypeId)
        bodyStart = '<YAMAHA_AV cmd="PUT"><Zone_' + str(zoneId) + '>'
        bodyEnd = '</Zone_' + str(zoneId) + '></YAMAHA_AV>'
        source = info.thing.stateValue(zoneInputSourceStateTypeId)
        powerCheck = info.thing.stateValue(zonePowerStateTypeId)
    elif info.thing.thingClassId == receiverThingClassId:
        deviceIp = info.thing.stateValue(receiverUrlStateTypeId)
        bodyStart = '<YAMAHA_AV cmd="PUT"><Main_Zone>'
        bodyEnd = '</Main_Zone></YAMAHA_AV>'
        source = info.thing.stateValue(receiverInputSourceStateTypeId)
        powerCheck = info.thing.stateValue(receiverPowerStateTypeId)

    logger.log("executeAction called for thing", info.thing.name, deviceIp, source, info.actionTypeId, info.params)
    rUrl = 'http://' + deviceIp + ':80/YamahaRemoteControl/ctrl'
    headers = {'Content-Type': 'text/xml', 'Accept': '*/*'}

    # turn receiver/zone on if needed, before executing the action
    if powerCheck == False and info.actionTypeId != receiverPowerActionTypeId and info.actionTypeId != zonePowerActionTypeId:
        body = bodyStart + '<Power_Control><Power>On</Power></Power_Control>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)

    if info.actionTypeId == receiverIncreaseVolumeActionTypeId or info.actionTypeId == zoneIncreaseVolumeActionTypeId:
        if info.actionTypeId == receiverIncreaseVolumeActionTypeId:
            stepsize = info.paramValue(receiverIncreaseVolumeActionStepParamTypeId)
        else:
            stepsize = info.paramValue(zoneIncreaseVolumeActionStepParamTypeId)
        volumeDelta = stepsize * 10
        while abs(volumeDelta) >= 5:
            if volumeDelta >= 50:
                step = "Up 5 dB"
                volumeDelta -= 50
            elif volumeDelta >= 10:
                step = "Up 1 dB"
                volumeDelta -= 10
            elif volumeDelta >= 5:
                step = "Up"
                volumeDelta -= 5
            else:
                break
            body = bodyStart + '<Volume><Lvl><Val>' + step + '</Val><Exp></Exp><Unit></Unit></Lvl></Volume>' + bodyEnd
            pr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverDecreaseVolumeActionTypeId or info.actionTypeId == zoneDecreaseVolumeActionTypeId:
        if info.actionTypeId == receiverDecreaseVolumeActionTypeId:
            stepsize = info.paramValue(receiverDecreaseVolumeActionStepParamTypeId)
        else:
            stepsize = info.paramValue(zoneDecreaseVolumeActionStepParamTypeId)
        volumeDelta = stepsize * -10
        while abs(volumeDelta) >= 5:
            if volumeDelta <= -50:
                step = "Down 5 dB"
                volumeDelta += 50
            elif volumeDelta <= -10:
                step = "Down 1 dB"
                volumeDelta += 10
            elif volumeDelta <= -5:
                step = "Down"
                volumeDelta += 5
            else:
                break
            body = bodyStart + '<Volume><Lvl><Val>' + step + '</Val><Exp></Exp><Unit></Unit></Lvl></Volume>' + bodyEnd
            pr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverSkipBackActionTypeId or info.actionTypeId == zoneSkipBackActionTypeId:
        body = '<YAMAHA_AV cmd="PUT"><' + source + '><Play_Control><Playback>Skip Rev</Playback></Play_Control></' + source + '></YAMAHA_AV>'
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        # AirPlay statusupdates appear to take a while longer to be available in API
        if source == "AirPlay":
            time.sleep(6)
            pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverStopActionTypeId or info.actionTypeId == zoneStopActionTypeId:
        body = '<YAMAHA_AV cmd="PUT"><' + source + '><Play_Control><Playback>Stop</Playback></Play_Control></' + source + '></YAMAHA_AV>'
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        if source == "AirPlay":
            time.sleep(6)
            pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverPlayActionTypeId or info.actionTypeId == zonePlayActionTypeId:
        body = '<YAMAHA_AV cmd="PUT"><' + source + '><Play_Control><Playback>Play</Playback></Play_Control></' + source + '></YAMAHA_AV>'
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        if source == "AirPlay":
            time.sleep(6)
            pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverPauseActionTypeId or info.actionTypeId == zonePauseActionTypeId:
        body = '<YAMAHA_AV cmd="PUT"><' + source + '><Play_Control><Playback>Pause</Playback></Play_Control></' + source + '></YAMAHA_AV>'
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        if source == "AirPlay":
            time.sleep(6)
            pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverSkipNextActionTypeId or info.actionTypeId == zoneSkipNextActionTypeId:
        body = '<YAMAHA_AV cmd="PUT"><' + source + '><Play_Control><Playback>Skip Fwd</Playback></Play_Control></' + source + '></YAMAHA_AV>'
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        if source == "AirPlay":
            time.sleep(6)
            pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverPowerActionTypeId or info.actionTypeId == zonePowerActionTypeId:
        if info.actionTypeId == receiverPowerActionTypeId:
            power = info.paramValue(receiverPowerActionPowerParamTypeId)
        else:
            power = info.paramValue(zonePowerActionPowerParamTypeId)
        if power == True:
            powerString = "On"
        else:
            powerString = "Standby"
        body = bodyStart + '<Power_Control><Power>' + powerString + '</Power></Power_Control>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverSleepActionTypeId or info.actionTypeId == zoneSleepActionTypeId:
        if info.actionTypeId == receiverSleepActionTypeId:
            sleepString = info.paramValue(receiverSleepActionSleepParamTypeId)
        else:
            sleepString = info.paramValue(zoneSleepActionSleepParamTypeId)
        body = bodyStart + '<Power_Control><Sleep>' + sleepString + '</Sleep></Power_Control>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverMuteActionTypeId or info.actionTypeId == zoneMuteActionTypeId:
        if info.actionTypeId == receiverMuteActionTypeId:
            mute = info.paramValue(receiverMuteActionMuteParamTypeId)
        else:
            mute = info.paramValue(zoneMuteActionMuteParamTypeId)
        if mute == True:
            muteString = "On"
        else:
            muteString = "Off"
        body = bodyStart + '<Volume><Mute>' + muteString + '</Mute></Volume>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverVolumeActionTypeId or info.actionTypeId == zoneVolumeActionTypeId:
        if info.actionTypeId == receiverVolumeActionTypeId:
            newVolume = info.paramValue(receiverVolumeActionVolumeParamTypeId)
        else:
            newVolume = info.paramValue(zoneVolumeActionVolumeParamTypeId)
        # volume needs to be multiple of 5
        remainder = newVolume % 5
        newVolume -= remainder
        volumeString = str(newVolume)
        logger.log("Volume set to", newVolume)
        body = bodyStart + '<Volume><Lvl><Val>' + volumeString + '</Val><Exp>1</Exp><Unit>dB</Unit></Lvl></Volume>' + bodyEnd
        pr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverSubwooferTrimActionTypeId:
        newTrim = info.paramValue(receiverSubwooferTrimActionSubwooferTrimParamTypeId)
        # trim needs to be multiple of 5
        remainder = newTrim % 5
        newTrim -= remainder
        trimString = str(newTrim)
        logger.log("Subwoofer trim set to", newTrim)
        body = bodyStart + '<Volume><Subwoofer_Trim><Val>' + trimString + '</Val><Exp>1</Exp><Unit>dB</Unit></Subwoofer_Trim></Volume>' + bodyEnd
        pr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverPureDirectActionTypeId:
        pureDirect = info.paramValue(receiverPureDirectActionPureDirectParamTypeId)
        if pureDirect == True:
            PureDirectString = "On"
        else:
            PureDirectString = "Off"
        body = bodyStart + '<Sound_Video><Pure_Direct><Mode>' + PureDirectString + '</Mode></Pure_Direct></Sound_Video>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverEnhancerActionTypeId:
        enhancer = info.paramValue(receiverEnhancerActionEnhancerParamTypeId)
        if enhancer == True:
            enhancerString = "On"
        else:
            enhancerString = "Off"
        body = bodyStart + '<Surround><Program_Sel><Current><Enhancer>' + enhancerString + '</Enhancer></Current></Program_Sel></Surround>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverDialogueLevelActionTypeId:
        diaLvl = info.paramValue(receiverDialogueLevelActionDialogueLevelParamTypeId)
        diaStr = str(diaLvl)
        body = bodyStart + '<Sound_Video><Dialogue_Adjust><Dialogue_Lvl>' + diaStr + '</Dialogue_Lvl></Dialogue_Adjust></Sound_Video>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverDialogueLiftActionTypeId:
        diaLift = info.paramValue(receiverDialogueLiftActionDialogueLiftParamTypeId)
        diaStr = str(diaLift)
        body = bodyStart + '<Sound_Video><Dialogue_Adjust><Dialogue_Lift>' + diaStr + '</Dialogue_Lift></Dialogue_Adjust></Sound_Video>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverBassActionTypeId:
        bass = info.paramValue(receiverBassActionBassParamTypeId)
        # bass needs to be multiple of 5
        remainder = bass % 5
        bass -= remainder
        bassStr = str(bass)
        logger.log("Bass set to", bassStr)
        body = bodyStart + '<Sound_Video><Tone><Bass><Val>' + bassStr + '</Val><Exp>1</Exp><Unit>dB</Unit></Bass></Tone></Sound_Video>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverTrebleActionTypeId:
        treble = info.paramValue(receiverTrebleActionTrebleParamTypeId)
        # treble needs to be multiple of 5
        remainder = treble % 5
        treble -= remainder
        trebleStr = str(treble)
        logger.log("Treble set to", trebleStr)
        body = bodyStart + '<Sound_Video><Tone><Treble><Val>' + trebleStr + '</Val><Exp>1</Exp><Unit>dB</Unit></Treble></Tone></Sound_Video>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverCinemaDSP3DActionTypeId:
        dsp3D = info.paramValue(receiverCinemaDSP3DActionCinemaDSP3DParamTypeId)
        logger.log("Cinema DSP 3D set to", dsp3D)
        body = bodyStart + '<Surround><_3D_Cinema_DSP>' + dsp3D + '</_3D_Cinema_DSP></Surround>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverAdaptiveDRCActionTypeId:
        adrc = info.paramValue(receiverAdaptiveDRCActionAdaptiveDRCParamTypeId)
        logger.log("Adaptive DRC set to", adrc)
        body = bodyStart + '<Sound_Video><Adaptive_DRC>' + adrc + '</Adaptive_DRC></Sound_Video>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverInputSourceActionTypeId or info.actionTypeId == zoneInputSourceActionTypeId:
        if info.actionTypeId == receiverInputSourceActionTypeId:
            inputSource = info.paramValue(receiverInputSourceActionInputSourceParamTypeId)
        else:
            inputSource = info.paramValue(zoneInputSourceActionInputSourceParamTypeId)
        logger.log("Input Source changed to", inputSource)
        body = bodyStart + '<Input><Input_Sel>' + inputSource + '</Input_Sel></Input>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverSurroundModeActionTypeId:
        surroundMode = info.paramValue(receiverSurroundModeActionSurroundModeParamTypeId)
        logger.log("Surround Mode changed to", surroundMode)
        if surroundMode != "Straight":
            body = bodyStart + '<Surround><Program_Sel><Current><Sound_Program>' + surroundMode + '</Sound_Program></Current></Program_Sel></Surround>' + bodyEnd
        else:
            body = bodyStart + '<Surround><Program_Sel><Current><Straight>On</Straight></Current></Program_Sel></Surround>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverShuffleActionTypeId or info.actionTypeId == zoneShuffleActionTypeId:
        if info.actionTypeId == receiverShuffleActionTypeId:
            shuffle = info.paramValue(receiverShuffleActionShuffleParamTypeId)
        else:
            shuffle = info.paramValue(zoneShuffleActionShuffleParamTypeId)
        if shuffle == True:
            shuffleString = "On"
        else:
            shuffleString = "Off"
        body = '<YAMAHA_AV cmd="PUT"><' + source + '><Play_Control><Play_Mode><Shuffle>' + shuffleString + '</Shuffle></Play_Mode></Play_Control></' + source + '></YAMAHA_AV>'
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverRepeatActionTypeId or info.actionTypeId == zoneRepeatActionTypeId:
        if info.actionTypeId == receiverRepeatActionTypeId:
            repeat = info.paramValue(receiverRepeatActionRepeatParamTypeId)
        else:
            repeat = info.paramValue(zoneRepeatActionRepeatParamTypeId)
        logger.log("Repeat mode:", repeat)
        if repeat == "All":
            repeatString = "All"
        elif repeat == "One":
            repeatString = "One"
        else:
            repeatString = "Off"
        body = '<YAMAHA_AV cmd="PUT"><' + source + '><Play_Control><Play_Mode><Repeat>' + repeatString + '</Repeat></Play_Mode></Play_Control></' + source + '></YAMAHA_AV>'
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
        return
    elif info.actionTypeId == receiverRandomAlbumActionTypeId or info.actionTypeId == zoneRandomAlbumActionTypeId:
        body = bodyStart + '<Input><Input_Sel>SERVER</Input_Sel></Input>' + bodyEnd
        rr = requests.post(rUrl, headers=headers, data=body)
        time.sleep(0.5)
        playRandomAlbum(rUrl, "SERVER")
        time.sleep(0.5)
        pollReceiver(info.thing)
        info.finish(nymea.ThingErrorNoError)
    else:
        logger.log("Action not yet implemented for thing")
        info.finish(nymea.ThingErrorNoError)
        return

def playRandomAlbum(rUrl, source):
    # currently source needs to be SERVER
    # To do: add code to filter out unselectable items
    if source == "SERVER":
        browseTree = ["Random", "Music", "By Album", "Random"]
        logger.log("Playing random album on source " + source)
    else:
        browseTree = []
        logger.log("Source not supported for this action")
    # navigate browseTree (first item select random server, then folder "Music", ...)
    menuLayer = browseInTree(rUrl, source, browseTree)

    # don't do anything unless browsing to the required menu item succeeded:
    if menuLayer == len(browseTree)+1 and menuLayer > 0: 
        # play album by selecting first "selectable" line (attribute = "Item")
        browseResponse, menuLayer = browseMenuReady(rUrl, source)
        selectable = False
        line = 1
        while selectable == False:
            itemTxt, itemAttr = readLine(browseResponse, line)
            if itemAttr == "Item":
                selectable = True
            else:
                line += 1
        logger.log("Selecting line %s with label %s" % (line, itemTxt))
        selectLine(rUrl, source, line)
    return

def browseInTree(rUrl, source, browseTree):
    menuLayer = 1
    if browseTree == None:
        #create empty tree
        browseTree = []
    # go up to the main menu level if needed
    if len(browseTree) > 0:
        selLayer = 1
        browseResponse, menuLayer = browseMenuReady(rUrl, source)
        while menuLayer > selLayer:
            menuLevelUp(rUrl, source)
            browseResponse, menuLayer = browseMenuReady(rUrl, source)
    # navigate browseTree
    for i in range (0, len(browseTree)):
        if browseTree[i] == "Random":
            currentLine, maxLine = getLineNbrs(browseResponse)
            selItem = random.randint(1, maxLine)
            selectLine(rUrl, source, selItem)
        else:
            selItem = findLine(rUrl, source, browseTree[i])
            if selItem > 0:
                selectLine(rUrl, source, selItem)
            else:
                logger.log("Requested item not found")
        # set menuLayer in case of error in browsing?
        browseResponse, menuLayer = browseMenuReady(rUrl, source)
        logger.log("Returning menuLayer", menuLayer)
    if menuLayer < len(browseTree)+1:
        logger.log("Attention, this isn't the requested menuLayer!")
    return menuLayer

def findLine(rUrl, source, searchTxt):
    # browse menu level: keep going through menu pages (of 8 items per page) until lineTxt is found
    loop = True
    selItem = 0
    gotoLine(rUrl, source, 1)
    while loop == True:
        browseResponse, menuLayer = browseMenuReady(rUrl, source)
        currentLine, maxLine = getLineNbrs(browseResponse)
        # read the 8 lines in the current browseResponse page
        for i in range(1, 9):
            itemTxt, itemAttr = readLine(browseResponse, i)
            if itemTxt == searchTxt:
                selItem = currentLine + i - 1
                loop = False
        if maxLine > currentLine + 7 and loop == True:
            # end of list not yet reached, go to next page
            pageDown(rUrl, source)
        else:
            # last page, stop loop
            loop = False
    return selItem

def browseThing(browseResult):
    # To do: add browse menu action "create shortcut here" as soon as nymea allows browse menu actions?
    # To do: limit browsing to sources that allow it?
    zoneOrReceiver = browseResult.thing
    pollReceiver(zoneOrReceiver)
    if zoneOrReceiver.thingClassId == zoneThingClassId:
        # get parent receiver thing, needed to get deviceIp
        for possibleParent in myThings():
            if possibleParent.id == zoneOrReceiver.parentId:
                parentReceiver = possibleParent
        zoneId = zoneOrReceiver.paramValue(zoneThingZoneIdParamTypeId)
        deviceIp = parentReceiver.stateValue(receiverUrlStateTypeId)
        bodyStart = '<YAMAHA_AV cmd="PUT"><Zone_' + str(zoneId) + '>'
        bodyEnd = '</Zone_' + str(zoneId) + '></YAMAHA_AV>'
        source = zoneOrReceiver.stateValue(zoneInputSourceStateTypeId)
        playRandomId = zonePlayRandomBrowserItemActionTypeId
        powerCheck = zoneOrReceiver.stateValue(zonePowerStateTypeId)
    elif zoneOrReceiver.thingClassId == receiverThingClassId:
        parentReceiver = zoneOrReceiver
        deviceIp = zoneOrReceiver.stateValue(receiverUrlStateTypeId)
        bodyStart = '<YAMAHA_AV cmd="PUT"><Main_Zone>'
        bodyEnd = '</Main_Zone></YAMAHA_AV>'
        source = zoneOrReceiver.stateValue(receiverInputSourceStateTypeId)
        playRandomId = receiverPlayRandomBrowserItemActionTypeId
        powerCheck = zoneOrReceiver.stateValue(receiverPowerStateTypeId)
    rUrl = 'http://' + deviceIp + ':80/YamahaRemoteControl/ctrl'
    headers = {'Content-Type': 'text/xml', 'Accept': '*/*'}
    maxItems = 128
    # maxItems is used to truncate very long lists, as browsing them is very slow due to the nature of Yamaha's API
    # the value of maxItems needs to be a multiple of 8 to work correctly with the browseResponse pages that contain 8 lines
    # and it needs to be browsable within nymea's browseThing timeout, which would be around 264-304
    # but it also seems quite easy to overload the device by making to many API calls, so we limit to 128
    # (if you want to test this and get stuck, powering off the receiver (not via nymea) should help)

    browsableSources = ["SERVER", "USB"]
    if source in browsableSources:
        logger.log("Source %s is browsable" % (source))
        # turn receiver/zone on if needed, before browsing
        if powerCheck == False:
            body = bodyStart + '<Power_Control><Power>On</Power></Power_Control>' + bodyEnd
            rr = requests.post(rUrl, headers=headers, data=body)
    else:
        logger.log("Source %s is not browsable" % (source))
        browseResult.addItem(nymea.BrowserItem("Empty", "Source not browsable", "Non-selectable item", executable=False, disabled=True, icon=nymea.BrowserIconFavorites))
        browseResult.finish(nymea.ThingErrorNoError)
        return

    if browseResult.itemId == "":
        # go to first menu layer
        selType = "BI"
        selLayer = 1
        selItem = 0
        selTxt = "Main menu"
        for i in range(1, 6):
            if i == 1:
                shortcutId = receiverSettingsBrowsingShortcut1ParamTypeId
                labelId = receiverSettingsShortcutLabel1ParamTypeId
            elif i == 2:
                shortcutId = receiverSettingsBrowsingShortcut2ParamTypeId
                labelId = receiverSettingsShortcutLabel2ParamTypeId
            elif i == 3:
                shortcutId = receiverSettingsBrowsingShortcut3ParamTypeId
                labelId = receiverSettingsShortcutLabel3ParamTypeId
            elif i == 4:
                shortcutId = receiverSettingsBrowsingShortcut4ParamTypeId
                labelId = receiverSettingsShortcutLabel4ParamTypeId
            elif i == 5:
                shortcutId = receiverSettingsBrowsingShortcut5ParamTypeId
                labelId = receiverSettingsShortcutLabel5ParamTypeId
            browseTree = parentReceiver.setting(shortcutId)
            labelTxt = parentReceiver.setting(labelId)
            if len(browseTree) > 0 and source == "SERVER": # shortcut is configured, and source needs to be server
                scLayer = len(browseTree) + 1
                subTxt = "Shortcut to " + browseTree
                treeInfo = "SC-layer-" + str(scLayer) + "-item-" + str(0) + "-" + browseTree
                browseResult.addItem(nymea.BrowserItem(treeInfo, labelTxt, subTxt, browsable=True, icon=nymea.BrowserIconFavorites))
    else:
        selType, selLayer, selItem, selTxt = splitBrowseItem(browseResult.itemId)

    # go up to the selected menu level if needed
    browseResponse, menuLayer = browseMenuReady(rUrl, source)
    while menuLayer > selLayer:
        menuLevelUp(rUrl, source)
        browseResponse, menuLayer = browseMenuReady(rUrl, source)
    
    if selType == "BI":
        selectLine(rUrl, source, selItem)
    elif selType == "EL":
        # jump to first line of truncated part of list
        gotoLine(rUrl, source, selItem)
    elif selType == "SC":
        # shortcut, browse shortcut tree
        browseTree = selTxt.split("/")
        selLayer = browseInTree(rUrl, source, browseTree)
        browseResponse, menuLayer = browseMenuReady(rUrl, source)
        if menuLayer == len(browseTree)+1 and menuLayer > 0:
            # don't do anything unless browsing to the required menu item succeeded
            logger.log("Browsing to required menu item succeeded")
            selLayer = len(browseTree)+1
            selItem = 0
            selTxt = "Main menu"
        else:
            logger.log("Browsing to required menu item unsuccessful")
            # go up to the selected menu level if needed
            while menuLayer > selLayer:
                menuLevelUp(rUrl, source)
                browseResponse, menuLayer = browseMenuReady(rUrl, source)
            selLayer = 1
        selType = "BI"
        selItem = 0
        selTxt = "Main menu"

    # browse menu level: keep going through menu pages (of 8 items per page) while last page hasn't been reached
    loop = True
    while loop == True:
        browseResponse, menuLayer = browseMenuReady(rUrl, source)
        currentLine, maxLine = getLineNbrs(browseResponse)
        remainder = currentLine % maxItems
        logger.log("selType", selType, "currentLine", currentLine, "remainder", remainder)
        # long lists (longer than maxItems) are truncated and can be extended with user action
        if selType == "BI" and remainder == 1 and currentLine != 1:
            # truncate list, and create browsable element that will allow user to continue browsing
            # create info about menu structure (BI = browsable item, EL = extend list in case long list was truncated)
            treeInfo = "EL-layer-" + str(menuLayer) + "-item-" + str(currentLine) + "-truncated"
            browseResult.addItem(nymea.BrowserItem(treeInfo, "Continue", "Click to show the next part of this list", browsable=True, icon=nymea.BrowserIconFolder))
            # truncate results, stop loop
            loop = False
        elif selType == "EL" and remainder == 1 and currentLine != selItem:
            # truncate list again, and create browsable element that will allow user to continue browsing
            # create info about menu structure (BI = browsable item, EL = extend list in case long list was truncated)
            treeInfo = "EL-layer-" + str(menuLayer) + "-item-" + str(currentLine) + "-truncated"
            browseResult.addItem(nymea.BrowserItem(treeInfo, "Continue", "Click to show the next part of this list", browsable=True, icon=nymea.BrowserIconFolder))
            # truncate results, stop loop
            loop = False
        else:
            # read the 8 lines in the current browseResponse page
            for i in range(1, 9):
                itemTxt, itemAttr = readLine(browseResponse, i)
                itemTxtClean = unescape(itemTxt, {"&amp;": "&", "&apos;": "'", "&quot;": '"'})
                # create info about menu structure (BI = browsable item, EL = extend list in case long list was truncated)
                treeInfo = "BI-layer-" + str(menuLayer) + "-item-" + str(currentLine+i-1) + "-" + itemTxt
                if itemAttr == "Container":
                    if source == "SERVER" and menuLayer == 1: # add browserItemAction play random album
                        # change when nymea supports browserItemActions for python plugins:
                        # browseResult.addItem(nymea.BrowserItem(treeInfo, itemTxtClean, browsable=True, icon=nymea.BrowserIconFolder, browserItemActions=playRandomId))
                        browseResult.addItem(nymea.BrowserItem(treeInfo, itemTxtClean, browsable=True, icon=nymea.BrowserIconFolder))
                    else:
                        browseResult.addItem(nymea.BrowserItem(treeInfo, itemTxtClean, browsable=True, icon=nymea.BrowserIconFolder))
                elif itemAttr == "Item":
                    browseResult.addItem(nymea.BrowserItem(treeInfo, itemTxtClean, executable=True, icon=nymea.BrowserIconMusic))
                else:
                    # found unselectable item, indicating end of list, stop loop
                    if len(itemTxt) > 0:
                        browseResult.addItem(nymea.BrowserItem(treeInfo, itemTxt, "Not selectable on this receiver", executable=False, disabled=True, icon=nymea.BrowserIconFavorites))
                    else:
                        loop = False
            if maxLine > currentLine + 7 and loop == True:
                # end of list not yet reached, go to next page
                pageDown(rUrl, source)
            else:
                # last page, stop loop
                loop = False
    
    browseResult.finish(nymea.ThingErrorNoError)
    return

def executeBrowserItem(info):
    zoneOrReceiver = info.thing
    pollReceiver(zoneOrReceiver)
    if zoneOrReceiver.thingClassId == zoneThingClassId:
        # get parent receiver thing, needed to get deviceIp
        for possibleParent in myThings():
            if possibleParent.id == zoneOrReceiver.parentId:
                parentReceiver = possibleParent
        deviceIp = parentReceiver.stateValue(receiverUrlStateTypeId)
        source = zoneOrReceiver.stateValue(zoneInputSourceStateTypeId)
    elif zoneOrReceiver.thingClassId == receiverThingClassId:
        deviceIp = zoneOrReceiver.stateValue(receiverUrlStateTypeId)
        source = zoneOrReceiver.stateValue(receiverInputSourceStateTypeId)
    rUrl = 'http://' + deviceIp + ':80/YamahaRemoteControl/ctrl'
    
    selType, selLayer, selItem, selTxt = splitBrowseItem(info.itemId)

    # go up to the selected menu level if needed
    browseResponse, menuLayer = browseMenuReady(rUrl, source)
    while menuLayer > selLayer:
        menuLevelUp(rUrl, source)
        browseResponse, menuLayer = browseMenuReady(rUrl, source)

    selectLine(rUrl, source, selItem)

    info.finish(nymea.ThingErrorNoError)
    time.sleep(0.5)
    pollReceiver(zoneOrReceiver)
    return

def executeBrowserItemAction(info):
    if info.actionTypeId == receiverPlayRandomBrowserItemActionTypeId or info.actionTypeId == zonePlayRandomBrowserItemActionTypeId:
        if info.thing.thingClassId == zoneThingClassId:
            # get parent receiver thing, needed to get deviceIp
            for possibleParent in myThings():
                if possibleParent.id == info.thing.parentId:
                    parentReceiver = possibleParent
            deviceIp = parentReceiver.stateValue(receiverUrlStateTypeId)
            zoneId = info.thing.paramValue(zoneThingZoneIdParamTypeId)
            source = info.thing.stateValue(zoneInputSourceStateTypeId)
        elif info.thing.thingClassId == receiverThingClassId:
            deviceIp = info.thing.stateValue(receiverUrlStateTypeId)
            source = info.thing.stateValue(receiverInputSourceStateTypeId)
        rUrl = 'http://' + deviceIp + ':80/YamahaRemoteControl/ctrl'
        playRandomAlbum(rUrl, source)
    return

def selectLine(rUrl, source, selItem):
    if selItem > 0:
        headers = {'Content-Type': 'text/xml', 'Accept': '*/*'}
        gotoLine(rUrl, source, 1)
        browseResponse, menuLayer = browseMenuReady(rUrl, source)
        currentLine, maxLine = getLineNbrs(browseResponse)
        while selItem > currentLine + 7:
            # jump to the list page with the selected line
            remainder = selItem % 8
            if remainder == 0:
                remainder = 8
            jumpBody = '<YAMAHA_AV cmd="PUT"><SERVER><List_Control><Jump_Line>' + str(selItem - remainder + 1) + '</Jump_Line></List_Control></SERVER></YAMAHA_AV>'
            jr = requests.post(rUrl, headers=headers, data=jumpBody)
            # confirm we got to right page
            browseResponse, menuLayer = browseMenuReady(rUrl, source)
            currentLine, maxLine = getLineNbrs(browseResponse)
        # now select correct line to go to the next menu level
        selectBody = '<YAMAHA_AV cmd="PUT"><' + source + '><List_Control><Direct_Sel>Line_' + str(selItem - currentLine + 1) + '</Direct_Sel></List_Control></' + source + '></YAMAHA_AV>'
        sr = requests.post(rUrl, headers=headers, data=selectBody)
    return

def pageDown(rUrl, source):
    # scroll to next page of list
    headers = {'Content-Type': 'text/xml', 'Accept': '*/*'}
    scrollBody = '<YAMAHA_AV cmd="PUT"><' + source + '><List_Control><Page>Down</Page></List_Control></' + source + '></YAMAHA_AV>'
    sr = requests.post(rUrl, headers=headers, data=scrollBody)
    return

def menuLevelUp(rUrl, source):
    headers = {'Content-Type': 'text/xml', 'Accept': '*/*'}
    returnBody = '<YAMAHA_AV cmd="PUT"><' + source + '><List_Control><Cursor>Return</Cursor></List_Control></' + source + '></YAMAHA_AV>'
    ur = requests.post(rUrl, headers=headers, data=returnBody)
    return

def readLine(browseResponse, i):
    lineResult = []
    stringIndex1 = browseResponse.find("<Line_" + str(i) + ">")
    stringIndex2 = browseResponse.find("</Line_" + str(i) + ">")
    browseTxt = browseResponse[stringIndex1+8:stringIndex2]
    stringIndex1 = browseTxt.find("<Txt>")
    stringIndex2 = browseTxt.find("</Txt>")
    itemTxt = browseTxt[stringIndex1+5:stringIndex2]
    stringIndex1 = browseTxt.find("<Attribute>")
    stringIndex2 = browseTxt.find("</Attribute>")
    itemAttr = browseTxt[stringIndex1+11:stringIndex2]
    return itemTxt, itemAttr

def splitBrowseItem(itemId):
    splitId = itemId.split("-",5)
    selType = splitId[0]
    selLayer = int(splitId[2])
    selItem = int(splitId[4])
    selTxt = splitId[5]
    return selType, selLayer, selItem, selTxt

def getLineNbrs(browseResponse):
    stringIndex1 = browseResponse.find("<Current_Line>")
    stringIndex2 = browseResponse.find("</Current_Line>")
    currentLine = int(browseResponse[stringIndex1+14:stringIndex2])
    stringIndex1 = browseResponse.find("<Max_Line>")
    stringIndex2 = browseResponse.find("</Max_Line>")
    maxLine = int(browseResponse[stringIndex1+10:stringIndex2])
    return currentLine, maxLine

def gotoLine(rUrl, source, lineNbr):
    # e.g. line 1: make sure we are on the first line in the menu before continuing
    headers = {'Content-Type': 'text/xml', 'Accept': '*/*'}
    browseBody = '<YAMAHA_AV cmd="GET"><' + source + '><List_Info>GetParam</List_Info></' + source + '></YAMAHA_AV>'   
    browseResponse, menuLayer = browseMenuReady(rUrl, source)
    jumpBody = '<YAMAHA_AV cmd="PUT"><' + source + '><List_Control><Jump_Line>' + str(lineNbr) + '</Jump_Line></List_Control></' + source + '></YAMAHA_AV>'
    jr = requests.post(rUrl, headers=headers, data=jumpBody)
    return

def browseMenuReady(rUrl, source):
    # make sure menu status is Ready before sending any further commands, as they may not be processed by the receiver
    # at same time, return list info as we got it anyway when checking menu status
    headers = {'Content-Type': 'text/xml', 'Accept': '*/*'}
    browseBody = '<YAMAHA_AV cmd="GET"><' + source + '><List_Info>GetParam</List_Info></' + source + '></YAMAHA_AV>'   
    ready = False
    while ready == False:
        br = requests.post(rUrl, headers=headers, data=browseBody)
        browseResponse = br.text
        stringIndex1 = browseResponse.find("<Menu_Status>")
        stringIndex2 = browseResponse.find("</Menu_Status>")
        menuStatus = browseResponse[stringIndex1+13:stringIndex2]
        if menuStatus == "Ready":
            ready = True
            stringIndex1 = browseResponse.find("<Menu_Layer>")
            stringIndex2 = browseResponse.find("</Menu_Layer>")
            menuLayer = int(browseResponse[stringIndex1+12:stringIndex2])
            stringIndex1 = browseResponse.find("<Menu_Name>")
            stringIndex2 = browseResponse.find("</Menu_Name>")
            menuTitle = browseResponse[stringIndex1+11:stringIndex2]
            logger.log("Menu layer", menuLayer, "Menu title", menuTitle)
        else:
            time.sleep(0.1)
    return browseResponse, menuLayer

def deinit():
    global pollTimer
    # If we started a poll timer, cancel it on shutdown.
    if pollTimer is not None:
        pollTimer = None

def thingRemoved(thing):
    global pollTimer
    logger.log("removeThing called for", thing.name)
    # Clean up all data related to this thing
    if len(myThings()) == 0 and pollTimer is not None:
        pollTimer = None
