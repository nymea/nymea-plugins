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

#include "bluetoothgattcharacteristic.h"

#include <QDBusReply>

QString BluetoothGattCharacteristic::chararcteristicName() const
{
    bool ok = false;
    quint16 typeId = m_uuid.toUInt16(&ok);

    if (ok) {
        QBluetoothUuid::CharacteristicType uuid = static_cast<QBluetoothUuid::CharacteristicType>(typeId);
        const QString name = QBluetoothUuid::characteristicToString(uuid);
        if (!name.isEmpty())
            return name;
    }

    return QString("Unknown Characteristic");
}

QBluetoothUuid BluetoothGattCharacteristic::uuid() const
{
    return m_uuid;
}

bool BluetoothGattCharacteristic::notifying() const
{
    return m_notifying;
}

BluetoothGattCharacteristic::Properties BluetoothGattCharacteristic::properties() const
{
    return m_properties;
}

QByteArray BluetoothGattCharacteristic::value() const
{
    return m_value;
}

QList<BluetoothGattDescriptor *> BluetoothGattCharacteristic::descriptors() const
{
    return m_descriptors;
}

BluetoothGattDescriptor *BluetoothGattCharacteristic::getDescriptor(const QBluetoothUuid &desciptorUuid) const
{
    foreach (BluetoothGattDescriptor *descriptor, m_descriptors) {
        if (descriptor->uuid() == desciptorUuid) {
            return descriptor;
        }
    }

    return nullptr;
}

BluetoothGattCharacteristic::BluetoothGattCharacteristic(const QDBusObjectPath &path, const QVariantMap &properties, QObject *parent) :
    QObject(parent),
    m_path(path),
    m_notifying(false)
{
    m_characteristicInterface = new QDBusInterface(orgBluez, m_path.path(), orgBluezGattCharacteristic1, QDBusConnection::systemBus(), this);
    if (!m_characteristicInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus characteristic interface for" << m_path.path();
        return;
    }

    QDBusConnection::systemBus().connect(orgBluez, m_path.path(), "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));

    processProperties(properties);
}

void BluetoothGattCharacteristic::processProperties(const QVariantMap &properties)
{
    foreach (const QString &propertyName, properties.keys()) {
        if (propertyName == "UUID") {
            m_uuid = QBluetoothUuid(properties.value(propertyName).toString());
        } else if (propertyName == "Notifying") {
            m_notifying = properties.value(propertyName).toBool();
            emit notifyingChanged(m_notifying);
        } else if (propertyName == "Flags") {
            m_properties = parsePropertyFlags(properties.value(propertyName).toStringList());
        } else if (propertyName == "Value") {
            m_value = properties.value(propertyName).toByteArray();
            emit valueChanged(m_value);
        }
    }
}

void BluetoothGattCharacteristic::addDescriptorInternally(const QDBusObjectPath &path, const QVariantMap &properties)
{
    if (hasDescriptor(path))
        return;

    BluetoothGattDescriptor *descriptor = new BluetoothGattDescriptor(path, properties, this);
    m_descriptors.append(descriptor);

    qCDebug(dcBluez()) << "[+]" << descriptor;
}

bool BluetoothGattCharacteristic::hasDescriptor(const QDBusObjectPath &path)
{
    foreach (BluetoothGattDescriptor *descriptor, m_descriptors) {
        if (descriptor->m_path == path) {
            return true;
        }
    }

    return false;
}

BluetoothGattDescriptor *BluetoothGattCharacteristic::getDescriptor(const QDBusObjectPath &path)
{
    foreach (BluetoothGattDescriptor *descriptor, m_descriptors) {
        if (descriptor->m_path == path) {
            return descriptor;
        }
    }

    return nullptr;
}

void BluetoothGattCharacteristic::setValueInternally(const QByteArray &value)
{
    if (m_value != value) {
        m_value = value;
        emit valueChanged(m_value);
    }
}

void BluetoothGattCharacteristic::setNotifyingInternally(const bool &notifying)
{
    if (m_notifying != notifying) {
        m_notifying = notifying;
        emit notifyingChanged(m_notifying);
    }
}

BluetoothGattCharacteristic::Properties BluetoothGattCharacteristic::parsePropertyFlags(const QStringList &characteristicProperties)
{
    Properties properties;

    foreach (const QString &propertyString, characteristicProperties) {
        if (propertyString == "broadcast") {
            properties |= Broadcasting;
        } else if (propertyString == "read") {
            properties |= Read;
        } else if (propertyString == "write-without-response") {
            properties |= WriteNoResponse;
        } else if (propertyString == "write") {
            properties |= Write;
        } else if (propertyString == "notify") {
            properties |= Notify;
        } else if (propertyString == "indicate") {
            properties |= Indicate;
        } else if (propertyString == "authenticated-signed-writes") {
            properties |= WriteAuthenticatedSigned;
        } else if (propertyString == "reliable-write") {
            properties |= ReliableWrite;
        } else if (propertyString == "writable-auxiliaries") {
            properties |= WritableAuxiliaries;
        } else if (propertyString == "encrypt-read") {
            properties |= EncryptRead;
        } else if (propertyString == "encrypt-write") {
            properties |= EncryptWrite;
        } else if (propertyString == "encrypt-authenticated-read") {
            properties |= EncryptAuthenticatedRead;
        } else if (propertyString == "encrypt-authenticated-write") {
            properties |= EncryptAuthenticatedWrite;
        } else if (propertyString == "secure-read") {
            properties |= SecureRead;
        } else if (propertyString == "secure-write") {
            properties |= Unknown;
        }
    }

    return properties;
}

void BluetoothGattCharacteristic::onPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    if (interface != orgBluezGattCharacteristic1)
        return;

    qCDebug(dcBluez()) << "BluetoothCharacteristic:" << m_uuid.toString() << "properties changed" << interface << changedProperties << invalidatedProperties;
    processProperties(changedProperties);
}

