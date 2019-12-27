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

#ifndef DEVICEPLUGINYEELIGHT_H
#define DEVICEPLUGINYEELIGHT_H

#include "devices/deviceplugin.h"
#include "plugintimer.h"
#include "network/networkaccessmanager.h"
#include "network/upnp/upnpdiscovery.h"
#include "yeelight.h"

#include <QTimer>

class DevicePluginYeelight : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginyeelight.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginYeelight();

    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;
    void deviceRemoved(Device *device) override;

private:
    class DiscoveryJob {
    public:
        UpnpDiscoveryReply* upnpReply;
        QNetworkReply* nUpnpReply;
        DeviceDescriptors results;
    };
    QHash<DeviceDiscoveryInfo*, DiscoveryJob*> m_discoveries;
    QList<QString> m_usedInterfaces;

    PluginTimer *m_pluginTimer = nullptr;
    QHash<int, DeviceActionInfo *> m_asyncActions;
    QHash<DeviceId, Yeelight *> m_yeelightConnections;

private slots:
    void onDeviceNameChanged();
    void onConnectionChanged(bool connected);
    void onRequestExecuted(int requestId, bool success);
    void onPropertyListReceived(QVariantList value);
};

#endif // DEVICEPLUGINYEELIGHT_H
