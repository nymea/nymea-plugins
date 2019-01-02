/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Michael Zanetti <michael.zanetti@nymea.io>          *
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

#ifndef DEVICEPLUGINANEL_H
#define DEVICEPLUGINANEL_H

#include "plugin/deviceplugin.h"
#include "devicemanager.h"

#include <QUdpSocket>

#include <QNetworkAccessManager>

class PluginTimer;

class DevicePluginAnel: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginanel.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginAnel();
    ~DevicePluginAnel();

    void init() override;
    DeviceManager::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;

private slots:
    void refreshStates();

private:
    void setConnectedState(Device *device, bool connected);

private:
    QNetworkAccessManager *m_nam = nullptr;
    PluginTimer *m_pollTimer = nullptr;

    QHash<DeviceClassId, StateTypeId> m_connectedStateTypeIdMap;
};

#endif // DEVICEPLUGINANEL_H
