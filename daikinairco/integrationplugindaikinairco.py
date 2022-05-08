import nymea
import time
import threading
import json
import requests
import random
from socket import *
import sys

pollTimer = None
pollFrequency = 30
ipMap = {}

def discoverThings(info):
    logger.log("Discovery started for", info.thingClassId)
    discoveredIps = findIps()
    
    for i in range(0, len(discoveredIps)):
        deviceIp = discoveredIps[i]
        # get basic info /common/basic_info
        # response example:
        #   ret=OK,type=aircon,reg=eu,dst=1,ver=1_14_58,rev=83B3526,pow=0,err=0,location=0,
        #   name=name-in-%hex,icon=0,method=polling,port=30050,id=uuid,
        #   pw=,lpw_flag=0,adp_kind=3,pv=3.30,cpv=3,cpv_minor=20,led=1,en_setzone=1,mac=mac,adp_mode=run,en_hol=0,
        #   ssid1=ssid,radio1=-43,ssid=DaikinAP14937,grp_name=,en_grp=0
        
        # get model info /aircon/get_model_info --> (how) can we use this?
        # response example:
        #   ret=OK,model=0FC3,type=N,pv=3.30,cpv=3,cpv_minor=20,mid=NA,humd=0,s_humd=0,acled=0,land=0,elec=1,temp=1,
        #   temp_rng=0,m_dtct=1,ac_dst=--,disp_dry=0,dmnd=1,en_scdltmr=1,en_frate=1,en_fdir=1,s_fdir=3,en_rtemp_a=0,
        #   en_spmode=5,en_ipw_sep=1,en_mompow=1,hmlmt_l=10.0

        aUrl = 'http://' + deviceIp + '/common/basic_info'
        headers = {'Accept': '*/*'}
        rr = requests.get(aUrl, headers=headers)
        pollResponse = rr.text
        if rr.status_code == requests.codes.ok:
            systemType = 'none'
            splitResponse = pollResponse.split(",")
            for j in range(0, len(splitResponse)):
                splitItem = splitResponse[j].split("=")
                if splitItem[0] == 'type':
                    systemType = splitItem[1]
                elif splitItem[0] == 'id':
                    systemId = splitItem[1]
                elif splitItem[0] == 'name':
                    hexArray = splitItem[1].split("%")
                    deviceName = ""
                    for k in range(0, len(hexArray)):
                        deviceName+=bytes.fromhex(hexArray[k]).decode('utf-8')
            if systemType == 'aircon':
                logger.log("Device with IP " + deviceIp + " is a Daikin airco unit.")
                logger.log("Device ID:", systemId)
                logger.log("Device Name:", deviceName)
                # check if device already known
                exists = False
                for thing in myThings():
                    logger.log("Comparing to existing units: is %s an airco unit?" % (thing.name))
                    if thing.thingClassId == aircoThingClassId:
                        logger.log("Yes, %s is an airco unit." % (thing.name))
                        if thing.paramValue(aircoThingDeviceIdParamTypeId) == systemId:
                            logger.log("Already have unit with serial number %s in the system: %s" % (systemId, thing.name))
                            exists = True
                        else:
                            logger.log("Thing %s doesn't match with found unit with serial number %s" % (thing.name, systemId))
                if exists == False: # unit doesn't exist yet, so add it
                    thingDescriptor = nymea.ThingDescriptor(aircoThingClassId, deviceName)
                    thingDescriptor.params = [
                        nymea.Param(aircoThingDeviceIdParamTypeId, systemId),
                        nymea.Param(aircoThingDeviceNameParamTypeId, deviceName)
                    ]
                    info.addDescriptor(thingDescriptor)
                else: # unit already exists, so show it to allow reconfiguration
                    thingDescriptor = nymea.ThingDescriptor(aircoThingClassId, deviceName, thingId=thing.id)
                    thingDescriptor.params = [
                        nymea.Param(aircoThingDeviceIdParamTypeId, systemId),
                        nymea.Param(aircoThingDeviceNameParamTypeId, deviceName)
                    ]
                    info.addDescriptor(thingDescriptor)
            else:
                logger.log("Device with IP " + deviceIp + " does not appear to be a supported Daikin airco unit.")
        else:
            logger.log("Device with IP " + deviceIp + " does not appear to be a supported Daikin airco unit.")
    info.finish(nymea.ThingErrorNoError)
    return

