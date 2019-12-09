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

#ifndef DEVICEPLUGINTASMOTA_H
#define DEVICEPLUGINTASMOTA_H

#include "devices/deviceplugin.h"

class MqttChannel;

class DevicePluginTasmota: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugintasmota.json")
    Q_INTERFACES(DevicePlugin)


public:
    explicit DevicePluginTasmota();
    ~DevicePluginTasmota();

    void init() override;
    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

private slots:
    void onClientConnected(MqttChannel *channel);
    void onClientDisconnected(MqttChannel *channel);
    void onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload);

private:
    QHash<Device*, MqttChannel*> m_mqttChannels;

    // Helpers for parent devices (the ones starting with sonoff)
    QHash<DeviceClassId, ParamTypeId> m_ipAddressParamTypeMap;
    QHash<DeviceClassId, QList<ParamTypeId> > m_attachedDeviceParamTypeIdMap;

    // Helpers for child devices (virtual ones, starting with tasmota)
    QHash<DeviceClassId, ParamTypeId> m_channelParamTypeMap;
    QHash<DeviceClassId, ParamTypeId> m_openingChannelParamTypeMap;
    QHash<DeviceClassId, ParamTypeId> m_closingChannelParamTypeMap;
    QHash<DeviceClassId, StateTypeId> m_powerStateTypeMap;

    QHash<DeviceClassId, ActionTypeId> m_closableOpenActionTypeMap;
    QHash<DeviceClassId, ActionTypeId> m_closableCloseActionTypeMap;
    QHash<DeviceClassId, ActionTypeId> m_closableStopActionTypeMap;

    // Helpers for both devices
    QHash<DeviceClassId, StateTypeId> m_connectedStateTypeMap;
};

#endif // DEVICEPLUGINTASMOTA_H
