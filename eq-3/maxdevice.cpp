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

#include "maxdevice.h"
#include "extern-plugininfo.h"

MaxDevice::MaxDevice(QObject *parent) :
    QObject(parent)
{
}

int MaxDevice::deviceType() const
{
    return m_deviceType;
}

void MaxDevice::setDeviceType(const int &deviceType)
{
    m_deviceType = deviceType;

    switch (m_deviceType) {
    case DeviceCube:
        m_deviceTypeString = "Cube";
        break;
    case DeviceRadiatorThermostat:
        m_deviceTypeString = "Radiator Thermostat";
        break;
    case DeviceRadiatorThermostatPlus:
        m_deviceTypeString = "Radiator Thermostat Plus";
        break;
    case DeviceEcoButton:
        m_deviceTypeString = "Eco Button";
        break;
    case DeviceWindowContact:
        m_deviceTypeString = "Window Contact";
        break;
    case DeviceWallThermostat:
        m_deviceTypeString = "Wall Thermostat";
        break;
    default:
        m_deviceTypeString = "-";
        break;
    }
}

QString MaxDevice::deviceTypeString() const
{
    return m_deviceTypeString;
}

QByteArray MaxDevice::rfAddress() const
{
    return m_rfAddress;
}

void MaxDevice::setRfAddress(const QByteArray &rfAddress)
{
    m_rfAddress = rfAddress;
}

QString MaxDevice::serialNumber() const
{
    return m_serialNumber;
}

void MaxDevice::setSerialNumber(const QString &serialNumber)
{
    m_serialNumber = serialNumber;
}

QString MaxDevice::deviceName() const
{
    return m_deviceName;
}

void MaxDevice::setDeviceName(const QString &deviceName)
{
    m_deviceName = deviceName;
}

int MaxDevice::roomId() const
{
    return m_roomId;
}

void MaxDevice::setRoomId(const int &roomId)
{
    m_roomId = roomId;
}

QString MaxDevice::roomName() const
{
    return m_roomName;
}

void MaxDevice::setRoomName(const QString &roomName)
{
    m_roomName = roomName;
}

