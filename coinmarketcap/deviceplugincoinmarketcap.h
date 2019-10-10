/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io         *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DEVICEPLUGINCOINMARKETCAP_H
#define DEVICEPLUGINCOINMARKETCAP_H

#include "devices/deviceplugin.h"
#include "devices/devicemanager.h"
#include "plugintimer.h"

#include <QNetworkReply>
#include <QHostInfo>

class DevicePluginCoinMarketCap : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugincoinmarketcap.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginCoinMarketCap();

    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;

private:
    PluginTimer *m_pluginTimer = nullptr;

    QHash<QUrl, Device *> m_priceRequests;
    QHash<QNetworkReply *, Device *>  m_httpRequests;

    void getPriceCall(Device *device);

private slots:
    void onPluginTimer();
    void onPriceCallFinished();
};

#endif // DEVICEPLUGINCOINMARKETCAP_H
