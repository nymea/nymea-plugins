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

#include "bluetoothmanager.h"

#include <QDBusObjectPath>
#include <QDBusArgument>
#include <QDBusMetaType>

BluetoothManager::BluetoothManager(QObject *parent) :
    QObject(parent),
    m_available(false)
{
    qDBusRegisterMetaType<InterfaceList>();
    qDBusRegisterMetaType<ManagedObjectList>();

    // Check DBus connection
    if (!QDBusConnection::systemBus().isConnected()) {
        qCWarning(dcBluez()) << "System DBus not connected.";
        return;
    }

    // Get notification when bluez appears/disappears on DBus
    m_serviceWatcher = new QDBusServiceWatcher(orgBluez, QDBusConnection::systemBus(), QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration, this);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &BluetoothManager::serviceRegistered);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &BluetoothManager::serviceUnregistered);

    m_objectManagerInterface = new QDBusInterface(orgBluez, "/", orgFreedesktopDBusObjectManager, QDBusConnection::systemBus(), this);
    if (!m_objectManagerInterface->isValid()) {
        qCWarning(dcBluez()) << "Invalid DBus ObjectManager interface.";
        return;
    }

    QDBusConnection::systemBus().connect(orgBluez, "/", orgFreedesktopDBusObjectManager, "InterfacesAdded", this, SLOT(onInterfaceAdded(QDBusObjectPath,InterfaceList)));
    QDBusConnection::systemBus().connect(orgBluez, "/", orgFreedesktopDBusObjectManager, "InterfacesRemoved", this, SLOT(onInterfaceRemoved(QDBusObjectPath,QStringList)));

    init();
}

QList<BluetoothAdapter *> BluetoothManager::adapters() const
{
    return m_adapters;
}

bool BluetoothManager::isAvailable() const
{
    return m_available;
}

void BluetoothManager::init()
{
    // Get current object from org.bluez
    QDBusMessage query = m_objectManagerInterface->call("GetManagedObjects");
    if(query.type() != QDBusMessage::ReplyMessage) {
        qCWarning(dcBluez()) << "Could not initialize BluetoothManager:" << query.errorName() << query.errorMessage();
        return;
    }

    const QDBusArgument &argument = query.arguments().at(0).value<QDBusArgument>();
    ManagedObjectList objectList = qdbus_cast<ManagedObjectList>(argument);
    processObjectList(objectList);

    if (!m_adapters.isEmpty())
        setAvailable(true);

    qCDebug(dcBluez()) << "BluetoothManager initialized successfully.";
}

void BluetoothManager::clean()
{
    // Delete all adapter objects
    // Note: devices, services and characteristic objects will be removed throug parent relation
    foreach (BluetoothAdapter *adapter, m_adapters) {
        m_adapters.removeOne(adapter);
        emit adapterRemoved(adapter);
        adapter->deleteLater();
    }

    m_adapters.clear();

    setAvailable(false);
}

void BluetoothManager::setAvailable(const bool &available)
{
    if (m_available != available) {
        m_available = available;
        emit availableChanged(m_available);
    }
}

void BluetoothManager::processObjectList(const ManagedObjectList &objectList)
{
    foreach (const QDBusObjectPath &objectPath, objectList.keys()) {
        InterfaceList interfaceList = objectList.value(objectPath);
        processInterfaceList(objectPath, interfaceList);
    }
}

