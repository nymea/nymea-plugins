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

#ifndef BLUETOOTHADAPTER_H
#define BLUETOOTHADAPTER_H

#include <QObject>
#include <QDebug>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusObjectPath>

#include "bluetoothdevice.h"

// Note: DBus documentation https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/adapter-api.txt

class BluetoothManager;

class BluetoothAdapter : public QObject
{
    Q_OBJECT

    friend class BluetoothManager;

public:
    // Properties
    QString name() const;

    QString alias() const;
    bool setAlias(const QString &alias);

    QString address() const;
    QString modalias() const;
    bool discovering() const;

    bool discoverable() const;
    bool setDiscoverable(const bool &discoverable);

    uint discoverableTimeout() const;
    bool setDiscoverableTimeout(const uint &seconds);

    bool pairable() const;
    bool setPairable(const bool &pairable);

    uint pairableTimeout() const;
    bool setPairableTimeout(const uint &seconds);

    uint adapterClass() const;
    QString adapterClassString() const;

    bool powered() const;
    bool setPower(const bool &power);

    QStringList uuids() const;

    QList<BluetoothDevice *> devices() const;
    bool hasDevice(const QBluetoothAddress &address);
    BluetoothDevice *getDevice(const QBluetoothAddress &address);
    bool removeDevice(const QBluetoothAddress &address);

    bool isValid() const;

private:
    // Note: only BluetoothManager can create adapter objects
    explicit BluetoothAdapter(const QDBusObjectPath &path, const QVariantMap &properties, QObject *parent = 0);
    ~BluetoothAdapter();

    QDBusObjectPath m_path;
    QDBusInterface *m_adapterInterface;

    QString m_name;
    QString m_address;

    QString m_alias;
    QString m_modalias;
    bool m_discovering;
    bool m_discoverable;
    uint m_discoverableTimeout;
    bool m_pairable;
    uint m_pairableTimeout;
    uint m_adapterClass;
    bool m_powered;
    QStringList m_uuids;

    QList<BluetoothDevice *> m_devices;

    void processProperties(const QVariantMap &properties);

    // Methods called from BluetoothManager
    void addDeviceInternally(const QDBusObjectPath &path, const QVariantMap &properties);
    void removeDeviceInternally(const QDBusObjectPath &path);

    // Verification methods
    bool hasDevice(const QDBusObjectPath &path);

    // DBus methods
    bool removeDevice(const QDBusObjectPath &path);


    void setAliasInternally(const QString &alias);
    void setDiscoveringInternally(const bool &discovering);
    void setDiscoverableInernally(const bool &discoverable);
    void setDiscoverableTimeoutInternally(const uint &discoverableTimeout);
    void setPairableInternally(const bool &pairable);
    void setPairableTimeoutInternally(const uint &pairableTimeout);
    void setPoweredInternally(const bool &powered);

private slots:
    void onPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
    void onRemoveDeviceFinished(QDBusPendingCallWatcher *call);

signals:
    void aliasChanged(const QString &alias);
    void discoveringChanged(const bool &discovering);
    void discoverableChanged(const bool &discoverable);
    void discoverableTimeoutChanged(const uint &discoverableTimeout);
    void pairableChanged(const bool &pairable);
    void pairableTimeoutChanged(const uint &pairableTimeout);
    void poweredChanged(const bool &powered);

    void deviceAdded(BluetoothDevice *thing);
    void deviceRemoved(BluetoothDevice *thing);

public slots:
    void startDiscovering();
    void stopDiscovering();

};

QDebug operator<<(QDebug debug, BluetoothAdapter *adapter);


#endif // BLUETOOTHADAPTER_H
