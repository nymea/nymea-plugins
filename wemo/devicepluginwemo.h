/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

#ifndef DEVICEPLUGINWEMO_H
#define DEVICEPLUGINWEMO_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"

#include <QNetworkReply>

class DevicePluginWemo : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginwemo.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginWemo();
    ~DevicePluginWemo();

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void executeAction(DeviceActionInfo *info) override;
    void deviceRemoved(Device *device) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<QNetworkReply *, Device *> m_refreshReplies;

    void refresh(Device* device);
    bool setPower(Device *device, const bool &power, const ActionId &actionId);

    void processRefreshData(const QByteArray &data, Device *device);

private slots:
    void onNetworkReplyFinished();
    void onPluginTimer();
    void onUpnpDiscoveryFinished();
    void onUpnpNotifyReceived(const QByteArray &notification);

};

#endif // DEVICEPLUGINWEMO_H
