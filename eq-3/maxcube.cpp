/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "maxcube.h"
#include "extern-plugininfo.h"

MaxCube::MaxCube(QObject *parent, QString serialNumber, QHostAddress hostAdress, quint16 port):
    QTcpSocket(parent), m_serialNumber(serialNumber), m_hostAddress(hostAdress), m_port(port)
{

    m_cubeInitialized = false;

    connect(this, &MaxCube::stateChanged, this, &MaxCube::connectionStateChanged);
    connect(this, &MaxCube::readyRead, this, &MaxCube::onReadyRead);
    connect(this, &MaxCube::cubeDataAvailable, this, &MaxCube::processCubeData);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    connect(this, &MaxCube::errorOccurred, this, &MaxCube::onTcpError);
#else
    connect(this, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onTcpError(QAbstractSocket::SocketError)));
#endif

}

QString MaxCube::serialNumber() const
{
    return m_serialNumber;
}

void MaxCube::setSerialNumber(const QString &serialNumber)
{
    m_serialNumber = serialNumber;
}

QByteArray MaxCube::rfAddress() const
{
    return m_rfAddress;
}

void MaxCube::setRfAddress(const QByteArray &rfAddress)
{
    m_rfAddress = rfAddress;
}

int MaxCube::firmware() const
{
    return m_firmware;
}

void MaxCube::setFirmware(const int &firmware)
{
    m_firmware = firmware;
}

QHostAddress MaxCube::hostAddress() const
{
    return m_hostAddress;
}

void MaxCube::setHostAddress(const QHostAddress &hostAddress)
{
    m_hostAddress = hostAddress;
}

quint16 MaxCube::port() const
{
    return m_port;
}

void MaxCube::setPort(const quint16 &port)
{
    m_port = port;
}

QDateTime MaxCube::cubeDateTime() const
{
    return m_cubeDateTime;
}

void MaxCube::setCubeDateTime(const QDateTime &cubeDateTime)
{
    m_cubeDateTime = cubeDateTime;
}

bool MaxCube::portalEnabeld() const
{
    return m_portalEnabeld;
}

QList<WallThermostat *> MaxCube::wallThermostatList()
{
    return m_wallThermostatList;
}

QList<RadiatorThermostat *> MaxCube::radiatorThermostatList()
{
    return m_radiatorThermostatList;
}

QList<Room *> MaxCube::roomList()
{
    return m_roomList;
}

void MaxCube::connectToCube()
{
    connectToHost(m_hostAddress,m_port);
}

void MaxCube::disconnectFromCube()
{
    disconnectFromHost();
}

bool MaxCube::sendData(QByteArray data)
{
    if(write(data) < 0){
        return false;
    }
    return true;
}

bool MaxCube::isConnected()
{
    if(state() == QAbstractSocket::ConnectedState){
        return true;
    }
    return false;
}

bool MaxCube::isInitialized()
{
    return m_cubeInitialized;
}

void MaxCube::decodeHelloMessage(QByteArray data)
{
    QList<QByteArray> list = data.split(',');
    m_cubeDateTime = calculateDateTime(list.at(7),list.at(8));

    m_rfAddress = list.at(1);
    m_firmware = list.at(2).toInt();

    qCDebug(dcEQ3) << "====================================================";
    qCDebug(dcEQ3) << "               HELLO message:";
    qCDebug(dcEQ3) << "====================================================";
    qCDebug(dcEQ3) << "           serial number | " << m_serialNumber;
    qCDebug(dcEQ3) << "        RF address (hex) | " << m_rfAddress;
    qCDebug(dcEQ3) << "                firmware | " << m_firmware;
    qCDebug(dcEQ3) << "               Cube date | " << m_cubeDateTime.date().toString("dd.MM.yyyy");
    qCDebug(dcEQ3) << "               Cube time | " << m_cubeDateTime.time().toString("HH:mm");
    qCDebug(dcEQ3) << "         State Cube Time | " << list.at(9);
    qCDebug(dcEQ3) << "             NTP counter | " << list.at(10);
}

