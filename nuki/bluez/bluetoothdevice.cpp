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

#include "bluetoothdevice.h"

#include <QDBusPendingReply>

BluetoothDevice::State BluetoothDevice::state() const
{
    return m_state;
}

QString BluetoothDevice::name() const
{
    return m_name;
}

QBluetoothAddress BluetoothDevice::address() const
{
    return m_address;
}

QString BluetoothDevice::iconName() const
{
    return m_iconName;
}

QBluetoothHostInfo BluetoothDevice::hostInfo() const
{
    return m_hostInfo;
}

QString BluetoothDevice::alias() const
{
    return m_alias;
}

bool BluetoothDevice::setAlias(const QString &alias)
{
    if (!m_deviceInterface->isValid())
        return false;

    return m_deviceInterface->setProperty("Alias", QVariant(alias));
}

QString BluetoothDevice::modalias() const
{
    return m_modalias;
}

quint32 BluetoothDevice::deviceClass() const
{
    return m_deviceClass;
}

quint16 BluetoothDevice::appearance() const
{
    return m_appearance;
}

qint16 BluetoothDevice::rssi() const
{
    return m_rssi;
}

qint16 BluetoothDevice::txPower() const
{
    return m_txPower;
}

QList<QBluetoothUuid> BluetoothDevice::uuids() const
{
    return m_uuids;
}

bool BluetoothDevice::paired() const
{
    return m_paired;
}

bool BluetoothDevice::connected() const
{
    return m_connected;
}

bool BluetoothDevice::trusted() const
{
    return m_trusted;
}

bool BluetoothDevice::setTrusted(const bool &trusted)
{
    if (!m_deviceInterface->isValid())
        return false;

    return m_deviceInterface->setProperty("Trusted", QVariant(trusted));
}

bool BluetoothDevice::blocked() const
{
    return m_blocked;
}

bool BluetoothDevice::setBlocked(const bool &blocked)
{
    if (!m_deviceInterface->isValid())
        return false;

    return m_deviceInterface->setProperty("Blocked", QVariant(blocked));
}

bool BluetoothDevice::legacyPairing() const
{
    return m_legacyPairing;
}

bool BluetoothDevice::servicesResolved() const
{
    return m_servicesResolved;
}

QList<BluetoothGattService *> BluetoothDevice::services() const
{
    return m_services;
}

bool BluetoothDevice::hasService(const QBluetoothUuid &serviceUuid)
{
    foreach (BluetoothGattService *service, m_services) {
        if (service->uuid() == serviceUuid) {
            return true;
        }
    }

    return false;
}

BluetoothGattService *BluetoothDevice::getService(const QBluetoothUuid &serviceUuid)
{
    foreach (BluetoothGattService *service, m_services) {
        if (service->uuid() == serviceUuid) {
            return service;
        }
    }

    return nullptr;
}

