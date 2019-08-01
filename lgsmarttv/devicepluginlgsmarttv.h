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
    Device::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    Device::DeviceSetupStatus setupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    void postSetupDevice(Device *device) override;

    Device::DeviceError executeAction(Device *device, const Action &action) override;
    DevicePairingInfo pairDevice(DevicePairingInfo &devicePairingInfo) override;
    Device::DeviceSetupStatus confirmPairing(const PairingTransactionId &pairingTransactionId, const DeviceClassId &deviceClassId, const ParamList &params, const QString &username, const QString &secret) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<TvDevice *, Device *> m_tvList;
    QHash<QString, QString> m_tvKeys;

    // first pairing setup
    QHash<QNetworkReply *, PairingTransactionId> m_setupPairingTv;
    QHash<QNetworkReply *, PairingTransactionId> m_setupEndPairingTv;

    // async setup
    QHash<QNetworkReply *, Device *> m_asyncSetup;
    QList<QNetworkReply *> m_deleteTv;

    // action requests
    QHash<QNetworkReply *, ActionId> m_asyncActions;

    // update requests
    QHash<QNetworkReply *, Device *> m_volumeInfoRequests;
    QHash<QNetworkReply *, Device *> m_channelInfoRequests;

    void pairTvDevice(Device *device);
    void unpairTvDevice(Device *device);
    void refreshTv(Device *device);

private slots:
    void onPluginTimer();
    void onUpnpDiscoveryFinished();
    void onNetworkManagerReplyFinished();
    void stateChanged();
};

#endif // DEVICEPLUGINLGSMARTTV_H
