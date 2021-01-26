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

#include "bluetoothgattdescriptor.h"

#include <QDBusReply>

QString BluetoothGattDescriptor::name() const
{
    bool ok = false;
    quint16 typeId = m_uuid.toUInt16(&ok);

    if (ok) {
        QBluetoothUuid::DescriptorType type = static_cast<QBluetoothUuid::DescriptorType>(typeId);
        const QString name = QBluetoothUuid::descriptorToString(type);
        if (!name.isEmpty())
            return name;
    }

    return QString("Unknown Descriptor");
}

QBluetoothUuid BluetoothGattDescriptor::uuid() const
{
    return m_uuid;
}

QByteArray BluetoothGattDescriptor::value() const
{
    return m_value;
}

BluetoothGattDescriptor::Properties BluetoothGattDescriptor::properties() const
{
    return m_properties;
}

BluetoothGattDescriptor::BluetoothGattDescriptor(const QDBusObjectPath &path, const QVariantMap &properties, QObject *parent) :
    QObject(parent),
    m_path(path)
{
    m_descriptorInterface = new QDBusInterface(orgBluez, m_path.path(), orgBluezGattDescriptor1, QDBusConnection::systemBus(), this);
    if (!m_descriptorInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus descriptor interface for" << m_path.path();
        return;
    }

    QDBusConnection::systemBus().connect(orgBluez, m_path.path(), "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));

    processProperties(properties);

//    QDBusPendingCall readingCall = m_descriptorInterface->asyncCall("GetAll", QVariantMap());
//    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(readingCall, this);
//    connect(watcher, &QDBusPendingCallWatcher::finished, this, [=](){
//        qCDebug(dcBluez()) << "Get descriptor properties finished";
//    });
}

void BluetoothGattDescriptor::processProperties(const QVariantMap &properties)
{
    qCDebug(dcBluez()) << "Descriptor properties" << properties;
    foreach (const QString &propertyName, properties.keys()) {
        if (propertyName == "UUID") {
            m_uuid = QBluetoothUuid(properties.value(propertyName).toString());
        } else if (propertyName == "Value") {
            setValueInternally(properties.value(propertyName).toByteArray());
        } else if (propertyName == "Flags") {
            m_properties = parsePropertyFlags(properties.value(propertyName).toStringList());
        }
    }
}

void BluetoothGattDescriptor::setValueInternally(const QByteArray &value)
{
    if (m_value != value) {
        m_value = value;
        emit valueChanged(m_value);
    }
}

BluetoothGattDescriptor::Properties BluetoothGattDescriptor::parsePropertyFlags(const QStringList &descriptorProperties)
{
    Properties properties;

    foreach (const QString &propertyString, descriptorProperties) {
        if (propertyString == "read") {
            properties |= Read;
        } else if (propertyString == "write") {
            properties |= Write;
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
            properties |= SecureWrite;
        }
    }

    return properties;
}

void BluetoothGattDescriptor::onPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    if (interface != orgBluezGattDescriptor1)
        return;

    qCDebug(dcBluez()) << "BluetoothDescriptor:" << m_uuid << "properties changed" << interface << changedProperties << invalidatedProperties;
    processProperties(changedProperties);
}

void BluetoothGattDescriptor::onReadingFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QByteArray> reply = *call;
    if (reply.isError()) {
        qCWarning(dcBluez()) << "Could not read descriptor" << m_uuid.toString() << reply.error().name() << reply.error().message();
    } else {
        QByteArray value = reply.argumentAt<0>();
        qCDebug(dcBluez()) << "Async descriptor reading finished for" << m_uuid.toString() << value;
        setValueInternally(value);
        emit readingFinished(value);
    }

    call->deleteLater();
}

void BluetoothGattDescriptor::onWritingFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    if (reply.isError()) {
        qCWarning(dcBluez()) << "Could not write descriptor" << m_uuid.toString() << reply.error().name() << reply.error().message();
    } else {
        QByteArray value = m_asyncWrites.take(call);
        qCDebug(dcBluez()) << "Async descriptor writing finished for" << m_uuid.toString() << value;
        emit writingFinished(value);
    }

    call->deleteLater();
}

bool BluetoothGattDescriptor::readValue()
{
    if (!m_descriptorInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus characteristic interface for" << m_path.path();
        return false;
    }

    QDBusPendingCall readingCall = m_descriptorInterface->asyncCall("ReadValue", QVariantMap());
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(readingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &BluetoothGattDescriptor::onReadingFinished);
    return true;
}

bool BluetoothGattDescriptor::writeValue(const QByteArray &value)
{
    if (!m_descriptorInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus characteristic interface for" << m_path.path();
        return false;
    }

    QDBusPendingCall writingCall = m_descriptorInterface->asyncCall("WriteValue", value, QVariantMap());
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(writingCall, this);
    m_asyncWrites.insert(watcher, value);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &BluetoothGattDescriptor::onWritingFinished);
    return true;
}

QDebug operator<<(QDebug debug, BluetoothGattDescriptor *descriptor)
{
    debug.noquote().nospace() << "GattDescriptor(" << descriptor->name();
    debug.noquote().nospace() << ", " << descriptor->uuid().toString();
    debug.noquote().nospace() << ", Properties: ";

    if (descriptor->properties().testFlag(BluetoothGattDescriptor::Read))
        debug.noquote().nospace() << " R ";

    if (descriptor->properties().testFlag(BluetoothGattDescriptor::Write))
        debug.noquote().nospace() << " W";

    if (descriptor->properties().testFlag(BluetoothGattDescriptor::EncryptRead))
        debug.noquote().nospace() << " ER";

    if (descriptor->properties().testFlag(BluetoothGattDescriptor::EncryptWrite))
        debug.noquote().nospace() << " EW";

    if (descriptor->properties().testFlag(BluetoothGattDescriptor::EncryptAuthenticatedRead))
        debug.noquote().nospace() << " EAR";

    if (descriptor->properties().testFlag(BluetoothGattDescriptor::EncryptAuthenticatedWrite))
        debug.noquote().nospace() << " EAW";


    if (descriptor->properties().testFlag(BluetoothGattDescriptor::SecureRead))
        debug.noquote().nospace() << " SR";

    if (descriptor->properties().testFlag(BluetoothGattDescriptor::SecureWrite))
        debug.noquote().nospace() << " SW";

    debug.noquote().nospace() << ", value: " << descriptor->value();
    debug.noquote().nospace() << ") ";
    return debug;
}
