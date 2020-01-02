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

#ifndef BLUETOOTHDEVICE_H
#define BLUETOOTHDEVICE_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QBluetoothAddress>
#include <QBluetoothHostInfo>
#include <QDBusPendingCallWatcher>

#include "blueztypes.h"
#include "bluetoothgattservice.h"

// Note: DBus documentation https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/device-api.txt

class BluetoothManager;
class BluetoothAdapter;

class BluetoothDevice : public QObject
{
    Q_OBJECT

    friend class BluetoothManager;
    friend class BluetoothAdapter;

public:
    enum State {
        Connecting,
        Connected,
        Pairing,
        Unpairing,
        Discovering,
        Discovered,
        Disconnecting,
        Disconnected
    };
    Q_ENUM(State)

    State state() const;

    QString name() const;
    QBluetoothAddress address() const;
    QString iconName() const;

    QBluetoothHostInfo hostInfo() const;

    QString alias() const;
    bool setAlias(const QString &alias);

    QString modalias() const;
    quint32 deviceClass() const;
    quint16 appearance() const;
    qint16 rssi() const;
    qint16 txPower() const;
    QList<QBluetoothUuid> uuids() const;
    bool paired() const;
    bool connected() const;

    bool trusted() const;
    bool setTrusted(const bool &trusted);

    bool blocked() const;
    bool setBlocked(const bool &blocked);

    bool legacyPairing() const;
    bool servicesResolved() const;

    // Service methods
    QList<BluetoothGattService *> services() const;
    bool hasService(const QBluetoothUuid &serviceUuid);
    BluetoothGattService *getService(const QBluetoothUuid &serviceUuid);

private:
    explicit BluetoothDevice(const QDBusObjectPath &path, const QVariantMap &properties, QObject *parent = 0);
    ~BluetoothDevice();

    QDBusObjectPath m_path;
    QDBusInterface *m_deviceInterface;

    QList<BluetoothGattService *> m_services;

    State m_state;

    QString m_name;
    QBluetoothAddress m_address;
    QBluetoothHostInfo m_hostInfo;
    QString m_iconName;
    QString m_alias;
    QString m_modalias;
    quint32 m_deviceClass;
    quint16 m_appearance;
    qint16 m_rssi;
    qint16 m_txPower;

    QList<QBluetoothUuid> m_uuids;
    bool m_paired;
    bool m_connected;
    bool m_trusted;
    bool m_blocked;
    bool m_legacyPairing;
    bool m_servicesResolved;

    QDBusObjectPath m_adapterObjectPath;

    QDBusPendingCallWatcher *m_connectWatcher;
    QDBusPendingCallWatcher *m_disconnectWatcher;
    QDBusPendingCallWatcher *m_pairingWatcher;

    // TODO: org.bluez.Device1.ManufacturerData
    // TODO: org.bluez.Device1.ServiceData

    void processProperties(const QVariantMap &properties);

    void evaluateCurrentState();

    // Methods called from BluetoothManager
    void addServiceInternally(const QDBusObjectPath &path, const QVariantMap &properties);
    bool hasService(const QDBusObjectPath &path);
    BluetoothGattService *getService(const QDBusObjectPath &path);

    void setStateInternally(const State &state);

    void setAliasInternally(const QString &alias);
    void setRssiInternally(const qint16 &rssi);
    void setTxPowerInternally(const qint16 &txPower);
    void setPairedInternally(const bool &paired);
    void setConnectedInternally(const bool &connected);
    void setTrustedInternally(const bool &trusted);
    void setBlockedInternally(const bool &blocked);
    void setServicesResolvedInternally(const bool &servicesResolved);

signals:
    void stateChanged(const State &state);

    void aliasChanged(const QString &alias);
    void rssiChanged(const qint16 &rssi);
    void txPowerChanged(const qint16 &txPower);
    void pairedChanged(const bool &paired);
    void connectedChanged(const bool &connected);
    void trustedChanged(const bool &trusted);
    void blockedChanged(const bool &blocked);
    void servicesResolvedChanged(const bool &servicesResolved);

private slots:
    void onPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

    void onConnectDeviceFinished(QDBusPendingCallWatcher *call);
    void onDisconnectDeviceFinished(QDBusPendingCallWatcher *call);
    void onPairingFinished(QDBusPendingCallWatcher *call);
    void onCancelPairingFinished(QDBusPendingCallWatcher *call);

public slots:
    bool connectDevice();
    bool disconnectDevice();
    bool disconnectDeviceBlocking();
    bool requestPairing();
    bool cancelPairingRequest();
};

QDebug operator<<(QDebug debug, BluetoothDevice *device);


#endif // BLUETOOTHDEVICE_H
