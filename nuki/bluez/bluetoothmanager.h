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
