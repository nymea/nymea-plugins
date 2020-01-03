/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by copyright law, and
* remains the property of nymea GmbH. All rights, including reproduction, publication,
* editing and translation, are reserved. The use of this project is subject to the terms of a
* license agreement to be concluded with nymea GmbH in accordance with the terms
* of use of nymea GmbH, available under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* This project may also contain libraries licensed under the open source software license GNU GPL v.3.
* Alternatively, this project may be redistributed and/or modified under the terms of the GNU
* Lesser General Public License as published by the Free Software Foundation; version 3.
* this project is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along with this project.
* If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under contact@nymea.io
* or see our FAQ/Licensing Information on https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
    QHash<QUuid, DeviceActionInfo *> m_asyncActions;

private slots:
    void onPluginTimer();

    void onConnectionChanged(bool connected);
    void onAuthenticationStatusChanged(bool authenticated);
    void onTokenReceived(Tado::Token token);
    void onHomesReceived(QList<Tado::Home> homes);
    void onZonesReceived(const QString &homeId, QList<Tado::Zone> zones);
    void onZoneStateReceived(const QString &homeId,const QString &zoneId, Tado::ZoneState sate);
    void onOverlayReceived(const QString &homeId, const QString &zoneId, const Tado::Overlay &overlay);
};

#endif // DEVICEPLUGINTADO_H
