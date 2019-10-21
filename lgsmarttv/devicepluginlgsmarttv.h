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

#ifndef DEVICEPLUGINLGSMARTTV_H
#define DEVICEPLUGINLGSMARTTV_H

#include "tvdevice.h"
#include "plugintimer.h"
#include "devices/deviceplugin.h"
#include "network/upnp/upnpdevicedescriptor.h"

class DevicePluginLgSmartTv : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginlgsmarttv.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginLgSmartTv();
    ~DevicePluginLgSmartTv();

    void init() override;

    void discoverDevices(DeviceDiscoveryInfo *info) override;

    void startPairing(DevicePairingInfo *info) override;
    void confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret) override;

    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

    void executeAction(DeviceActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<TvDevice *, Device *> m_tvList;
    QHash<QString, QString> m_tvKeys;

    // update requests
    QHash<QNetworkReply *, Device *> m_volumeInfoRequests;
    QHash<QNetworkReply *, Device *> m_channelInfoRequests;

    void pairTvDevice(Device *device);
    void unpairTvDevice(Device *device);
    void refreshTv(Device *device);

private slots:
    void onPluginTimer();
    void onNetworkManagerReplyFinished();
    void stateChanged();
};

#endif // DEVICEPLUGINLGSMARTTV_H