void MaxCube::decodeMetadataMessage(QByteArray data)
{
    QList<QByteArray> list = data.left(data.length()-2).split(',');
    QByteArray dataDecoded = QByteArray::fromBase64(list.at(2));
    qCDebug(dcEQ3) << "====================================================";
    qCDebug(dcEQ3) << "               METADATA message:";
    qCDebug(dcEQ3) << "====================================================";

    // parse room list
    int roomCount = dataDecoded.toHex().mid(4,2).toInt(0,16);

    QByteArray roomRawData = dataDecoded.toHex();
    roomRawData = roomRawData.right(roomRawData.length()-6);

    for(int i = 0; i < roomCount; i++){
        Room *room = new Room(this);
        room->setRoomId(roomRawData.left(2).toInt(0,16));
        int roomNameLength = roomRawData.mid(2,2).toInt(0,16);
        room->setRoomName(QByteArray::fromHex(roomRawData.mid(4,roomNameLength*2)));
        room->setGroupRfAddress(roomRawData.mid(roomNameLength*2 + 4, 6));
        m_roomList.append(room);
        roomRawData = roomRawData.right(roomRawData.length() - ((roomNameLength*2) + 10));
    }
    qCDebug(dcEQ3) << "-------------------------|-------------------------";
    qCDebug(dcEQ3) << "found " << m_roomList.count() << "rooms";
    qCDebug(dcEQ3) << "-------------------------|-------------------------";

    foreach (Room *room, m_roomList) {
        qCDebug(dcEQ3) << "               Room Name | " << room->roomName();
        qCDebug(dcEQ3) << "                 Room ID | " << room->roomId();
        qCDebug(dcEQ3) << "        Group RF Address | " << room->groupRfAddress();
        qCDebug(dcEQ3) << "-------------------------|-------------------------";
    }

    // parse thing list
    int deviceCount = roomRawData.left(2).toInt(0,16);
    QByteArray deviceRawData = roomRawData.right(roomRawData.length() - 2);

    qCDebug(dcEQ3) << "-------------------------|-------------------------";
    qCDebug(dcEQ3) << "found " << deviceCount << "devices";
    qCDebug(dcEQ3) << "-------------------------|-------------------------";

    for(int i = 0; i < deviceCount; i++){
        int deviceType = deviceRawData.left(2).toInt(0,16);
        switch (deviceType) {
        case MaxDevice::DeviceRadiatorThermostat:{
            RadiatorThermostat* thing = new RadiatorThermostat(this);
            thing->setDeviceType(deviceType);
            thing->setRfAddress(deviceRawData.mid(2,6));
            thing->setSerialNumber(QByteArray::fromHex(deviceRawData.mid(8,20)));
            int deviceNameLenght = deviceRawData.mid(28,2).toInt(0,16);
            thing->setDeviceName(QByteArray::fromHex(deviceRawData.mid(30,deviceNameLenght*2)));
            thing->setRoomId(deviceRawData.mid(30 + deviceNameLenght*2,2).toInt(0,16));
            deviceRawData = deviceRawData.right(deviceRawData.length() - (32 + deviceNameLenght*2));

            // set room data for each thing
            foreach (Room * room, m_roomList) {
                if(thing->roomId() == room->roomId()){
                    thing->setRoomName(room->roomName());
                }
            }
            m_radiatorThermostatList.append(thing);

            qCDebug(dcEQ3) << "             Device Name | " << thing->deviceName();
            qCDebug(dcEQ3) << "            Serial Number| " << thing->serialNumber();
            qCDebug(dcEQ3) << "      Device Type String | " << thing->deviceTypeString();
            qCDebug(dcEQ3) << "        RF address (hex) | " << thing->rfAddress();
            qCDebug(dcEQ3) << "                 Room ID | " << thing->roomId();
            qCDebug(dcEQ3) << "               Room Name | " << thing->roomName();
            qCDebug(dcEQ3) << "-------------------------|-------------------------";
            break;
        }
        case MaxDevice::DeviceWallThermostat:{
            WallThermostat* thing = new WallThermostat(this);
            thing->setDeviceType(deviceType);
            thing->setRfAddress(deviceRawData.mid(2,6));
            thing->setSerialNumber(QByteArray::fromHex(deviceRawData.mid(8,20)));
            int deviceNameLenght = deviceRawData.mid(28,2).toInt(0,16);
            thing->setDeviceName(QByteArray::fromHex(deviceRawData.mid(30,deviceNameLenght*2)));
            thing->setRoomId(deviceRawData.mid(30 + deviceNameLenght*2,2).toInt(0,16));
            deviceRawData = deviceRawData.right(deviceRawData.length() - (32 + deviceNameLenght*2));

            // set room data for each thing
            foreach (Room * room, m_roomList) {
                if(thing->roomId() == room->roomId()){
                    thing->setRoomName(room->roomName());
                }
            }
            m_wallThermostatList.append(thing);

            qCDebug(dcEQ3) << "             Device Name | " << thing->deviceName();
            qCDebug(dcEQ3) << "            Serial Number| " << thing->serialNumber();
            qCDebug(dcEQ3) << "      Device Type String | " << thing->deviceTypeString();
            qCDebug(dcEQ3) << "        RF address (hex) | " << thing->rfAddress();
            qCDebug(dcEQ3) << "                 Room ID | " << thing->roomId();
            qCDebug(dcEQ3) << "               Room Name | " << thing->roomName();
            qCDebug(dcEQ3) << "-------------------------|-------------------------";
            break;
        }
        default:
            break;
        }
    }

    m_cubeInitialized = true;
}