def findIps():
    discoveredIps = []

    # Create a UDP socket
    sock = socket(AF_INET, SOCK_DGRAM)
    sock.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
    sock.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
    sock.settimeout(20)

    server_address = ('255.255.255.255', 30050)
    message = 'DAIKIN_UDP/common/basic_info'

    try:
        while True:
            # Send data
            logger.log('sending: ' + message)
            sent = sock.sendto(message.encode(), server_address)

            # Receive response
            logger.log('waiting to receive')
            data, server = sock.recvfrom(4096)
            if data.decode('UTF-8')[0:18] == 'ret=OK,type=aircon':
                print('Received confirmation; server ip: ' + str(server[0]) )
                discoveredIps.append(str(server[0]))
                break
            else:
                print('Verification failed')
            
            print('Trying again...')
    finally:	
        sock.close()
    return discoveredIps
    
def findDeviceIP(airco):
    searchSystemId = airco.paramValue(aircoThingDeviceIdParamTypeId)
    discoveredIps = findIps()
    foundIp = None
    found = False
    for i in range(0, len(discoveredIps)):
        deviceIp = discoveredIps[i]
        aUrl = 'http://' + deviceIp + '/common/basic_info'
        headers = {'Accept': '*/*'}
        rr = requests.get(aUrl, headers=headers)
        pollResponse = rr.text
        if rr.status_code == requests.codes.ok:
            logger.log("Device with IP " + deviceIp + " is a supported Daikin Airco.")
            # get device info
            splitResponse = pollResponse.split(",")
            for j in range(0, len(splitResponse)):
                splitItem = splitResponse[j].split("=")
                if splitItem[0] == 'id':
                    systemId = splitItem[1]
            logger.log("Device ID:", systemId)
            # check if this is the device with the serial number we're looking for
            if systemId == searchSystemId:
                logger.log("Device with IP " + deviceIp + " is the existing device.")
                found = True
                foundIp = deviceIp
                ipMap[airco] = foundIp
        else:
            logger.log("Device with IP " + deviceIp + " does not appear to be a supported Daikin Airco.")
    if found == True:
        airco.setStateValue(aircoConnectedStateTypeId, True)
    else:
        airco.setStateValue(aircoConnectedStateTypeId, False)
    return found

def setupThing(info):
    searchSystemId = info.thing.paramValue(aircoThingDeviceIdParamTypeId)
    logger.log("setupThing called for", info.thing.name, searchSystemId)
    found = findDeviceIP(info.thing)
    if found == True:
        pollAirco(info.thing)
        info.finish(nymea.ThingErrorNoError)
    else:
        info.finish(nymea.ThingErrorHardwareFailure, "Error connecting to the device in the network.")
    
    logger.log("Airco added:", info.thing.name)

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

