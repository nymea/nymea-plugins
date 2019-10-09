/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016-2018 Simon Stürz <simon.stuerz@guh.io>              *
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

#ifndef NUIMO_H
#define NUIMO_H

#include <QObject>
#include <QTimer>
#include <QBluetoothUuid>

#include "typeutils.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"

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

    explicit LukeRoberts(BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent = nullptr);

    BluetoothLowEnergyDevice *bluetoothDevice();

private:
    BluetoothLowEnergyDevice *m_bluetoothDevice = nullptr;

    QLowEnergyService *m_deviceInfoService = nullptr;
    QLowEnergyService *m_controlService = nullptr;      //Luke Roberts Custom control service
    QLowEnergyService *m_dfuService = nullptr;

    QLowEnergyCharacteristic m_deviceInfoCharacteristic;
    QLowEnergyCharacteristic m_externalApiEndpoint;

    void ping();
    void queryScene(int id);
    void immediateLight(int flags, int duration);
    void brightness(int percent);
    void colorTemperature(int kelvin);
    void selectScene(int id);
    void nextSceneByBrightness();
    void adjustColorTemperature();
    void relativeBrightness(int percent);

    void printService(QLowEnergyService *service);
    void onLongPressTimer();

signals:
    void connectedChanged(bool connected);
    void deviceInformationChanged(const QString &firmwareRevision, const QString &hardwareRevision, const QString &softwareRevision);
    void deviceInitializationFinished(bool success);
    void statusCodeReveiced(StatusCodes statusCode);

private slots:
    void onConnectedChanged(bool connected);
    void onServiceDiscoveryFinished();

    void onDeviceInfoServiceStateChanged(const QLowEnergyService::ServiceState &state);

    void onBatteryServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onBatteryCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    void onInputServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onInputCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
};

#endif // NUIMO_H