void BluetoothGattCharacteristic::onReadingFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QByteArray> reply = *call;
    if (reply.isError()) {
        qCWarning(dcBluez()) << "Could not read characteristic" << m_uuid.toString() << reply.error().name() << reply.error().message();
    } else {
        QByteArray value = reply.argumentAt<0>();
        qCDebug(dcBluez()) << "Async reading finished for" << m_uuid.toString() << value;
        setValueInternally(value);
        emit readingFinished(value);
    }

    call->deleteLater();
}

void BluetoothGattCharacteristic::onWritingFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    if (reply.isError()) {
        qCWarning(dcBluez()) << "Could not write characteristic" << m_uuid.toString() << reply.error().name() << reply.error().message();
    } else {
        QByteArray value = m_asyncWrites.take(call);
        qCDebug(dcBluez()) << "Async characteristic writing finished for" << m_uuid.toString() << value;
        emit writingFinished(value);
    }

    call->deleteLater();
}

void BluetoothGattCharacteristic::onStartNotificationFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    if (reply.isError())
        qCWarning(dcBluez()) << "Could not start notifications on characteristic" << m_uuid.toString() << reply.error().name() << reply.error().message();

    call->deleteLater();
}

void BluetoothGattCharacteristic::onStopNotificationFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    if (reply.isError())
        qCWarning(dcBluez()) << "Could not stop notifications on characteristic" << m_uuid.toString() << reply.error().name() << reply.error().message();

    call->deleteLater();
}

bool BluetoothGattCharacteristic::readCharacteristic()
{
    if (!m_characteristicInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus characteristic interface for" << m_path.path();
        return false;
    }

    QDBusPendingCall readingCall = m_characteristicInterface->asyncCall("ReadValue", QVariantMap());
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(readingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &BluetoothGattCharacteristic::onReadingFinished);
    return true;
}

bool BluetoothGattCharacteristic::writeCharacteristic(const QByteArray &value)
{
    if (!m_characteristicInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus characteristic interface for" << m_path.path();
        return false;
    }

    QDBusPendingCall writingCall = m_characteristicInterface->asyncCall("WriteValue", value, QVariantMap());
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(writingCall, this);
    m_asyncWrites.insert(watcher, value);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &BluetoothGattCharacteristic::onWritingFinished);
    return true;
}

bool BluetoothGattCharacteristic::startNotifications()
{
    if (!m_characteristicInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus characteristic interface for" << m_path.path();
        return false;
    }

    // If already notifying
    if (notifying())
        return true;

    QDBusPendingCall startNotifyCall = m_characteristicInterface->asyncCall("StartNotify");
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(startNotifyCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &BluetoothGattCharacteristic::onStartNotificationFinished);
    return true;
}

bool BluetoothGattCharacteristic::stopNotifications()
{
    if (!m_characteristicInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus characteristic interface for" << m_path.path();
        return false;
    }

    // If already stopped
    if (!notifying())
        return true;

    QDBusPendingCall stopNotifyCall = m_characteristicInterface->asyncCall("StopNotify");
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(stopNotifyCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &BluetoothGattCharacteristic::onStopNotificationFinished);
    return true;
}

QDebug operator<<(QDebug debug, BluetoothGattCharacteristic *characteristic)
{
    debug.noquote().nospace() << "GattCharacteristic(" << characteristic->chararcteristicName();
    debug.noquote().nospace() << ", " << characteristic->uuid().toString();
    debug.noquote().nospace() << ", Properties: " << characteristic->properties();

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::Unknown))
        debug.noquote().nospace() << " Unknown";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::Broadcasting))
        debug.noquote().nospace() << " B";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::Read))
        debug.noquote().nospace() << " R ";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::WriteNoResponse))
        debug.noquote().nospace() << " WNR";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::Write))
        debug.noquote().nospace() << " W";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::Notify))
        debug.noquote().nospace() << " N";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::Indicate))
        debug.noquote().nospace() << " I";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::WriteAuthenticatedSigned))
        debug.noquote().nospace() << " WAS";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::ReliableWrite))
        debug.noquote().nospace() << " RW";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::WritableAuxiliaries))
        debug.noquote().nospace() << " WA";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::EncryptWrite))
        debug.noquote().nospace() << " EW";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::EncryptRead))
        debug.noquote().nospace() << " ER";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::EncryptAuthenticatedRead))
        debug.noquote().nospace() << " EAR";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::EncryptAuthenticatedWrite))
        debug.noquote().nospace() << " EAW";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::SecureRead))
        debug.noquote().nospace() << " SR";

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::SecureWrite))
        debug.noquote().nospace() << " SW";

    // If this is a notifying characteristic, inform if notifications are enabled
    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::Notify))
        debug.noquote().nospace() << ", Notify: " << (characteristic->notifying() ? "ON" : "OFF");

    debug.noquote().nospace() << ", value:" << characteristic->value();
    debug.noquote().nospace() << ") ";
    return debug;
}