def pollAirco(info):
    global pollFrequency
    global ipMap
    deviceIp = ipMap[info]
    logger.log("polling airco", deviceIp, info.name)
    airco = info
    headers = {'Accept': '*/*'}
    # get sensor info
    # response example:
    #   ret=OK,htemp=25.0,hhum=30,otemp=2.0,err=0,cmpfreq=0,mompow=1
    aUrl = 'http://' + deviceIp + '/aircon/get_sensor_info'
    pr = requests.get(aUrl, headers=headers)
    pollResponse = pr.text
    if pr.status_code == requests.codes.ok:
        airco.setStateValue(aircoConnectedStateTypeId, True)
        splitResponse = pollResponse.split(",")
        for i in range(0, len(splitResponse)):
            splitItem = splitResponse[i].split("=")
            if splitItem[0] == 'htemp':
                homeTemp = float(splitItem[1])
                airco.setStateValue(aircoTemperatureStateTypeId, homeTemp)
            elif splitItem[0] == 'hhum':
                homeHumidity = float(splitItem[1])
                airco.setStateValue(aircoHumidityStateTypeId, homeHumidity)
            elif splitItem[0] == 'otemp':
                outTemp = float(splitItem[1])
                airco.setStateValue(aircoOutsideTemperatureStateTypeId, outTemp)
    else:
        airco.setStateValue(aircoConnectedStateTypeId, False)
        #device disconnected, IP may have changed, so search again
        found = findDeviceIP(airco)
        if found == False:
            return

    # get control info
    # response example:
    #   ret=OK,pow=0,mode=4,adv=,stemp=21.0,shum=0,dt1=22.0,dt2=M,dt3=20.0,dt4=21.0,dt5=21.0,dt7=22.0,
    #   dh1=0,dh2=0,dh3=0,dh4=0,dh5=0,dh7=0,dhh=50,b_mode=4,b_stemp=21.0,b_shum=0,alert=255,
    #   f_rate=A,f_dir=0,b_f_rate=A,b_f_dir=0,dfr1=A,dfr2=A,dfr3=A,dfr4=A,dfr5=A,dfr6=A,dfr7=A,dfrh=5,
    #   dfd1=0,dfd2=0,dfd3=0,dfd4=0,dfd5=0,dfd6=0,dfd7=0,dfdh=0,dmnd_run=0,en_demand=1
    aUrl = 'http://' + deviceIp + '/aircon/get_control_info'
    pr = requests.get(aUrl, headers=headers)
    pollResponse = pr.text
    if pr.status_code == requests.codes.ok:
        airco.setStateValue(aircoConnectedStateTypeId, True)
        splitResponse = pollResponse.split(",")
        for i in range(0, len(splitResponse)):
            splitItem = splitResponse[i].split("=")
            # get power state
            if splitItem[0] == 'pow':
                power = int(splitItem[1])
                logger.log("Power", power)
                airco.setStateValue(aircoPowerStateTypeId, power)
            elif splitItem[0] == 'mode':
                mode = int(splitItem[1])
                if mode == 2: # 2 = dehumidifier mode
                    logger.log("Airco mode: dehumidifier", mode)
                    airco.setStateValue(aircoModeStateTypeId, "Dehumidifier")
                elif mode == 3: # 3 = cooling mode
                    logger.log("Airco mode: cooling", mode)
                    airco.setStateValue(aircoModeStateTypeId, "Cooling")
                elif mode == 4: # 4 = heating mode
                    logger.log("Airco mode: heating", mode)
                    airco.setStateValue(aircoModeStateTypeId, "Heating")
                elif mode == 6: # 6 = fan mode
                    logger.log("Airco mode: fan", mode)
                    airco.setStateValue(aircoModeStateTypeId, "Fan")
                else: # 0-1-7 = automatic mode
                    logger.log("Airco mode: automatic", mode)
                    airco.setStateValue(aircoModeStateTypeId, "Automatic")
            elif splitItem[0] == 'stemp':
                try:
                    targetTemp = float(splitItem[1])
                    logger.log("Target temperature", targetTemp)
                    airco.setStateValue(aircoTargetTemperatureStateTypeId, targetTemp)
                except:
                    logger.log("Target temperature parameter is not a number:", splitItem[1])
                    for j in range(0, len(splitResponse)):
                        splitItem = splitResponse[j].split("=")
                        if splitItem[0] == 'dt1':
                            targetTemp = float(splitItem[1])
                            logger.log("Using target temperature from automatic mode", targetTemp)
                            airco.setStateValue(aircoTargetTemperatureStateTypeId, targetTemp)
    else:
        airco.setStateValue(aircoConnectedStateTypeId, False)

    #set heatingOn / coolingOn based on mode, temperature & target temperature (API doesn't seem to return it)
    targetTemperature = airco.stateValue(aircoTargetTemperatureStateTypeId)
    temperature = airco.stateValue(aircoTemperatureStateTypeId)
    mode = airco.stateValue(aircoModeStateTypeId)
    power = airco.stateValue(aircoPowerStateTypeId)
    if mode == "Cooling" and power == True:
        airco.setStateValue(aircoHeatingOnStateTypeId, False)
        if targetTemperature < temperature:
            airco.setStateValue(aircoCoolingOnStateTypeId, True)
        else:
            airco.setStateValue(aircoCoolingOnStateTypeId, False)
    elif mode == "Heating" and power == True:
        airco.setStateValue(aircoCoolingOnStateTypeId, False)
        if targetTemperature > temperature:
            airco.setStateValue(aircoHeatingOnStateTypeId, True)
        else:
            airco.setStateValue(aircoHeatingOnStateTypeId, False)
    elif mode == "Automatic" and power == True:
        if targetTemperature == temperature:
            airco.setStateValue(aircoCoolingOnStateTypeId, False)
            airco.setStateValue(aircoHeatingOnStateTypeId, False)
        elif targetTemperature > temperature:
            airco.setStateValue(aircoCoolingOnStateTypeId, False)
            airco.setStateValue(aircoHeatingOnStateTypeId, True)
        else:
            airco.setStateValue(aircoCoolingOnStateTypeId, True)
            airco.setStateValue(aircoHeatingOnStateTypeId, False)
    else:
        airco.setStateValue(aircoCoolingOnStateTypeId, False)
        airco.setStateValue(aircoHeatingOnStateTypeId, False)