void MaxCube::decodeConfigMessage(QByteArray data)
{
    QList<QByteArray> list = data.split(',');
    if(list.count() < 2){
        return;
    }
    QByteArray rfAddress = list.at(0);
    QByteArray dataRaw = QByteArray::fromBase64(list.at(1)).toHex();
    int lengthData = dataRaw.left(2).toInt(0,16);
    if(rfAddress != dataRaw.mid(2,6)){
        qCWarning(dcEQ3) << "RF addresses not equal!";
    }
    int deviceType = dataRaw.mid(8,2).toInt(0,16);
    //QByteArray unknown = dataRaw.mid(12,6);

    QByteArray serialNumber = QByteArray::fromHex(dataRaw.mid(16,20));
    qCDebug(dcEQ3) << "====================================================";
    qCDebug(dcEQ3) << "               CONFIG message:";
    qCDebug(dcEQ3) << "====================================================";
    qCDebug(dcEQ3) << "           Serial Number | " << serialNumber;
    qCDebug(dcEQ3) << "             thing Type | " << deviceTypeString(deviceType);
    qCDebug(dcEQ3) << "        RF address (hex) | " << rfAddress;
    qCDebug(dcEQ3) << "             data length | " << lengthData;
    qCDebug(dcEQ3) << "-------------------------|-------------------------";

    switch (deviceType) {
    case MaxDevice::DeviceCube:{

        m_portalEnabeld = (bool)dataRaw.mid(36,2).toInt(0,16);

        qCDebug(dcEQ3) << "          portal enabled | " << m_portalEnabeld;
        qCDebug(dcEQ3) << "              portal URL | " << QString(QByteArray::fromHex(dataRaw.mid(170,68)));
        qCDebug(dcEQ3) << "               time zone | " << QString(QByteArray::fromHex(dataRaw.mid(428,6)));
        qCDebug(dcEQ3) << "      summer/winter time | " << QString(QByteArray::fromHex(dataRaw.mid(452,8)));
        emit cubeConfigReady();
        break;
    }
    case MaxDevice::DeviceRadiatorThermostat:{
        foreach (RadiatorThermostat* thing, m_radiatorThermostatList) {
            if(thing->rfAddress() == rfAddress){

                //int roomId = dataRaw.mid(10,2).toInt(0,16);
                int firmware = dataRaw.mid(12,2).toInt(0,16);
                thing->setComfortTemp((double)dataRaw.mid(36,2).toInt(0,16) / 2.0);
                thing->setEcoTemp((double)dataRaw.mid(38,2).toInt(0,16) / 2.0);
                thing->setMaxSetPointTemp((double)dataRaw.mid(40,2).toInt(0,16) / 2.0);
                thing->setMinSetPointTemp((double)dataRaw.mid(42,2).toInt(0,16) / 2.0);
                thing->setOffsetTemp((double)(dataRaw.mid(44,2).toInt(0,16) / 2.0 ) - 3.5);
                thing->setWindowOpenTemp((double)dataRaw.mid(46,2).toInt(0,16)/2.0);
                thing->setWindowOpenDuration(dataRaw.mid(48,2).toInt(0,16));
                // boost code
                QByteArray boostDurationCode = fillBin(QByteArray::number(dataRaw.mid(50,2).toInt(0,16),2),8);
                thing->setBoostDuration(boostDurationCode.left(3).toInt(0,2) * 5);
                thing->setBoostValveValue(boostDurationCode.right(5).toInt(0,2) * 5);

                // day of week an time
                QByteArray dowTime = fillBin(QByteArray::number(dataRaw.mid(52,2).toInt(0,16),2),8);
                thing->setDiscalcingWeekDay(weekDayString(dowTime.left(3).toInt(0,2)));
                thing->setDiscalcingTime(QTime(dowTime.right(5).toInt(0,2),0));

                thing->setValveMaximumSettings((double)dataRaw.mid(54,2).toInt(0,16)*(double)100.0/255.0);
                thing->setValveOffset((double)dataRaw.mid(56,2).toInt(0,16)*100.0/255.0);

                qCDebug(dcEQ3) << "                 Room ID | " << thing->roomId();
                qCDebug(dcEQ3) << "                firmware | " << firmware;
                //qCDebug(dcEQ3) << "           Confort Temp. | " << thing->confortTemp() << "C";
                qCDebug(dcEQ3) << "               Eco Temp. | " << thing->ecoTemp() << "C";
                qCDebug(dcEQ3) << "    Max. Set Point Temp. | " << thing->maxSetPointTemp() << "C";
                qCDebug(dcEQ3) << "    Min. Set Point Temp. | " << thing->minSetPointTemp() << "C";
                qCDebug(dcEQ3) << "            Temp. Offset | " << thing->offsetTemp() << "C";
                qCDebug(dcEQ3) << "       Window Open Temp. | " << thing->windowOpenTemp() << "C";
                qCDebug(dcEQ3) << "   Window Open Duration  | " << thing->windowOpenDuration() << "min";
                qCDebug(dcEQ3) << "         Boost Duration  | " << thing->boostDuration() << "min";
                qCDebug(dcEQ3) << "             Valve value | " << thing->boostValveValue() << "%";
                qCDebug(dcEQ3) << "     disclaiming run day | " << thing->discalcingWeekDay();
                qCDebug(dcEQ3) << "    disclaiming run time | " << thing->discalcingTime().toString("HH:mm");
                qCDebug(dcEQ3) << "  Valve Maximum Settings | " << thing->valveMaximumSettings() << "%";
                qCDebug(dcEQ3) << "            Valve Offset | " << thing->valveOffset() << "%";
                parseWeeklyProgram(dataRaw.right(dataRaw.length() - 58));
                emit radiatorThermostatFound();
            }
        }
        break;
    }
    case MaxDevice::DeviceRadiatorThermostatPlus:
        break;
    case MaxDevice::DeviceWallThermostat:{
        foreach (WallThermostat* thing, m_wallThermostatList) {
            if(thing->rfAddress() == rfAddress){
                //int roomId = dataRaw.mid(10,2).toInt(0,16);
                int firmware = dataRaw.mid(12,2).toInt(0,16);
                thing->setComfortTemp((double)dataRaw.mid(36,2).toInt(0,16) / 2.0);
                thing->setEcoTemp((double)dataRaw.mid(38,2).toInt(0,16)/2.0);
                thing->setMaxSetPointTemp((double)dataRaw.mid(40,2).toInt(0,16)/2.0);
                thing->setMinSetPointTemp((double)dataRaw.mid(42,2).toInt(0,16)/2.0);

                qCDebug(dcEQ3) << "                 Room ID | " << thing->roomId();
                qCDebug(dcEQ3) << "                firmware | " << firmware;
                //qCDebug(dcEQ3) << "           Confort Temp. | " << thing->confortTemp();
                qCDebug(dcEQ3) << "               Eco Temp. | " << thing->ecoTemp();
                qCDebug(dcEQ3) << "    Max. Set Point Temp. | " << thing->maxSetPointTemp();
                qCDebug(dcEQ3) << "    Min. Set Point Temp. | " << thing->minSetPointTemp();

                parseWeeklyProgram(dataRaw.right(dataRaw.length() - 44));
                emit wallThermostatFound();
            }
        }
        break;
    }
    case MaxDevice::DeviceEcoButton:
        break;
    case MaxDevice::DeviceWindowContact:
        break;
    default:
        qCWarning(dcEQ3) << "unknown thing type: " << deviceType;
        break;
    }
}

