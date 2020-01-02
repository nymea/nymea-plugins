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
