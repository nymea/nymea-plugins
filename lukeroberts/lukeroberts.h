/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinens <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef LUKEROBERTS_H
#define LUKEROBERTS_H

#include <QObject>
#include <QTimer>
#include <QBluetoothUuid>
#include <QByteArray>

#include "typeutils.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"


inline QByteArray &operator<<(QByteArray &l, quint8 r)
{
    l.append(r);
    return l;
}

inline QByteArray &operator<<(QByteArray &l, quint16 r)
{
    return l<<quint8(r>>8)<<quint8(r);
}

inline QByteArray &operator<<(QByteArray &l, quint32 r)
{
    return l<<quint16(r>>16)<<quint16(r);
}

class LukeRoberts : public QObject
{
    Q_OBJECT
public:
enum StatusCodes {
    StatusCodeSuccess           = 0x00,
    StatusCodeInvalidParameters = 0x81,
    StatusCodeInvalidId         = 0x84,
    StatusCodeInvalidVersion    = 0x87,
    StatusCodeBadCommand        = 0xbc,
    StatusCodeForbidden         = 0xfc,
    StatusCodeFilteredDuplicate = 0x88
};

enum Opcode {
    OpcodePing,
    OpcodeQueryScene,
    OpcodeImmediateLight,
    OpcodeDuration,
    OpcodeBrightness,
    OpcodeColorTemperature,
    OpcodeSelectScene,
    OpcodeNextSceneByBrightness,
    OpcodeAdjustColorTemperature,
    OpcodeRelativeBrightness
};

struct Scene {
    int8_t id;
    QString name;
};
    explicit LukeRoberts(BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent = nullptr);

    BluetoothLowEnergyDevice *bluetoothDevice();
    void ping();
    void queryScene(uint8_t id);
    void getSceneList();
    void setImmediateLight(uint16_t duration, uint8_t saturation, uint16_t hue, uint8_t brightness);
    void modifyBrightness(uint8_t percent);
    void modifyColorTemperature(uint16_t kelvin);
    void selectScene(uint8_t id);
    void setNextSceneByBrightness(int8_t direction);
    void adjustColorTemperature(uint16_t kelvinIncrement);
    void setRelativeBrightness(uint8_t percent);


private:
    BluetoothLowEnergyDevice *m_bluetoothDevice = nullptr;

    QLowEnergyService *m_deviceInfoService = nullptr;
    QLowEnergyService *m_controlService = nullptr;      //Luke Roberts Custom control service
    QLowEnergyService *m_dfuService = nullptr;

    QLowEnergyCharacteristic m_deviceInfoCharacteristic;
    QLowEnergyCharacteristic m_externalApiEndpoint;

    QList<Scene> m_sceneList;

    void printService(QLowEnergyService *service);
    void onLongPressTimer();

signals:
    void connectedChanged(bool connected);
    void deviceInformationChanged(const QString &firmwareRevision, const QString &hardwareRevision, const QString &softwareRevision);
    void deviceInitializationFinished(bool success);
    void statusCodeReveiced(StatusCodes statusCode);
    void sceneListReceived(QList<Scene> scenes);

private slots:
    void onConnectedChanged(bool connected);
    void onServiceDiscoveryFinished();

    void onDeviceInfoServiceStateChanged(const QLowEnergyService::ServiceState &state);

    void onControlServiceChanged(const QLowEnergyService::ServiceState &state);
    void onExternalApiEndpointCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
};

#endif // LUKEROBERTS_H