void MaxCube::decodeDevicelistMessage(QByteArray data)
{
    qCDebug(dcEQ3) << "====================================================";
    qCDebug(dcEQ3) << "               LIVE message:";
    qCDebug(dcEQ3) << "====================================================";

    QByteArray rawDataAll = QByteArray::fromBase64(data).toHex();
    QList<QByteArray> deviceMessageList = splitMessage(rawDataAll);

    foreach (QByteArray rawData, deviceMessageList) {

        QByteArray rfAddress = rawData.mid(0,6);
        int deviceType = deviceTypeFromRFAddress(rfAddress);

        switch (deviceType) {
        case MaxDevice::DeviceWallThermostat:{
            foreach (WallThermostat *thing, m_wallThermostatList) {
                if(thing->rfAddress() == rfAddress){

                    // init/valid code
                    QByteArray initCode = fillBin(QByteArray::number(rawData.mid(8,2).toInt(0,16),2),8);
                    thing->setInformationValid((bool)initCode.mid(3,1).toInt());
                    thing->setErrorOccurred((bool)initCode.mid(4,1).toInt());
                    thing->setIsAnswereToCommand((bool)initCode.mid(5,1).toInt());
                    thing->setInitialized((bool)initCode.mid(6,1).toInt());

                    // status code
                    QByteArray statusCode = fillBin(QByteArray::number(rawData.mid(10,2).toInt(0,16),2),8);
                    thing->setBatteryLow((bool)statusCode.mid(0,1).toInt());
                    thing->setLinkStatusOK(!(bool)statusCode.mid(1,1).toInt());
                    thing->setPanelLocked((bool)statusCode.mid(2,1).toInt());
                    thing->setGatewayKnown((bool)statusCode.mid(3,1).toInt());
                    thing->setDtsActive((bool)statusCode.mid(4,1).toInt());
                    thing->setDeviceMode(statusCode.right(2).toInt(0,2));

                    // calculate current temperature and setpoint temperature
                    QByteArray tempCode = fillBin(QByteArray::number(rawData.mid(14,2).toInt(0,16),2),8);
                    thing->setSetpointTemperatre((double)tempCode.right(6).toInt(0,2) / 2.0);
                    if(tempCode.left(2) == "10"){
                        thing->setCurrentTemperatre(((double)rawData.right(2).toInt(0,16) / 10.0) + 25.6);
                    }else{
                        thing->setCurrentTemperatre((double)rawData.right(2).toInt(0,16) / 10.0);
                    }

                    qCDebug(dcEQ3) << "                raw data | " << rawData;
                    qCDebug(dcEQ3) << "             thing type | " << thing->deviceTypeString();
                    qCDebug(dcEQ3) << "             thing name | " << thing->deviceName();
                    qCDebug(dcEQ3) << "        RF address (hex) | " << thing->rfAddress();
                    qCDebug(dcEQ3) << "                initCode | " << initCode;
                    qCDebug(dcEQ3) << "       information valid | " << thing->informationValid();
                    qCDebug(dcEQ3) << "          error occurred | " << thing->errorOccurred();
                    qCDebug(dcEQ3) << " is answere to a command | " << thing->isAnswereToCommand();
                    qCDebug(dcEQ3) << "             initialized | " << thing->initialized();
                    qCDebug(dcEQ3) << "             battery low | " << thing->batteryLow();
                    qCDebug(dcEQ3) << "          link status OK | " << thing->linkStatusOK();
                    qCDebug(dcEQ3) << "            panel locked | " << thing->panelLocked();
                    qCDebug(dcEQ3) << "           gateway known | " << thing->gatewayKnown();
                    qCDebug(dcEQ3) << "     DST settings active | " << thing->dtsActive();
                    qCDebug(dcEQ3) << "             thing mode | " << thing->deviceModeString();
                    qCDebug(dcEQ3) << "     Temperatur Setpoint | " << thing->setpointTemperature();
                    qCDebug(dcEQ3) << "            Current Temp | " << thing->currentTemperature();
                    qCDebug(dcEQ3) << "-------------------------|-------------------------";

                    emit wallThermostatDataUpdated();
                }
            }
            break;
        }
        case MaxDevice::DeviceRadiatorThermostat:{
            foreach (RadiatorThermostat* thing, m_radiatorThermostatList) {
                if(thing->rfAddress() == rfAddress){

                    QByteArray initCode = fillBin(QByteArray::number(rawData.mid(8,2).toInt(0,16),2),8);
                    thing->setInformationValid((bool)initCode.mid(3,1).toInt());
                    thing->setErrorOccurred((bool)initCode.mid(4,1).toInt());
                    thing->setIsAnswereToCommand((bool)initCode.mid(5,1).toInt());
                    thing->setInitialized((bool)initCode.mid(6,1).toInt());

                    QByteArray statusCode = fillBin(QByteArray::number(rawData.mid(10,2).toInt(0,16),2),8);
                    thing->setBatteryLow((bool)statusCode.mid(0,1).toInt());
                    thing->setLinkStatusOK(!(bool)statusCode.mid(1,1).toInt());
                    thing->setPanelLocked((bool)statusCode.mid(2,1).toInt());
                    thing->setGatewayKnown((bool)statusCode.mid(3,1).toInt());
                    thing->setDtsActive((bool)statusCode.mid(4,1).toInt());
                    thing->setDeviceMode(statusCode.right(2).toInt(0,2));

                    thing->setValvePosition((double)rawData.mid(12,2).toInt(0,16));
                    thing->setSetpointTemperatre((double)rawData.mid(14,2).toInt(0,16)/ 2.0);

                    qCDebug(dcEQ3) << "             thing type | " << thing->deviceTypeString();
                    qCDebug(dcEQ3) << "             thing name | " << thing->deviceName();
                    qCDebug(dcEQ3) << "        RF address (hex) | " << thing->rfAddress();
                    qCDebug(dcEQ3) << "       information valid | " << thing->informationValid();
                    qCDebug(dcEQ3) << "          error occurred | " << thing->errorOccurred();
                    qCDebug(dcEQ3) << " is answere to a command | " << thing->isAnswereToCommand();
                    qCDebug(dcEQ3) << "             initialized | " << thing->initialized();
                    qCDebug(dcEQ3) << "             battery low | " << thing->batteryLow();
                    qCDebug(dcEQ3) << "          link status OK | " << thing->linkStatusOK();
                    qCDebug(dcEQ3) << "            panel locked | " << thing->panelLocked();
                    qCDebug(dcEQ3) << "           gateway known | " << thing->gatewayKnown();
                    qCDebug(dcEQ3) << "     DST settings active | " << thing->dtsActive();
                    qCDebug(dcEQ3) << "             thing mode | " << thing->deviceModeString();
                    qCDebug(dcEQ3) << "          valve position | " << thing->valvePosition() << "%";
                    qCDebug(dcEQ3) << "     Temperatur Setpoint | " << thing->setpointTemperature() << " deg C";
                    qCDebug(dcEQ3) << "-------------------------|-------------------------";

                    emit radiatorThermostatDataUpdated();
                }
            }
            break;
        }
        case MaxDevice::DeviceWindowContact:{
            //QByteArray windowOpenRawData = fillBin(QByteArray::number(rawData.mid(10,2).toInt(0,16),2),8);
            //bool windowOpen = (bool)windowOpenRawData.mid(5,1).toInt();

            //            qCDebug(dcEQ3) << "                raw data | " << rawData;
            //            qCDebug(dcEQ3) << "        thing type name | " << "Window Contact";
            //            qCDebug(dcEQ3) << "        RF address (hex) | " << rfAddress;
            //            qCDebug(dcEQ3) << "        window open code | " << windowOpenRawData;
            //            qCDebug(dcEQ3) << "             window open | " << windowOpen;
            //            qCDebug(dcEQ3) << "-------------------------|-------------------------";
            break;
        }
        default:
            break;
        }
    }
}

