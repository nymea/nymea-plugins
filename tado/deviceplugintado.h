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

#ifndef DEVICEPLUGINTADO_H
#define DEVICEPLUGINTADO_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"
#include "network/oauth2.h"
#include "tado.h"

#include <QHash>
#include <QTimer>

class DevicePluginTado : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugintado.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginTado();
    ~DevicePluginTado();

    void init() override;
    void startPairing(DevicePairingInfo *info) override;
    void confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;
    void postSetupDevice(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<DeviceId, Tado*> m_unfinishedTadoAccounts;
    QHash<DeviceId, DevicePairingInfo *> m_unfinishedDevicePairings;

    QHash<DeviceId, Tado*> m_tadoAccounts;
    QHash<Tado *, DeviceSetupInfo *> m_asyncDeviceSetup;

private slots:
    void onPluginTimer();

    void onConnectionChanged(bool connected);
    void onAuthenticationStatusChanged(bool authenticated);
    void onTokenReceived(Tado::Token token);
    void onHomesReceived(QList<Tado::Home> homes);
    void onZonesReceived(const QString &homeId, QList<Tado::Zone> zones);
    void onZoneStateReceived(const QString &homeId,const QString &zoneId, Tado::ZoneState sate);
};

#endif // DEVICEPLUGINTADO_H
