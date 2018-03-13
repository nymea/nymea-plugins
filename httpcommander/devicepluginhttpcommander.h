/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
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

#ifndef DEVICEPLUGINDHTTPCOMMANDER_H
#define DEVICEPLUGINDHTTPCOMMANDER_H

#include "plugin/deviceplugin.h"
#include "devicemanager.h"
#include "plugintimer.h"

#include <QNetworkReply>

class DevicePluginHttpCommander : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginhttpcommander.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginHttpCommander();
    ~DevicePluginHttpCommander();

    void init() override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    void  postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<QNetworkReply *, Device *> m_httpRequests;

    void makeGetCall(Device *device);

private slots:
    void onPluginTimer();

    void onGetRequestFinished();
    void onPostRequestFinished();
    void onPutRequestFinished();
};

#endif // DEVICEPLUGINHTTPCOMMANDER_H