BluetoothDevice::BluetoothDevice(const QDBusObjectPath &path, const QVariantMap &properties, QObject *parent) :
    QObject(parent),
    m_path(path),
    m_state(Disconnected),
    m_deviceClass(0),
    m_appearance(0),
    m_rssi(0),
    m_txPower(0),
    m_paired(false),
    m_connected(false),
    m_trusted(false),
    m_blocked(false),
    m_legacyPairing(false),
    m_servicesResolved(false),
    m_connectWatcher(nullptr),
    m_disconnectWatcher(nullptr),
    m_pairingWatcher(nullptr)
{
    // Check DBus connection
    if (!QDBusConnection::systemBus().isConnected()) {
        qCWarning(dcBluez()) << "System DBus not connected.";
        return;
    }

    m_deviceInterface = new QDBusInterface(orgBluez, m_path.path(), orgBluezDevice1, QDBusConnection::systemBus(), this);
    if (!m_deviceInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus device interface for" << m_path.path();
        return;
    }

    QDBusConnection::systemBus().connect(orgBluez, m_path.path(), "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));

    processProperties(properties);

    // Set initial state
    evaluateCurrentState();
}

BluetoothDevice::~BluetoothDevice()
{

}

void BluetoothDevice::processProperties(const QVariantMap &properties)
{
    foreach (const QString &propertyName, properties.keys()) {
        if (propertyName == "Name") {
            m_name = properties.value(propertyName).toString();
            m_hostInfo.setName(m_name);
        } else if (propertyName == "Address") {
            m_address = QBluetoothAddress(properties.value(propertyName).toString());
            m_hostInfo.setAddress(m_address);
        }  else if (propertyName == "Icon") {
            m_iconName = properties.value(propertyName).toString();
        } else if (propertyName == "Alias") {
            setAliasInternally(properties.value(propertyName).toString());
        } else if (propertyName == "Modalias") {
            m_modalias = properties.value(propertyName).toString();
        } else if (propertyName == "Class") {
            m_deviceClass = properties.value(propertyName).toUInt();
        } else if (propertyName == "Appearance") {
            m_appearance = properties.value(propertyName).toUInt();
        } else if (propertyName == "RSSI") {
            setRssiInternally(properties.value(propertyName).toInt());
        } else if (propertyName == "TxPower") {
            setTxPowerInternally(properties.value(propertyName).toInt());
        } else if (propertyName == "UUIDs") {
            QStringList uuidStrings = properties.value(propertyName).toStringList();
            m_uuids.clear();
            foreach (const QString &uuidString, uuidStrings) {
                QBluetoothUuid uuid = QBluetoothUuid(QUuid(uuidString));
                m_uuids.append(uuid);
            }
        } else if (propertyName == "Paired") {
            setPairedInternally(properties.value(propertyName).toBool());
        } else if (propertyName == "Connected") {
            setConnectedInternally(properties.value(propertyName).toBool());
        } else if (propertyName == "Trusted") {
            setTrustedInternally(properties.value(propertyName).toBool());
        } else if (propertyName == "Blocked") {
            setBlockedInternally(properties.value(propertyName).toBool());
        } else if (propertyName == "LegacyPairing") {
            m_legacyPairing = properties.value(propertyName).toBool();
        } else if (propertyName == "ServicesResolved") {
            setServicesResolvedInternally(properties.value(propertyName).toBool());
        }
    }

}

void BluetoothDevice::evaluateCurrentState()
{
    if (!connected()) {
        setStateInternally(Disconnected);
    } else if (connected() && servicesResolved()) {
        setStateInternally(Discovered);
    }
}

void BluetoothDevice::addServiceInternally(const QDBusObjectPath &path, const QVariantMap &properties)
{
    if (hasService(path))
        return;

    BluetoothGattService *service = new BluetoothGattService(path, properties, this);
    m_services.append(service);
    qCDebug(dcBluez()) << "[+]" << service;
}

bool BluetoothDevice::hasService(const QDBusObjectPath &path)
{
    foreach (BluetoothGattService *service, m_services) {
        if (service->m_path == path) {
            return true;
        }
    }

    return false;
}

BluetoothGattService *BluetoothDevice::getService(const QDBusObjectPath &path)
{
    foreach (BluetoothGattService *service, m_services) {
        if (service->m_path == path) {
            return service;
        }
    }

    return nullptr;
}

void BluetoothDevice::setStateInternally(const BluetoothDevice::State &state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(m_state);
    }
}

void BluetoothDevice::setAliasInternally(const QString &alias)
{
    if (m_alias != alias) {
        m_alias = alias;
        emit aliasChanged(m_alias);
    }
}

void BluetoothDevice::setRssiInternally(const qint16 &rssi)
{
    if (m_rssi != rssi) {
        m_rssi = rssi;
        emit rssiChanged(m_rssi);
    }
}

void BluetoothDevice::setTxPowerInternally(const qint16 &txPower)
{
    if (m_txPower != txPower) {
        m_txPower = txPower;
        emit txPowerChanged(m_txPower);
    }
}

void BluetoothDevice::setPairedInternally(const bool &paired)
{
    if (m_paired != paired) {
        m_paired = paired;
        emit pairedChanged(m_paired);

        // Paired, if services not resolved
        if (!m_servicesResolved) {
            setStateInternally(Discovering);
        } else {
            setStateInternally(Discovered);
        }
    }
}

void BluetoothDevice::setConnectedInternally(const bool &connected)
{
    if (m_connected != connected) {
        m_connected = connected;
        emit connectedChanged(m_connected);

        if (m_connected) {
            setStateInternally(Connected);

            if (!m_servicesResolved)
                setStateInternally(Discovering);

        } else {
            setStateInternally(Disconnected);
        }
    }
}

void BluetoothDevice::setTrustedInternally(const bool &trusted)
{
    if (m_trusted != trusted) {
        m_trusted = trusted;
        emit trustedChanged(m_trusted);
    }
}

void BluetoothDevice::setBlockedInternally(const bool &blocked)
{
    if (m_blocked != blocked) {
        m_blocked = blocked;
        emit blockedChanged(m_blocked);
    }
}

void BluetoothDevice::setServicesResolvedInternally(const bool &servicesResolved)
{
    if (m_servicesResolved != servicesResolved) {
        m_servicesResolved = servicesResolved;
        emit servicesResolvedChanged(m_servicesResolved);

        if (m_servicesResolved)
            setStateInternally(Discovered);

        // Note: if unresolved, the device is about to disconnect
    }
}

