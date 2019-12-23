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

#ifndef DEVICEPLUGINAQI_H
#define DEVICEPLUGINAQI_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"
#include "network/networkaccessmanager.h"

#include <QTimer>
#include <QUrl>
#include <QHostAddress>

class DevicePluginAqi : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginaqi.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginAqi();

    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;
    void postSetupDevice(Device *device) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QString m_baseUrl = "https://api.waqi.info";
    QString m_apiKey;

    void getDataByIp();

private slots:
    void onPluginTimer();
};

#endif // DEVICEPLUGINAQI_H
