/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
