/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stuerz <simon.stuerz@guh.io>                  *
 *  Copyright (C) 2016 Christian Stachowitz                                *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  Nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DEVICEPLUGINKEBA_H
#define DEVICEPLUGINKEBA_H

#include "plugin/deviceplugin.h"
#include "devicemanager.h"
#include "plugintimer.h"

#include <QHash>
#include <QNetworkReply>
#include <QUdpSocket>

class DevicePluginKeba : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginkeba.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginKeba();
    ~DevicePluginKeba();

    void init() override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;

    void postSetupDevice(Device* device) override;
    void deviceRemoved(Device* device) override;

    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;
    void updateData();

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<QHostAddress, Device *> m_kebaDevices;
    QUdpSocket *m_kebaSocket;

private slots:
    void readPendingDatagrams();

};

#endif // DEVICEPLUGINKEBA_H
