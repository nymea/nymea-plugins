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

#include "bluetoothgattservice.h"

QString BluetoothGattService::serviceName() const
{
    bool ok = false;
    quint16 typeId = m_uuid.toUInt16(&ok);

    if (ok) {
        QBluetoothUuid::ServiceClassUuid uuid = static_cast<QBluetoothUuid::ServiceClassUuid>(typeId);
        const QString name = QBluetoothUuid::serviceClassToString(uuid);
        if (!name.isEmpty())
            return name;
    }

    return QString("Unknown Service");
}

BluetoothGattService::Type BluetoothGattService::type() const
{
    return m_type;
}

QBluetoothUuid BluetoothGattService::uuid() const
{
    return m_uuid;
}

QList<BluetoothGattCharacteristic *> BluetoothGattService::characteristics() const
{
    return m_characteristics;
}

bool BluetoothGattService::hasCharacteristic(const QBluetoothUuid &characteristicUuid)
{
    foreach (BluetoothGattCharacteristic *characteristic, m_characteristics) {
        if (characteristic->uuid() == characteristicUuid) {
            return true;
        }
    }

    return false;
}

BluetoothGattCharacteristic *BluetoothGattService::getCharacteristic(const QBluetoothUuid &characteristicUuid)
{
    foreach (BluetoothGattCharacteristic *characteristic, m_characteristics) {
        if (characteristic->uuid() == characteristicUuid) {
            return characteristic;
        }
    }

    return nullptr;
}

BluetoothGattService::BluetoothGattService(const QDBusObjectPath &path, const QVariantMap &properties, QObject *parent) :
    QObject(parent),
    m_path(path),
    m_type(Primary),
    m_discovered(false)
{
    processProperties(properties);
}

void BluetoothGattService::processProperties(const QVariantMap &properties)
{
    foreach (const QString &propertyName, properties.keys()) {
        if (propertyName == "Primary") {
            m_type = (properties.value(propertyName).toBool() ? Primary : Secondary);
        } else if (propertyName == "UUID") {
            m_uuid = QBluetoothUuid(properties.value(propertyName).toString());
        }
    }
}

void BluetoothGattService::addCharacteristicInternally(const QDBusObjectPath &path, const QVariantMap &properties)
{
    if (hasCharacteristic(path))
        return;

    BluetoothGattCharacteristic *characteristic = new BluetoothGattCharacteristic(path, properties, this);
    m_characteristics.append(characteristic);

    connect(characteristic, &BluetoothGattCharacteristic::readingFinished, this, &BluetoothGattService::onCharacteristicReadFinished);
    connect(characteristic, &BluetoothGattCharacteristic::writingFinished, this, &BluetoothGattService::onCharacteristicReadFinished);
    connect(characteristic, &BluetoothGattCharacteristic::valueChanged, this, &BluetoothGattService::onCharacteristicValueChanged);

    qCDebug(dcBluez()) << "[+]" << characteristic;
}

bool BluetoothGattService::hasCharacteristic(const QDBusObjectPath &path)
{
    foreach (BluetoothGattCharacteristic *characteristic, m_characteristics) {
        if (characteristic->m_path == path) {
            return true;
        }
    }

    return false;
}

BluetoothGattCharacteristic *BluetoothGattService::getCharacteristic(const QDBusObjectPath &path)
{
    foreach (BluetoothGattCharacteristic *characteristic, m_characteristics) {
        if (characteristic->m_path == path) {
            return characteristic;
        }
    }

    return nullptr;
}

void BluetoothGattService::onCharacteristicReadFinished(const QByteArray &value)
{
    BluetoothGattCharacteristic *characteristic = static_cast<BluetoothGattCharacteristic *>(sender());
    emit characteristicReadFinished(characteristic, value);
}

void BluetoothGattService::onCharacteristicWriteFinished(const QByteArray &value)
{
    BluetoothGattCharacteristic *characteristic = static_cast<BluetoothGattCharacteristic *>(sender());
    emit characteristicWriteFinished(characteristic, value);
}

void BluetoothGattService::onCharacteristicValueChanged(const QByteArray &newValue)
{
    BluetoothGattCharacteristic *characteristic = static_cast<BluetoothGattCharacteristic *>(sender());
    emit characteristicChanged(characteristic, newValue);
}

bool BluetoothGattService::readCharacteristic(const QBluetoothUuid &characteristicUuid)
{
    if (!hasCharacteristic(characteristicUuid))
        return false;

    return getCharacteristic(characteristicUuid)->readCharacteristic();
}

QDebug operator<<(QDebug debug, BluetoothGattService *service)
{
    debug.noquote().nospace() << "GattService(" << (service->type() == BluetoothGattService::Primary ? "Primary" : "Secondary");
    debug.noquote().nospace() << ", " << service->serviceName();
    debug.noquote().nospace() << ", " << service->uuid().toString();
    debug.noquote().nospace() << ") ";

    return debug;
}
