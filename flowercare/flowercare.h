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

#ifndef FLOWERCARE_H
#define FLOWERCARE_H

#include <QObject>

#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"

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
    explicit FlowerCare(BluetoothLowEnergyDevice* device, QObject *parent = nullptr);

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