void BluetoothDevice::onPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    if (interface != orgBluezDevice1)
        return;

    qCDebug(dcBluez()) << "BluetoothDevice:" << m_name << m_address << "properties changed" << interface << changedProperties << invalidatedProperties;
    processProperties(changedProperties);
}

void BluetoothDevice::onConnectDeviceFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    if (reply.isError()) {
        setStateInternally(Disconnected);
        qCWarning(dcBluez()) << "Could not connect device" << m_address.toString() << reply.error().name() << reply.error().message();
    }

    call->deleteLater();
    m_connectWatcher = nullptr;
}

void BluetoothDevice::onDisconnectDeviceFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    if (reply.isError())
        qCWarning(dcBluez()) << "Could not disconnect device" << m_address.toString() << reply.error().name() << reply.error().message();

    evaluateCurrentState();

    call->deleteLater();
    m_disconnectWatcher = nullptr;
}

void BluetoothDevice::onPairingFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    if (reply.isError())
        qCWarning(dcBluez()) << "Could not pair device" << m_address.toString() << reply.error().name() << reply.error().message();

    evaluateCurrentState();

    call->deleteLater();
    m_pairingWatcher = nullptr;
}

void BluetoothDevice::onCancelPairingFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    if (reply.isError())
        qCWarning(dcBluez()) << "Could not cancel pairing" << m_address.toString() << reply.error().name() << reply.error().message();

    evaluateCurrentState();

    call->deleteLater();
}

bool BluetoothDevice::connectDevice()
{
    if (!m_deviceInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus device interface for" << m_path.path();
        return false;
    }

    if (connected() || state() == Connecting || m_connectWatcher)
        return true;

    setStateInternally(Connecting);

    QDBusPendingCall connectingCall = m_deviceInterface->asyncCall("Connect");
    m_connectWatcher = new QDBusPendingCallWatcher(connectingCall, this);
    connect(m_connectWatcher, &QDBusPendingCallWatcher::finished, this, &BluetoothDevice::onConnectDeviceFinished);
    return true;
}

bool BluetoothDevice::disconnectDevice()
{
    if (!m_deviceInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus device interface for" << m_path.path();
        return false;
    }

    if (!connected() || state() == Disconnecting || m_disconnectWatcher)
        return true;

    setStateInternally(Disconnecting);

    QDBusPendingCall disconnectingCall = m_deviceInterface->asyncCall("Disconnect");
    m_disconnectWatcher = new QDBusPendingCallWatcher(disconnectingCall, this);
    connect(m_disconnectWatcher, &QDBusPendingCallWatcher::finished, this, &BluetoothDevice::onDisconnectDeviceFinished);
    return true;
}

bool BluetoothDevice::disconnectDeviceBlocking()
{
    if (!m_deviceInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus device interface for" << m_path.path();
        return false;
    }

    if (!connected() || state() == Disconnecting)
        return true;

    qCWarning(dcBluez()) << "Disconnecting blocking" << this;
    QDBusPendingReply<void> reply = m_deviceInterface->call("Disconnect");
    reply.waitForFinished();
    if(reply.isError()) {
        qCWarning(dcBluez()) << reply.error();
        return false;
    }

    return true;
}

bool BluetoothDevice::requestPairing()
{
    if (!m_deviceInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus device interface for" << m_path.path();
        return false;
    }

    if (paired() || state() == Pairing || m_pairingWatcher)
        return true;

    setStateInternally(Pairing);

    QDBusPendingCall pairCall = m_deviceInterface->asyncCall("Pair");
    m_pairingWatcher = new QDBusPendingCallWatcher(pairCall, this);
    connect(m_pairingWatcher, &QDBusPendingCallWatcher::finished, this, &BluetoothDevice::onPairingFinished);
    return true;
}

bool BluetoothDevice::cancelPairingRequest()
{
    if (!m_deviceInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus device interface for" << m_path.path();
        return false;
    }

    if (!paired() || state() == Unpairing)
        return true;

    setStateInternally(Unpairing);

    QDBusPendingCall cancelPairingCall = m_deviceInterface->asyncCall("CancelPairing");
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(cancelPairingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &BluetoothDevice::onPairingFinished);
    return true;
}

QDebug operator<<(QDebug debug, BluetoothDevice *device)
{
    debug.noquote().nospace() << "BluetoothDevice(" << device->name() << ", " << device->address() << ") ";
    return debug;
}
