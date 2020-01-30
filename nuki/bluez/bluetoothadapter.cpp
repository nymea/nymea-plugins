/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016-2018 Simon Stürz <simon.stuerz@guh.io>              *
 *                                                                         *
 *  This file is part of guh.                                              *
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

#include "bluetoothadapter.h"
#include "blueztypes.h"

#include <QDBusPendingReply>

QString BluetoothAdapter::name() const
{
    return m_name;
}

QString BluetoothAdapter::alias() const
{
    return m_alias;
}

bool BluetoothAdapter::setAlias(const QString &alias)
{
    if (!m_adapterInterface->isValid())
        return false;

    return m_adapterInterface->setProperty("Alias", QVariant(alias));
}

QString BluetoothAdapter::address() const
{
    return m_address;
}

QString BluetoothAdapter::modalias() const
{
    return m_modalias;
}

bool BluetoothAdapter::discovering() const
{
    return m_discovering;
}

bool BluetoothAdapter::discoverable() const
{
    return m_discoverable;
}

bool BluetoothAdapter::setDiscoverable(const bool &discoverable)
{
    if (!m_adapterInterface->isValid())
        return false;

    return m_adapterInterface->setProperty("Discoverable", QVariant(discoverable));
}

uint BluetoothAdapter::discoverableTimeout() const
{
    return m_discoverableTimeout;
}

bool BluetoothAdapter::setDiscoverableTimeout(const uint &seconds)
{
    if (!m_adapterInterface->isValid())
        return false;

    return m_adapterInterface->setProperty("DiscoverableTimeout", QVariant(seconds));
}

bool BluetoothAdapter::pairable() const
{
    return m_pairable;
}

bool BluetoothAdapter::setPairable(const bool &pairable)
{
    if (!m_adapterInterface->isValid())
        return false;

    return m_adapterInterface->setProperty("Pairable", QVariant(pairable));
}

uint BluetoothAdapter::pairableTimeout() const
{
    return m_pairableTimeout;
}

bool BluetoothAdapter::setPairableTimeout(const uint &seconds)
{
    if (!m_adapterInterface->isValid())
        return false;

    return m_adapterInterface->setProperty("PairableTimeout", QVariant(seconds));
}

uint BluetoothAdapter::adapterClass() const
{
    return m_adapterClass;
}

QString BluetoothAdapter::adapterClassString() const
{
    QString adapterClassString;
    switch (m_adapterClass) {
    // TODO: get device type from class
    default:
        break;
    }
    return adapterClassString;
}

bool BluetoothAdapter::powered() const
{
    return m_powered;
}

bool BluetoothAdapter::setPower(const bool &power)
{
    if (!m_adapterInterface->isValid())
        return false;

    return m_adapterInterface->setProperty("Powered", QVariant(power));
}

QStringList BluetoothAdapter::uuids() const
{
    return m_uuids;
}

QList<BluetoothDevice *> BluetoothAdapter::devices() const
{
    return m_devices;
}

bool BluetoothAdapter::hasDevice(const QBluetoothAddress &address)
{
    foreach (BluetoothDevice *device, m_devices) {
        if (device->address() == address) {
            return true;
        }
    }

    return false;
}

BluetoothDevice *BluetoothAdapter::getDevice(const QBluetoothAddress &address)
{
    foreach (BluetoothDevice *device, m_devices) {
        if (device->address() == address) {
            return device;
        }
    }

    return nullptr;
}

bool BluetoothAdapter::removeDevice(const QBluetoothAddress &address)
{
    foreach (BluetoothDevice *device, m_devices) {
        if (device->address() == address) {
            return removeDevice(device->m_path);
        }
    }

    return false;
}

bool BluetoothAdapter::isValid() const
{
    return m_adapterInterface->isValid();
}

