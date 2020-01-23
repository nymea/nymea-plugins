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

#ifndef BLUETOOTHGATTDESCRIPTOR_H
#define BLUETOOTHGATTDESCRIPTOR_H

#include <QObject>
#include <QBluetoothUuid>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>

#include "blueztypes.h"

// Note: DBus documentation https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/gatt-api.txt

class BluetoothManager;
class BluetoothGattCharacteristic;

class BluetoothGattDescriptor : public QObject
{
    Q_OBJECT
    Q_FLAGS(Properties)

    friend class BluetoothManager;
    friend class BluetoothGattCharacteristic;

public:
    enum Property {
        Unknown = 0x00,
        Read = 0x01,
        Write = 0x02,
        EncryptRead = 0x04,
        EncryptWrite = 0x08,
        EncryptAuthenticatedRead = 0x10,
        EncryptAuthenticatedWrite = 0x20,
        SecureRead = 0x40, // Server only
        SecureWrite = 0x80 // Server only
    };
    Q_DECLARE_FLAGS(Properties, Property)

    QString name() const;
    QBluetoothUuid uuid() const;
    QByteArray value() const;
    Properties properties() const;

private:
    explicit BluetoothGattDescriptor(const QDBusObjectPath &path, const QVariantMap &properties, QObject *parent = 0);

    QDBusObjectPath m_path;
    QDBusInterface *m_descriptorInterface;

    QBluetoothUuid m_uuid;
    QByteArray m_value;
    Properties m_properties;
    QHash<QDBusPendingCallWatcher *, QByteArray> m_asyncWrites;

    void processProperties(const QVariantMap &properties);

    void setValueInternally(const QByteArray &value);

    Properties parsePropertyFlags(const QStringList &descriptorProperties);

signals:
    void valueChanged(const QByteArray &value);
    void readingFinished(const QByteArray &value);
    void writingFinished(const QByteArray &value);

private slots:
    void onPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

    void onReadingFinished(QDBusPendingCallWatcher *call);
    void onWritingFinished(QDBusPendingCallWatcher *call);

public slots:
    bool readValue();
    bool writeValue(const QByteArray &value);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(BluetoothGattDescriptor::Properties)

QDebug operator<<(QDebug debug, BluetoothGattDescriptor *descriptor);


#endif // BLUETOOTHGATTDESCRIPTOR_H