void MaxCube::decodeCommandMessage(QByteArray data)
{
    QList<QByteArray> list = data.split(',');

    if(list.isEmpty()){
        return;
    }
    bool succeeded = !(bool)list.at(2).toInt(0,10);

    emit commandActionFinished(succeeded, m_pendingCommand.commandId);

    m_pendingCommand.commandId = -1;
    processCommandQueue();
}

void MaxCube::parseWeeklyProgram(QByteArray data)
{
    for(int i=0; i < 7; i++){
        QByteArray dayData = data.left(52);
        //qCDebug(dcEQ3) << weekDayString(i);
        for(int i = 0; i < 52; i+=4){
            QByteArray element = fillBin(QByteArray::number(dayData.mid(i,4).toInt(0,16),2),16);
            //int minutes = element.right(9).toInt(0,2) * 5;
            //int hours = (minutes / 60) % 24;
            //minutes = minutes % 60;
            //QTime time = QTime(hours,minutes);
            //qCDebug(dcEQ3) << (double)element.left(7).toInt(0,2) / 2 << "\t" << "deg. until" << "\t" << time.toString("HH:mm");
        }
        data = data.right(data.length() - 52);
    }
    if(!data.isEmpty()){
        //qCDebug(dcEQ3) << "                       ? | " << data;
    }
}

