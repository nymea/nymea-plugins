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

#ifndef DEVICEPLUGINNETATMO_H
#define DEVICEPLUGINNETATMO_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"
#include "network/oauth2.h"
#include "netatmobasestation.h"
#include "netatmooutdoormodule.h"

#include <QHash>
#include <QTimer>

class DevicePluginNetatmo : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginnetatmo.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginNetatmo();
    ~DevicePluginNetatmo();

    void init() override;
    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;
    void postSetupDevice(Device *device) override;

private:
    PluginTimer *m_pluginTimer = nullptr;

    QHash<QString, QVariantMap> m_indoorStationInitData;
    QHash<QString, QVariantMap> m_outdoorStationInitData;

    QHash<OAuth2 *, Device *> m_authentications;
    QHash<NetatmoBaseStation *, Device *> m_indoorDevices;
    QHash<NetatmoOutdoorModule *, Device *> m_outdoorDevices;

    QHash<QNetworkReply *, Device *> m_refreshRequest;

    void refreshData(Device *device, const QString &token);
    void processRefreshData(const QVariantMap &data, Device *connectionDevice);

    Device *findIndoorDevice(const QString &macAddress);
    Device *findOutdoorDevice(const QString &macAddress);

private slots:
    void onPluginTimer();
    void onNetworkReplyFinished();
    void onAuthenticationChanged();
    void onIndoorStatesChanged();
    void onOutdoorStatesChanged();

};

#endif // DEVICEPLUGINNETATMO_H