void BluetoothManager::processInterfaceList(const QDBusObjectPath &objectPath, const InterfaceList &interfaceList)
{
    // Note: object hierarchy: first add adapters, than devices, services, characteristics and finally descriptors

    // Adapter interface
    foreach (const QString &interface, interfaceList.keys()) {
        if (interface == orgBluezAdapter1) {
            QVariantMap properties = interfaceList.value(interface);
            // Check if this adapter already added
            if (!adapterAlreadyAdded(objectPath)) {
                BluetoothAdapter *adapter = new BluetoothAdapter(objectPath, properties, this);
                m_adapters.append(adapter);
                emit adapterAdded(adapter);
                qCDebug(dcBluez()) << "[+]" << adapter;
            }
        }
    }

    // Device interface
    foreach (const QString &interface, interfaceList.keys()) {
        if (interface == orgBluezDevice1) {
            QVariantMap properties = interfaceList.value(interface);
            // Find adapter for this thing and add the thing internally
            if (properties.contains("Adapter")) {
                QDBusObjectPath adapterObjectPath = qvariant_cast<QDBusObjectPath>(properties.value("Adapter"));
                BluetoothAdapter *adapter = findAdapter(adapterObjectPath);
                if (adapter)
                    adapter->addDeviceInternally(objectPath, properties);

            }
        }
    }

    // GATT Service interface
    foreach (const QString &interface, interfaceList.keys()) {
        if (interface == orgBluezGattService1) {
            QVariantMap properties = interfaceList.value(interface);
            // Find thing for this service and add the service internally
            if (properties.contains("Device")) {
                QDBusObjectPath deviceObjectPath = qvariant_cast<QDBusObjectPath>(properties.value("Device"));
                BluetoothDevice *thing = findDevice(deviceObjectPath);
                if (thing)
                    thing->addServiceInternally(objectPath, properties);

            }
        }
    }

    // GATT Characteristic interface
    foreach (const QString &interface, interfaceList.keys()) {
        if (interface == orgBluezGattCharacteristic1) {
            QVariantMap properties = interfaceList.value(interface);
            // Find service for this characteristic
            if (properties.contains("Service")) {
                QDBusObjectPath serviceObjectPath = qvariant_cast<QDBusObjectPath>(properties.value("Service"));
                BluetoothGattService *service = findService(serviceObjectPath);
                if (service)
                    qCDebug(dcBluez()) << "Add characteristic" << serviceObjectPath.path() << properties << service;
                    service->addCharacteristicInternally(objectPath, properties);

            }
        }
    }

    // GATT Descriptor interface
    foreach (const QString &interface, interfaceList.keys()) {
        if (interface == orgBluezGattDescriptor1) {
            QVariantMap properties = interfaceList.value(interface);
            // Find characteristic for this desciptor
            if (properties.contains("Characteristic")) {
                QDBusObjectPath characterisitcObjectPath = qvariant_cast<QDBusObjectPath>(properties.value("Characteristic"));
                BluetoothGattCharacteristic *characteristic = findCharacteristic(characterisitcObjectPath);
                if (characteristic)
                    qCDebug(dcBluez()) << "Add descriptor" << characterisitcObjectPath.path() << properties << characteristic;
                    characteristic->addDescriptorInternally(objectPath, properties);

            }
        }
    }

}

bool BluetoothManager::adapterAlreadyAdded(const QDBusObjectPath &objectPath)
{
    foreach (BluetoothAdapter *existingAdapter, m_adapters) {
        if (existingAdapter->m_path == objectPath) {
            return true;
        }
    }

    return false;
}

BluetoothAdapter *BluetoothManager::findAdapter(const QDBusObjectPath &objectPath)
{
    foreach (BluetoothAdapter *adapter, m_adapters) {
        if (adapter->m_path == objectPath) {
            return adapter;
        }
    }

    return nullptr;
}

BluetoothDevice *BluetoothManager::findDevice(const QDBusObjectPath &objectPath)
{
    foreach (BluetoothAdapter *adapter, m_adapters) {
        foreach (BluetoothDevice *thing, adapter->devices()) {
            if (thing->m_path == objectPath) {
                return thing;
            }
        }
    }

    return nullptr;
}

BluetoothGattService *BluetoothManager::findService(const QDBusObjectPath &objectPath)
{
    foreach (BluetoothAdapter *adapter, m_adapters) {
        foreach (BluetoothDevice *thing, adapter->devices()) {
            if (thing->hasService(objectPath)) {
                return thing->getService(objectPath);
            }
        }
    }

    return nullptr;
}

BluetoothGattCharacteristic *BluetoothManager::findCharacteristic(const QDBusObjectPath &objectPath)
{
    foreach (BluetoothAdapter *adapter, m_adapters) {
        foreach (BluetoothDevice *thing, adapter->devices()) {
            foreach (BluetoothGattService *service, thing->services()) {
                if (service->hasCharacteristic(objectPath)) {
                    return service->getCharacteristic(objectPath);
                }
            }
        }
    }

    return nullptr;
}

void BluetoothManager::serviceRegistered(const QString &serviceName)
{
    qCDebug(dcBluez()) << "BluetoothManager: service registered" << serviceName;
    init();
}

void BluetoothManager::serviceUnregistered(const QString &serviceName)
{
    qCDebug(dcBluez()) << "BluetoothManager: service unregistered" << serviceName;
    if (serviceName == orgBluez)
        clean();
}

void BluetoothManager::onInterfaceAdded(const QDBusObjectPath &objectPath, const InterfaceList &interfaceList)
{
    //qCDebug(dcBluez()) << "Interface added" << objectPath.path();
    processInterfaceList(objectPath, interfaceList);
}

void BluetoothManager::onInterfaceRemoved(const QDBusObjectPath &objectPath, const QStringList &interfaces)
{
    //qCDebug(dcBluez()) << "Interface removed" << objectPath.path() << interfaces;

    // Adapter removed
    if (interfaces.contains(orgBluezAdapter1)) {
        BluetoothAdapter *adapter = findAdapter(objectPath);
        qCDebug(dcBluez()) << "[-]" << adapter;
        if (adapter) {
            m_adapters.removeOne(adapter);
            emit adapterRemoved(adapter);
            adapter->deleteLater();
        }
    }

    // Device removed
    if (interfaces.contains(orgBluezDevice1)) {
        // Find adapter for this thing
        foreach (BluetoothAdapter *adapter, m_adapters) {
            if (adapter->hasDevice(objectPath)) {
                adapter->removeDeviceInternally(objectPath);
            }
        }
    }

}