void MaxCube::decodeNewDeviceFoundMessage(QByteArray data)
{
    if(data.isEmpty()){
        return;
    }

    qCDebug(dcEQ3) << "====================================================";
    qCDebug(dcEQ3) << "               NEW DEVICE message:";
    qCDebug(dcEQ3) << "====================================================";
    qCDebug(dcEQ3) << "           Serial Number | " << QByteArray::fromBase64(data);

}

QDateTime MaxCube::calculateDateTime(QByteArray dateRaw, QByteArray timeRaw)
{
    QDate date;
    QTime time;
    date.setDate(dateRaw.left(2).toInt(0,16) + 2000, dateRaw.mid(2,2).toInt(0,16), dateRaw.right(2).toInt(0,16));
    time.setHMS(timeRaw.left(2).toInt(0,16), timeRaw.right(2).toInt(0,16), 0);

    return QDateTime(date,time);
}

QString MaxCube::deviceTypeString(int deviceType)
{
    QString deviceTypeString;

    switch (deviceType) {
    case MaxDevice::DeviceCube:
        deviceTypeString = "Cube";
        break;
    case MaxDevice::DeviceRadiatorThermostat:
        deviceTypeString = "Radiator Thermostat";
        break;
    case MaxDevice::DeviceRadiatorThermostatPlus:
        deviceTypeString = "Radiator Thermostat Plus";
        break;
    case MaxDevice::DeviceEcoButton:
        deviceTypeString = "Eco Button";
        break;
    case MaxDevice::DeviceWindowContact:
        deviceTypeString = "Window Contact";
        break;
    case MaxDevice::DeviceWallThermostat:
        deviceTypeString = "Wall Thermostat";
        break;
    default:
        deviceTypeString = "-";
        break;
    }

    return deviceTypeString;
}