BluetoothAdapter::BluetoothAdapter(const QDBusObjectPath &path, const QVariantMap &properties, QObject *parent) :
    QObject(parent),
    m_path(path),
    m_discovering(false),
    m_discoverable(false),
    m_discoverableTimeout(0),
    m_pairable(false),
    m_pairableTimeout(0),
    m_adapterClass(0),
    m_powered(false)
{
    // Check DBus connection
    if (!QDBusConnection::systemBus().isConnected()) {
        qCWarning(dcBluez()) << "System DBus not connected.";
        return;
    }

    m_adapterInterface = new QDBusInterface(orgBluez, m_path.path(), orgBluezAdapter1, QDBusConnection::systemBus(), this);
    if (!m_adapterInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus adapter interface for" << m_path.path();
        return;
    }

    QDBusConnection::systemBus().connect(orgBluez, m_path.path(), "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));

    processProperties(properties);
}

BluetoothAdapter::~BluetoothAdapter()
{

}

void BluetoothAdapter::processProperties(const QVariantMap &properties)
{
    foreach (const QString &propertyName, properties.keys()) {
        if (propertyName == "Name") {
            m_name = properties.value(propertyName).toString();
        } else if (propertyName == "Alias") {
            setAliasInternally(properties.value(propertyName).toString());
        } else if (propertyName == "Address") {
            m_address = properties.value(propertyName).toString();
        } else if (propertyName == "Modalias") {
            m_modalias = properties.value(propertyName).toString();
        } else if (propertyName == "Discovering") {
            setDiscoveringInternally(properties.value(propertyName).toBool());
        } else if (propertyName == "Discoverable") {
            setDiscoverableInernally(properties.value(propertyName).toBool());
        } else if (propertyName == "DiscoverableTimeout") {
            setDiscoverableTimeoutInternally(properties.value(propertyName).toUInt());
        } else if (propertyName == "Pairable") {
            setPairableInternally(properties.value(propertyName).toBool());
        } else if (propertyName == "PairableTimeout") {
            setPairableTimeoutInternally(properties.value(propertyName).toUInt());
        } else if (propertyName == "Class") {
            m_adapterClass = properties.value(propertyName).toUInt();
        } else if (propertyName == "Powered") {
            setPoweredInternally(properties.value(propertyName).toBool());
        } else if (propertyName == "UUIDs") {
            m_uuids = properties.value(propertyName).toStringList();
        }
    }
}

void BluetoothAdapter::addDeviceInternally(const QDBusObjectPath &path, const QVariantMap &properties)
{
    // Check if device already added
    if (hasDevice(path))
        return;

    BluetoothDevice *device = new BluetoothDevice(path, properties, this);
    m_devices.append(device);

    qCDebug(dcBluez()) << "[+]" << device;

    emit deviceAdded(device);
}

void BluetoothAdapter::removeDeviceInternally(const QDBusObjectPath &path)
{
    foreach (BluetoothDevice *device, m_devices) {
        if (device->m_path == path) {
            m_devices.removeOne(device);
            emit deviceRemoved(device);
            device->deleteLater();
        }
    }
}

bool BluetoothAdapter::hasDevice(const QDBusObjectPath &path)
{
    foreach (BluetoothDevice *device, m_devices) {
        if (device->m_path == path)
            return true;
    }

    return false;
}

bool BluetoothAdapter::removeDevice(const QDBusObjectPath &path)
{
    if (!m_adapterInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus adapter interface for" << m_path.path();
        return false;
    }

    qCDebug(dcBluez()) << "Remove and unpair device" << path.path();
    QDBusPendingCall removeCall = m_adapterInterface->asyncCall("RemoveDevice", QVariant::fromValue(path));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(removeCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &BluetoothAdapter::onRemoveDeviceFinished);
    return true;
}

void BluetoothAdapter::setAliasInternally(const QString &alias)
{
    if (m_alias != alias) {
        m_alias = alias;
        emit aliasChanged(m_alias);
    }
}

void BluetoothAdapter::setDiscoveringInternally(const bool &discovering)
{
    if (m_discovering != discovering) {
        m_discovering = discovering;
        emit discoveringChanged(m_discovering);
    }
}

void BluetoothAdapter::setDiscoverableInernally(const bool &discoverable)
{
    if (m_discoverable != discoverable) {
        m_discoverable = discoverable;
        emit discoverableChanged(m_discoverable);
    }
}

void BluetoothAdapter::setDiscoverableTimeoutInternally(const uint &discoverableTimeout)
{
    if (m_discoverableTimeout != discoverableTimeout) {
        m_discoverableTimeout = discoverableTimeout;
        emit discoverableTimeoutChanged(m_discoverableTimeout);
    }
}

void BluetoothAdapter::setPairableInternally(const bool &pairable)
{
    if (m_pairable != pairable) {
        m_pairable = pairable;
        emit pairableChanged(m_pairable);
    }
}

void BluetoothAdapter::setPairableTimeoutInternally(const uint &pairableTimeout)
{
    if (m_pairableTimeout != pairableTimeout) {
        m_pairableTimeout = pairableTimeout;
        emit pairableTimeoutChanged(m_pairableTimeout);
    }
}

void BluetoothAdapter::setPoweredInternally(const bool &powered)
{
    if (m_powered != powered) {
        m_powered = powered;
        emit poweredChanged(m_powered);
    }
}

void BluetoothAdapter::onPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    if (interface != orgBluezAdapter1)
        return;

    qCDebug(dcBluez()) << "BluetoothAdapter:" <<  m_name << m_address << "properties changed" << interface << changedProperties << invalidatedProperties;
    processProperties(changedProperties);
}

void BluetoothAdapter::onRemoveDeviceFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    if (reply.isError())
        qCWarning(dcBluez()) << "Could not remove device" << m_address << reply.error().name() << reply.error().message();

    call->deleteLater();
}

void BluetoothAdapter::startDiscovering()
{
    if (!m_adapterInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus adapter interface for" << m_path.path();
        return;
    }

    if (discovering())
        return;

    QDBusMessage query = m_adapterInterface->call("StartDiscovery");
    if(query.type() != QDBusMessage::ReplyMessage) {
        qCWarning(dcBluez()) << "Could not start discovery" << m_name << ":" << query.errorName() << query.errorMessage();
        return;
    }
}

void BluetoothAdapter::stopDiscovering()
{
    if (!m_adapterInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus adapter interface for" << m_path.path();
        return;
    }

    QDBusMessage query = m_adapterInterface->call("StopDiscovery");
    if(query.type() != QDBusMessage::ReplyMessage) {
        qCWarning(dcBluez()) << "Could not start discovery" << m_name << ":" << query.errorName() << query.errorMessage();
        return;
    }
}

QDebug operator<<(QDebug debug, BluetoothAdapter *adapter)
{
    debug.noquote().nospace() << "BluetoothAdapter(" << adapter->name() << ", " << adapter->address();
    debug.noquote().nospace() << ", powered: " << adapter->powered();
    debug.noquote().nospace() << ", pairable: " << adapter->pairable();
    debug.noquote().nospace() << ", visible: " << adapter->discoverable();
    debug.noquote().nospace() << ") ";
    return debug;
}
