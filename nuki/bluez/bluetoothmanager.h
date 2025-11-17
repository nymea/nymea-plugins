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

#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusServiceWatcher>

#include "blueztypes.h"
#include "bluetoothadapter.h"

class BluetoothManager : public QObject
{
    Q_OBJECT
public:
    explicit BluetoothManager(QObject *parent = 0);

    QList<BluetoothAdapter *> adapters() const;

    bool isAvailable() const;

private:
    QDBusInterface *m_objectManagerInterface;
    QDBusServiceWatcher *m_serviceWatcher;

    QList<BluetoothAdapter *> m_adapters;

    bool m_available;

    void init();
    void clean();

    void setAvailable(const bool &available);

    // DBus object helpers
    void processObjectList(const ManagedObjectList &objectList);
    void processInterfaceList(const QDBusObjectPath &objectPath, const InterfaceList &interfaceList);

    bool adapterAlreadyAdded(const QDBusObjectPath &objectPath);

    BluetoothAdapter *findAdapter(const QDBusObjectPath &objectPath);
    BluetoothDevice *findDevice(const QDBusObjectPath &objectPath);
    BluetoothGattService *findService(const QDBusObjectPath &objectPath);
    BluetoothGattCharacteristic *findCharacteristic(const QDBusObjectPath &objectPath);

signals:
    void availableChanged(const bool &available);

    void adapterAdded(BluetoothAdapter *adapter);
    void adapterRemoved(BluetoothAdapter *adapter);

private slots:
    void serviceRegistered(const QString &serviceName);
    void serviceUnregistered(const QString &serviceName);

    void onInterfaceAdded(const QDBusObjectPath &objectPath, const InterfaceList &interfaceList);
    void onInterfaceRemoved(const QDBusObjectPath &objectPath, const QStringList &interfaces);

public slots:

};

#endif // BLUETOOTHMANAGER_H
