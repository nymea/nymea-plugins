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

#ifndef FLOWERCARE_H
#define FLOWERCARE_H

#include <QObject>

#include <hardware/bluetoothlowenergy/bluetoothlowenergydevice.h>

static QBluetoothUuid sensorServiceUuid                      = QBluetoothUuid(QUuid("00001204-0000-1000-8000-00805f9b34fb"));

// Contains Battery level and firmware version
static QBluetoothUuid batteryFirmwareCharacteristicUuid      = QBluetoothUuid(QUuid("00001a02-0000-1000-8000-00805f9b34fb"));

// Need to write 0xA01F to this
static QBluetoothUuid sensorControlCharacteristicUuid        = QBluetoothUuid(QUuid("00001a00-0000-1000-8000-00805f9b34fb"));

// contains sensor data
static QBluetoothUuid sensorDataCharacteristicUuid           = QBluetoothUuid(QUuid("00001a01-0000-1000-8000-00805f9b34fb"));


class FlowerCare : public QObject
{
    Q_OBJECT
public:
    explicit FlowerCare(BluetoothLowEnergyDevice* thing, QObject *parent = nullptr);

    void refreshData();

    BluetoothLowEnergyDevice* btDevice() const;

signals:
    void finished(quint8 batteryLevel, double degreeCelsius, double lux, double moisture, double fertility);
    void failed();

private slots:
    void onConnectedChanged(bool connected);
    void onServiceDiscoveryFinished();

    void onSensorServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onSensorServiceCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void onSensorServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

private:
    void printServiceDetails(QLowEnergyService* service) const;

    void processSensorData(const QByteArray &data);

    BluetoothLowEnergyDevice *m_bluetoothDevice;

    // Services
    QLowEnergyService *m_sensorService = nullptr;
    QLowEnergyCharacteristic m_sensorDataCharacteristic;

    // cache
    quint8 m_batteryLevel = 0;

};

#endif // FLOWERCARE_H
