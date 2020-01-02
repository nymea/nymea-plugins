/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  Copyright 2013 - 2020, nymea GmbH
*  Contact: contact@nymea.io
*
*  This file is part of nymea.
*  This project including source code and documentation is protected by copyright law, and
*  remains the property of nymea GmbH. All rights, including reproduction, publication,
*  editing and translation, are reserved. The use of this project is subject to the terms of a
*  license agreement to be concluded with nymea GmbH in accordance with the terms
*  of use of nymea GmbH, available under https://nymea.io/license
*
*  GNU Lesser General Public License Usage
*  This project may also contain libraries licensed under the open source software license GNU GPL v.3.
*  Alternatively, this project may be redistributed and/or modified under the terms of the GNU
*  Lesser General Public License as published by the Free Software Foundation; version 3.
*  this project is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
*  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*  See the GNU Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public License along with this project.
*  If not, see <https://www.gnu.org/licenses/>.
*
*  For any further details and any questions please contact us under contact@nymea.io
*  or see our FAQ/Licensing Information on https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
