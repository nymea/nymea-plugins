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

#ifndef DEVICEPLUGINSHELLY_H
#define DEVICEPLUGINSHELLY_H

#include "devices/deviceplugin.h"

class ZeroConfServiceBrowser;

class MqttChannel;

class DevicePluginShelly: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginshelly.json")
    Q_INTERFACES(DevicePlugin)


public:
    explicit DevicePluginShelly();
    ~DevicePluginShelly() override;

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

private slots:
    void onClientConnected(MqttChannel* channel);
    void onClientDisconnected(MqttChannel* channel);
    void onPublishReceived(MqttChannel* channel, const QString &topic, const QByteArray &payload);

private:
    void setupShellyGateway(DeviceSetupInfo *info);
    void setupShellyChild(DeviceSetupInfo *info);

    QString getIP(Device *device) const;

private:
    ZeroConfServiceBrowser *m_zeroconfBrowser = nullptr;

    QHash<Device*, MqttChannel*> m_mqttChannels;

    QHash<DeviceClassId, StateTypeId> m_connectedStateTypesMap;
    QHash<DeviceClassId, StateTypeId> m_powerStateTypeMap;
    QHash<ActionTypeId, DeviceClassId> m_powerActionTypesMap;
    QHash<ActionTypeId, ParamTypeId> m_powerActionParamTypesMap;
};

#endif // DEVICEPLUGINSHELLY_H