QString MaxCube::weekDayString(int weekDay)
{
    QString weekDayString;

    switch (weekDay) {
    case Monday:
        weekDayString = "Monday";
        break;
    case Tuesday:
        weekDayString = "Tuesday";
        break;
    case Wednesday:
        weekDayString = "Wednesday";
        break;
    case Thursday:
        weekDayString = "Thursday";
        break;
    case Friday:
        weekDayString = "Friday";
        break;
    case Saturday:
        weekDayString = "Saturday";
        break;
    case Sunday:
        weekDayString = "Sunday";
        break;
    default:
        weekDayString = "-";
        break;
    }

    return weekDayString;
}

QByteArray MaxCube::fillBin(QByteArray data, int dataLength)
{
    QByteArray zeros;
    for(int i = 0; i < dataLength - data.length(); i++){
        zeros.append("0");
    }
    data = zeros.append(data);
    return data;
}

QList<QByteArray> MaxCube::splitMessage(QByteArray data)
{
    QList<QByteArray> messageList;
    while(!data.isEmpty()){
        int length = data.left(2).toInt(0,16)*2;
        messageList.append(data.mid(2,length));
        data = data.right(data.length() - (length+2));
    }
    //qCDebug(dcEQ3) << messageList;
    return messageList;
}

int MaxCube::deviceTypeFromRFAddress(QByteArray rfAddress)
{
    foreach (WallThermostat* thing, m_wallThermostatList) {
        if(thing->rfAddress() == rfAddress){
            return thing->deviceType();
        }
    }

    foreach (RadiatorThermostat* thing, m_radiatorThermostatList) {
        if(thing->rfAddress() == rfAddress){
            return thing->deviceType();
        }
    }
    return -1;
}

quint8 MaxCube::generateCommandId()
{
    static quint8 cmd = 0;
    return cmd++;
}

void MaxCube::connectionStateChanged(QAbstractSocket::SocketState socketState)
{
    switch (socketState) {
    case QAbstractSocket::ConnectedState:
        qCDebug(dcEQ3) << "connected to cube " << m_serialNumber << m_hostAddress.toString();
        emit cubeConnectionStatusChanged(true);
        break;
    case QAbstractSocket::UnconnectedState:
        m_cubeInitialized = false;
        qCDebug(dcEQ3) << "disconnected from cube " << m_serialNumber << m_hostAddress.toString();
        emit cubeConnectionStatusChanged(false);
        break;
    default:
        break;
    }
}

void MaxCube::onTcpError(QAbstractSocket::SocketError error)
{
    qCWarning(dcEQ3) << "connection error (" << m_serialNumber << "): " << error;
    emit cubeConnectionStatusChanged(false);
}

void MaxCube::onReadyRead()
{
    QByteArray message;
    while(canReadLine()){
        QByteArray dataLine = readLine();
        message.append(dataLine);
    }
    emit cubeDataAvailable(message);
}

void MaxCube::processCubeData(const QByteArray &data)
{
    //qCDebug(dcEQ3) << "data" << data;
    if(data.startsWith("H")){
        decodeHelloMessage(data.right(data.length() -2 ));
        return;
    }
    // METADATA message
    if(data.startsWith("M")){
        decodeMetadataMessage(data.right(data.length() -2 ));
        return;
    }
    // CONFIG message
    if(data.startsWith("C")){
        QList<QByteArray> dataList = data.split('\r');
        foreach (QByteArray dataElement, dataList) {
            if(dataElement.startsWith("C")){
                decodeConfigMessage(dataElement.right(dataElement.length() -2 ));
            }
            if(dataElement.startsWith("\nC")){
                decodeConfigMessage(dataElement.right(dataElement.length() -3 ));
            }
        }
        return;
    }
    // LIVE message
    if(data.startsWith("L")){
        decodeDevicelistMessage(data.right(data.length() -2 ));
        return;
    }
    // NEWDEVICEFOUND message
    if(data.startsWith("N")){
        decodeNewDeviceFoundMessage(data.right(data.length() -2));
        return;
    }
    // COMMAND answere message
    if(data.startsWith("S")){
        decodeCommandMessage(data.right(data.length() -2));
        return;
    }
    // ACK message
    if(data.startsWith("A")){
        qCDebug(dcEQ3) << "cube ACK!";
        emit cubeACK();
        return;
    }
    qCWarning(dcEQ3) << "  -> unknown message!!!!!!! from cube:" << data;
}

