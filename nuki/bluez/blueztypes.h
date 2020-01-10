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

#ifndef BLUEZTYPES_H
#define BLUEZTYPES_H

#include <QDebug>
#include <QString>
#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QLoggingCategory>

// Interfaces DBus
static const QString orgFreedesktopDBus = QStringLiteral("org.freedesktop.DBus");
static const QString orgFreedesktopDBusObjectManager = QStringLiteral("org.freedesktop.DBus.ObjectManager");

// Interfaces Bluez
static const QString orgBluez = QStringLiteral("org.bluez");
static const QString orgBluezAdapter1 = QStringLiteral("org.bluez.Adapter1");
static const QString orgBluezDevice1 = QStringLiteral("org.bluez.Device1");
static const QString orgBluezGattService1 = QStringLiteral("org.bluez.GattService1");
static const QString orgBluezGattCharacteristic1 = QStringLiteral("org.bluez.GattCharacteristic1");
static const QString orgBluezGattDescriptor1 = QStringLiteral("org.bluez.GattDescriptor1");

// DBus Object and interface types
typedef QMap<QString, QVariantMap> InterfaceList;
Q_DECLARE_METATYPE(InterfaceList)

typedef QMap<QDBusObjectPath, InterfaceList> ManagedObjectList;
Q_DECLARE_METATYPE(ManagedObjectList)

typedef struct {
    quint16 manufacturerId;
    QByteArray data;
} ManufacturerData;

Q_DECLARE_METATYPE(ManufacturerData)

typedef QList<ManufacturerData> ManufacturerDataList;
Q_DECLARE_METATYPE(ManufacturerDataList)

// TODO: get naufacturer data
//QDBusArgument &operator<<(QDBusArgument &argument, const ManufacturerDataList &manufacturerDataList);

// Logging cathegory
Q_DECLARE_LOGGING_CATEGORY(dcBluez)

// Gadget class fot handling types
class Bluez {

    Q_GADGET
    Q_ENUMS(Error)

public:
    enum Error {
        NoError,
        NotReady,
        Failed,
        Rejected,
        Canceled,
        InvalidArguments,
        AlreadyExists,
        DoesNotExist,
        InProgress,
        NotInProgress,
        AlreadyConnected,
        ConnectFailed,
        NotConnected,
        NotSupported,
        NotAuthorized,
        AuthenticationCanceled,
        AuthenticationFailed,
        AuthenticationRejected,
        AuthenticationTimeout,
        ConnectionAttemptFailed,
        DBusError,
        UnknownError
    };
    Q_ENUM(Error)

    Bluez();
};

#endif // BLUEZTYPES_H
