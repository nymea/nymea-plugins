/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
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

#ifndef DEVICEPLUGINATAG_H
#define DEVICEPLUGINATAG_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"
#include "network/oauth2.h"
#include "atag.h"

#include <QHash>
#include <QTimer>
#include <QUdpSocket>
#include <QDateTime>

class DevicePluginAtag : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginatag.json")
    Q_INTERFACES(DevicePlugin)

public:
    struct AtagDiscoveryInfo {
        QHostAddress address;
        int port;
        QDateTime lastSeen;
    };

    explicit DevicePluginAtag();
    ~DevicePluginAtag() override;

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void startPairing(DevicePairingInfo *info) override;
    void confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;
    void postSetupDevice(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

private:
    QUdpSocket *m_udpSocket = nullptr;
    PluginTimer *m_pluginTimer = nullptr;
    QHash<QByteArray, AtagDiscoveryInfo> m_discoveredDevices;

    QHash<DeviceId, Atag *> m_unfinishedAtagConnections;
    QHash<DeviceId, DevicePairingInfo *> m_unfinishedDevicePairings;

    QHash<DeviceId, Atag *> m_atagConnections;
    QHash<Atag *, DeviceSetupInfo *> m_asyncDeviceSetup;

private slots:
    void onPluginTimer();

    //void onConnectionChanged(bool connected);
    //void onAuthenticationStatusChanged(bool authenticated);
    void onPairMessageReceived(Atag::AuthResult result);
};

#endif // DEVICEPLUGINATAG_H