void MaxCube::processCommandQueue()
{
    if (m_commandQueue.isEmpty()) {
        return; // Queue empty
    }

    if (m_pendingCommand.commandId == -1) {
        return; // Busy...
    }

    m_pendingCommand = m_commandQueue.takeFirst();

    write(m_pendingCommand.data);
}

void MaxCube::enablePairingMode()
{
    qCDebug(dcEQ3) << "-------> enable pairing mode! press the boost button for min. 3 seconds";
    write("n:003c\r\n");
}

void MaxCube::disablePairingMode()
{
    qCDebug(dcEQ3) << " ----> disable pairing mode!";
    write("x:\r\n");
}

void MaxCube::refresh()
{
    if(!isInitialized() || !isConnected()){
        return;
    }
    write("l:\r\n");
}

void MaxCube::customRequest(QByteArray data)
{
    qCDebug(dcEQ3) << " ----> custom request" << data;
    write(data + "\r\n");
}

int MaxCube::setDeviceSetpointTemp(QByteArray rfAddress, int roomId, double temperature)
{
    if(!isConnected() || !isInitialized()){
        return -1;
    }

    QByteArray data = "000440000000";
    data.append(rfAddress);

    // if roomID = 0....means all rooms
    data.append(fillBin(QByteArray::number(roomId,16),2));

    QByteArray temperatureData;

    //temperature in 6 bits
    temperatureData = fillBin(QByteArray::number((int)temperature*2,2),6);

    // set auto/ permanent/ temp
    // 00 = auto (weekly program...the hole tempererature byte to 0x00
    // 01 = Permanent
    // 10 = Temporary (date/time has to be set)


    data.append(fillBin(QByteArray::number(temperatureData.toInt(0,2),16),2));
    temperatureData.append("01");

    // add date/time until (000000 = forever)
    data.append("000000");

    qCDebug(dcEQ3) << "sending command " << temperatureData << data;

    Command command;
    command.commandId = generateCommandId();
    command.data = "s:" + QByteArray::fromHex(data).toBase64() + "\r\n";
    m_commandQueue.append(command);

    processCommandQueue();

    return command.commandId;
}

int MaxCube::setDeviceAutoMode(QByteArray rfAddress, int roomId)
{
    if(!isConnected() || !isInitialized()){
        return -1;
    }

    QByteArray data = "000440000000";
    data.append(rfAddress);

    // if roomID = 0....means all rooms
    data.append(fillBin(QByteArray::number(roomId,16),2));

    QByteArray temperatureData;

    temperatureData.append("00000000");
    data.append("000000");

    qCDebug(dcEQ3) << "sending command " << temperatureData << data;

    Command command;
    command.commandId = generateCommandId();
    command.data = "s:" + QByteArray::fromHex(data).toBase64() + "\r\n";
    m_commandQueue.append(command);
    processCommandQueue();
    return command.commandId;
}

int MaxCube::setDeviceManuelMode(QByteArray rfAddress, int roomId)
{
    if(!isConnected() || !isInitialized()){
        return -1;
    }

    QByteArray data = "000440000000";
    data.append(rfAddress);

    // if roomID = 0....means all rooms
    data.append(fillBin(QByteArray::number(roomId,16),2));
    data.append("62");

    Command command;
    command.commandId = generateCommandId();
    command.data = "s:" + QByteArray::fromHex(data).toBase64() + "\r\n";
    m_commandQueue.append(command);
    processCommandQueue();
    return command.commandId;
}

int MaxCube::setDeviceEcoMode(QByteArray rfAddress, int roomId)
{
    if(!isConnected() || !isInitialized()){
        return -1;
    }

    QByteArray data = "000440000000";
    data.append(rfAddress);

    // if roomID = 0....means all rooms
    data.append(fillBin(QByteArray::number(roomId,16),2));
    data.append("6b");

    Command command;
    command.commandId = generateCommandId();
    command.data = "s:" + QByteArray::fromHex(data).toBase64() + "\r\n";
    m_commandQueue.append(command);
    processCommandQueue();
    return command.commandId;
}

int MaxCube::displayCurrentTemperature(QByteArray rfAddress, int roomId, bool display)
{
    Q_UNUSED(roomId)
    if(!isConnected() || !isInitialized()){
        return -1;
    }

    QByteArray data = "000082000000";
    data.append(rfAddress);

    if(display){
        data.append("0004");
    }else{
        data.append("0000");
    }

    Command command;
    command.commandId = generateCommandId();
    command.data = "s:" + QByteArray::fromHex(data).toBase64() + "\r\n";
    m_commandQueue.append(command);
    processCommandQueue();
    return command.commandId;
}

