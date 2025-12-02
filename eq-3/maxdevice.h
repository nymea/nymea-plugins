// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef MAXDEVICE_H
#define MAXDEVICE_H

#include <QObject>

class MaxDevice : public QObject
{
    Q_OBJECT
public:
    explicit MaxDevice(QObject *parent = 0);

    enum MaxDeviceType{
        DeviceCube = 0,
        DeviceRadiatorThermostat = 1,
        DeviceRadiatorThermostatPlus = 2,
        DeviceWallThermostat = 3,
        DeviceWindowContact = 4,
        DeviceEcoButton = 5
    };

    enum DeviceMode{
        Auto = 0,
        Manual = 1,
        Temporary = 2,
        Boost = 3
    };

    int deviceType() const;
    void setDeviceType(const int &deviceType);

    QString deviceTypeString() const;

    QByteArray rfAddress() const;
    void setRfAddress(const QByteArray &rfAddress);

    QString serialNumber() const;
    void setSerialNumber(const QString &serialNumber);

    QString deviceName() const;
    void setDeviceName(const QString &deviceName);

    int roomId() const;
    void setRoomId(const int &roomId);

    QString roomName() const;
    void setRoomName(const QString &roomName);

private:
    int m_deviceType;
    QString m_deviceTypeString;
    QByteArray m_rfAddress;
    QString m_serialNumber;
    QString m_deviceName;
    int m_roomId;
    QString m_roomName;
    bool m_batteryOk;

};

#endif // MAXDEVICE_H
