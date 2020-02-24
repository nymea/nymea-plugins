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

    QHash<DeviceClassId, ParamTypeId> m_idParamTypeMap;
    QHash<DeviceClassId, ParamTypeId> m_usernameParamTypeMap;
    QHash<DeviceClassId, ParamTypeId> m_passwordParamTypeMap;
    QHash<DeviceClassId, ParamTypeId> m_connectedDeviceParamTypeMap;
    QHash<DeviceClassId, ParamTypeId> m_connectedDevice2ParamTypeMap;
    QHash<DeviceClassId, ParamTypeId> m_channelParamTypeMap;

    QHash<DeviceClassId, StateTypeId> m_connectedStateTypesMap;
    QHash<DeviceClassId, StateTypeId> m_powerStateTypeMap;
    QHash<DeviceClassId, StateTypeId> m_currentPowerStateTypeMap;
    QHash<DeviceClassId, StateTypeId> m_totalEnergyConsumedStateTypeMap;
    QHash<DeviceClassId, StateTypeId> m_colorStateTypeMap;
    QHash<DeviceClassId, StateTypeId> m_colorTemperatureStateTypeMap;
    QHash<DeviceClassId, StateTypeId> m_brightnessStateTypeMap;

    QHash<ActionTypeId, DeviceClassId> m_rebootActionTypeMap;
    // Relay based power actions
    QHash<ActionTypeId, DeviceClassId> m_powerActionTypesMap;
    QHash<ActionTypeId, ParamTypeId> m_powerActionParamTypesMap;

    // Color JSON based power actions
    QHash<ActionTypeId, DeviceClassId> m_colorPowerActionTypesMap;
    QHash<ActionTypeId, ParamTypeId> m_colorPowerActionParamTypesMap;
    // Color actions
    QHash<ActionTypeId, DeviceClassId> m_colorActionTypesMap;
    QHash<ActionTypeId, ParamTypeId> m_colorActionParamTypesMap;
    // Color JSON brightness actions
    QHash<ActionTypeId, DeviceClassId> m_colorBrightnessActionTypesMap;
    QHash<ActionTypeId, ParamTypeId> m_colorBrightnessActionParamTypesMap;
    // Color temp
    QHash<ActionTypeId, DeviceClassId> m_colorTemperatureActionTypesMap;
    QHash<ActionTypeId, ParamTypeId> m_colorTemperatureActionParamTypesMap;

    // Dimmable based power actions
    QHash<ActionTypeId, DeviceClassId> m_dimmablePowerActionTypesMap;
    QHash<ActionTypeId, ParamTypeId> m_dimmablePowerActionParamTypesMap;
    // Dimmable based brightness actions
    QHash<ActionTypeId, DeviceClassId> m_dimmableBrightnessActionTypesMap;
    QHash<ActionTypeId, ParamTypeId> m_dimmableBrightnessActionParamTypesMap;

    // Roller shutter actions
    QHash<ActionTypeId, DeviceClassId> m_rollerOpenActionTypeMap;
    QHash<ActionTypeId, DeviceClassId> m_rollerCloseActionTypeMap;
    QHash<ActionTypeId, DeviceClassId> m_rollerStopActionTypeMap;
};

#endif // DEVICEPLUGINSHELLY_H
