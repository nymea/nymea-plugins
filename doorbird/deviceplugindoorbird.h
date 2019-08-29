/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Michael Zanetti <michael.zanetti@nymea.io>          *
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

#ifndef DEVICEPLUGINDOORBIRD_H
#define DEVICEPLUGINDOORBIRD_H

#include "devices/deviceplugin.h"
#include "devices/devicemanager.h"

class QNetworkAccessManager;
class QNetworkReply;

class DevicePluginDoorbird: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugindoorbird.json")
    Q_INTERFACES(DevicePlugin)


public:
    explicit DevicePluginDoorbird();
    ~DevicePluginDoorbird() override;

    Device::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;

    Device::DeviceSetupStatus setupDevice(Device *device) override;
    Device::DeviceError executeAction(Device *device, const Action &action) override;

    void connectToEventMonitor(Device *device);
private:

    QNetworkAccessManager *m_nam = nullptr;

    QHash<QNetworkReply*, Device*> m_networkRequests;
    QHash<Device*, QByteArray> m_readBuffers;
};

#endif // DEVICEPLUGINDOORBIRD_H
