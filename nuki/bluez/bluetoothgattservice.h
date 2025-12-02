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

#ifndef BLUETOOTHGATTSERVICE_H
#define BLUETOOTHGATTSERVICE_H

#include <QObject>
#include <QBluetoothUuid>

#include "blueztypes.h"
#include "bluetoothgattcharacteristic.h"

// Note: DBus documentation https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/gatt-api.txt

class BluetoothManager;
class BluetoothDevice;

class BluetoothGattService : public QObject
{
    Q_OBJECT

    friend class BluetoothManager;
    friend class BluetoothDevice;

public:
    enum Type {
        Primary,
        Secondary
    };
    Q_ENUM(Type)

    QString serviceName() const;
    Type type() const;
    QBluetoothUuid uuid() const;

    // Characteristic methods
    QList<BluetoothGattCharacteristic *> characteristics() const;
    bool hasCharacteristic(const QBluetoothUuid &characteristicUuid);
    BluetoothGattCharacteristic *getCharacteristic(const QBluetoothUuid &characteristicUuid);

private:
    explicit BluetoothGattService(const QDBusObjectPath &path, const QVariantMap &properties, QObject *parent = 0);

    QDBusObjectPath m_path;

    Type m_type;
    QBluetoothUuid m_uuid;
    QList<BluetoothGattCharacteristic *> m_characteristics;

    bool m_discovered;

    void processProperties(const QVariantMap &properties);

    // Methods called from BluetoothManager
    void addCharacteristicInternally(const QDBusObjectPath &path, const QVariantMap &properties);
    bool hasCharacteristic(const QDBusObjectPath &path);
    BluetoothGattCharacteristic *getCharacteristic(const QDBusObjectPath &path);

private slots:
    void onCharacteristicReadFinished(const QByteArray &value);
    void onCharacteristicWriteFinished(const QByteArray &value);
    void onCharacteristicValueChanged(const QByteArray &newValue);

signals:
    void characteristicReadFinished(BluetoothGattCharacteristic *characteristic, const QByteArray &value);
    void characteristicWriteFinished(BluetoothGattCharacteristic *characteristic, const QByteArray &value);
    void characteristicChanged(BluetoothGattCharacteristic *characteristic, const QByteArray &newValue);

public slots:
    bool readCharacteristic(const QBluetoothUuid &characteristicUuid);

};

QDebug operator<<(QDebug debug, BluetoothGattService *service);


#endif // BLUETOOTHGATTSERVICE_H
