/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DEVICEPLUGINEDIMAX_H
#define DEVICEPLUGINEDIMAX_H

#include "devices/deviceplugin.h"

#include "plugintimer.h"
#include "network/networkaccessmanager.h"
#include "network/upnp/upnpdiscovery.h"

#include <QObject>
#include <QUuid>
#include <QXmlStreamReader>
#include <QtWebSockets/QtWebSockets>

class PluginTimer;

class DevicePluginEdimax: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginedimax.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginEdimax();
    ~DevicePluginEdimax() override;

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void startPairing(DevicePairingInfo *info) override;
    void confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;
    void deviceRemoved(Device *device) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
   //QHash<NetworkReply *, ActionInfo *> m_asyncActions;
   NetworkAccessManager *m_networkManager = nullptr;

   QUuid setPower(QHostAddress address, bool power);
};

#endif