def pollService():
    logger.log("pollTimer triggered")
    global pollTimer
    global pollFrequency
    pollTimer.interval = pollFrequency
    # Poll all airco units we know
    for thing in myThings():
        if thing.thingClassId == aircoThingClassId:
            pollAirco(thing)

def executeAction(info):
    global ipMap
    deviceIp = ipMap[info.thing]
    logger.log("executeAction called for thing", info.thing.name, deviceIp, info.actionTypeId, info.params)
    headers = {'Accept': '*/*'}
    aUrl = 'http://' + deviceIp + '/aircon/get_control_info'
    pr = requests.get(aUrl, headers=headers)
    # get control info, to obtain target temperature per mode, to set the correct target temperature (stemp) when changing mode
    pollResponse = pr.text
    logger.log("response", pr.text)
    if pr.status_code == requests.codes.ok:
        info.thing.setStateValue(aircoConnectedStateTypeId, True)
        splitResponse = pollResponse.split(",")
        for i in range(0, len(splitResponse)):
            splitItem = splitResponse[i].split("=")
            if splitItem[0] == 'pow':
                power = int(splitItem[1])
            elif splitItem[0] == 'mode':
                mode = int(splitItem[1])
            elif splitItem[0] == 'stemp':
                targetTemp = splitItem[1]
            elif splitItem[0] == 'shum':
                targetHum = splitItem[1]
            elif splitItem[0] == 'f_rate':
                f_rate = splitItem[1]
            elif splitItem[0] == 'f_dir':
                f_dir = splitItem[1]
            elif splitItem[0] == 'dt1':
                dt1 = splitItem[1]
            elif splitItem[0] == 'dt2':
                dt2 = splitItem[1]
            elif splitItem[0] == 'dt3':
                dt3 = splitItem[1]
            elif splitItem[0] == 'dt4':
                dt4 = splitItem[1]
            elif splitItem[0] == 'dt5':
                dt5 = splitItem[1]
            elif splitItem[0] == 'dt7':
                dt7 = splitItem[1]
            elif splitItem[0] == 'dh1':
                dh1 = splitItem[1]
            elif splitItem[0] == 'dh2':
                dh2 = splitItem[1]
            elif splitItem[0] == 'dh3':
                dh3 = splitItem[1]
            elif splitItem[0] == 'dh4':
                dh4 = splitItem[1]
            elif splitItem[0] == 'dh5':
                dh5 = splitItem[1]
            elif splitItem[0] == 'dh7':
                dh7 = splitItem[1]
    else:
        info.thing.setStateValue(aircoConnectedStateTypeId, False)
        info.finish(nymea.ThingErrorNoError)

    if info.actionTypeId == aircoPowerActionTypeId:
        power = info.paramValue(aircoPowerActionPowerParamTypeId)
        if power == True:
            power = 1
        elif power == False:
            power = 0
        logger.log("new power value", power)
    elif info.actionTypeId == aircoTargetTemperatureActionTypeId:
        targetTemp = str(info.paramValue(aircoTargetTemperatureActionTargetTemperatureParamTypeId))
        logger.log("new target temperature", targetTemp)
    elif info.actionTypeId == aircoModeActionTypeId:
        targetMode = info.paramValue(aircoModeActionModeParamTypeId)
        if targetMode == "Automatic":
            mode = 0
            targetTemp = dt1
            targetHum = dh1
        elif targetMode == "Cooling":
            mode = 3
            targetTemp = dt3
            targetHum = dh3
        elif targetMode == "Heating":
            mode = 4
            targetTemp = dt4
            targetHum = dh4
        elif targetMode == "Dehumidifier":
            mode = 2
            targetTemp = dt2
            targetHum = dh2
        elif targetMode == "Fan":
            mode = 6
        logger.log("new target mode:", targetMode, mode)

    # mandatory parameters: pow, mode, stemp, shum, f_rate, f_dir (f_rate & f_dir can't be set from the plugin yet, so we always use the values obtained above)
    param = "pow="+str(power)+"&mode="+str(mode)+"&stemp="+targetTemp+"&shum="+targetHum+"&f_rate="+f_rate+"&f_dir="+f_dir
    logger.log("parameters", param)
    aUrl = "http://" + deviceIp + "/aircon/set_control_info?"+param
    pr = requests.post(aUrl, headers=headers)
    time.sleep(0.5)
    pollAirco(info.thing)
    info.finish(nymea.ThingErrorNoError)
    return

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