/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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
