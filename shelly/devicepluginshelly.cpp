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

#include "devicepluginshelly.h"
#include "plugininfo.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QHostAddress>
#include <QJsonDocument>
#include <QColor>

#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"
#include "network/mqtt/mqttprovider.h"
#include "network/mqtt/mqttchannel.h"

#include "network/zeroconf/zeroconfservicebrowser.h"
#include "platform/platformzeroconfcontroller.h"

DevicePluginShelly::DevicePluginShelly()
{
    // Device param types
    m_idParamTypeMap[shelly1DeviceClassId] = shelly1DeviceIdParamTypeId;
    m_idParamTypeMap[shelly1pmDeviceClassId] = shelly1pmDeviceIdParamTypeId;
    m_idParamTypeMap[shellyPlugDeviceClassId] = shellyPlugDeviceIdParamTypeId;
    m_idParamTypeMap[shellyRgbw2DeviceClassId] = shellyRgbw2DeviceIdParamTypeId;
    m_idParamTypeMap[shellyDimmerDeviceClassId] = shellyDimmerDeviceIdParamTypeId;
    m_idParamTypeMap[shelly25DeviceClassId] = shelly25DeviceIdParamTypeId;

    m_usernameParamTypeMap[shelly1DeviceClassId] = shelly1DeviceUsernameParamTypeId;
    m_usernameParamTypeMap[shelly1pmDeviceClassId] = shelly1pmDeviceUsernameParamTypeId;
    m_usernameParamTypeMap[shellyPlugDeviceClassId] = shellyPlugDeviceUsernameParamTypeId;
    m_usernameParamTypeMap[shellyRgbw2DeviceClassId] = shellyRgbw2DeviceUsernameParamTypeId;
    m_usernameParamTypeMap[shellyDimmerDeviceClassId] = shellyDimmerDeviceUsernameParamTypeId;
    m_usernameParamTypeMap[shelly25DeviceClassId] = shelly25DeviceUsernameParamTypeId;

    m_passwordParamTypeMap[shelly1DeviceClassId] = shelly1DevicePasswordParamTypeId;
    m_passwordParamTypeMap[shelly1pmDeviceClassId] = shelly1pmDevicePasswordParamTypeId;
    m_passwordParamTypeMap[shellyPlugDeviceClassId] = shellyPlugDevicePasswordParamTypeId;
    m_passwordParamTypeMap[shellyRgbw2DeviceClassId] = shellyRgbw2DevicePasswordParamTypeId;
    m_passwordParamTypeMap[shellyDimmerDeviceClassId] = shellyDimmerDevicePasswordParamTypeId;
    m_passwordParamTypeMap[shelly25DeviceClassId] = shelly25DevicePasswordParamTypeId;

    m_connectedDeviceParamTypeMap[shelly1DeviceClassId] = shelly1DeviceConnectedDeviceParamTypeId;
    m_connectedDeviceParamTypeMap[shelly1pmDeviceClassId] = shelly1pmDeviceConnectedDeviceParamTypeId;
    m_connectedDeviceParamTypeMap[shellyPlugDeviceClassId] = shellyPlugDeviceConnectedDeviceParamTypeId;
    m_connectedDeviceParamTypeMap[shelly25DeviceClassId] = shelly25DeviceConnectedDevice1ParamTypeId;

    m_connectedDevice2ParamTypeMap[shelly25DeviceClassId] = shelly25DeviceConnectedDevice2ParamTypeId;

    m_channelParamTypeMap[shellyGenericDeviceClassId] = shellyGenericDeviceChannelParamTypeId;
    m_channelParamTypeMap[shellyLightDeviceClassId] = shellyLightDeviceChannelParamTypeId;
    m_channelParamTypeMap[shellySocketDeviceClassId] = shellySocketDeviceChannelParamTypeId;
    m_channelParamTypeMap[shellyGenericPMDeviceClassId] = shellyGenericPMDeviceChannelParamTypeId;
    m_channelParamTypeMap[shellyLightPMDeviceClassId] = shellyLightPMDeviceChannelParamTypeId;
    m_channelParamTypeMap[shellySocketPMDeviceClassId] = shellySocketPMDeviceChannelParamTypeId;
    m_channelParamTypeMap[shellyRollerDeviceClassId] = shellyRollerDeviceChannelParamTypeId;

    // States
    m_connectedStateTypesMap[shelly1DeviceClassId] = shelly1ConnectedStateTypeId;
    m_connectedStateTypesMap[shelly1pmDeviceClassId] = shelly1pmConnectedStateTypeId;
    m_connectedStateTypesMap[shelly25DeviceClassId] = shelly25ConnectedStateTypeId;
    m_connectedStateTypesMap[shellyPlugDeviceClassId] = shellyPlugConnectedStateTypeId;
    m_connectedStateTypesMap[shellyRgbw2DeviceClassId] = shellyRgbw2ConnectedStateTypeId;
    m_connectedStateTypesMap[shellyDimmerDeviceClassId] = shellyDimmerConnectedStateTypeId;
    m_connectedStateTypesMap[shellySwitchDeviceClassId] = shellySwitchConnectedStateTypeId;
    m_connectedStateTypesMap[shellyGenericDeviceClassId] = shellyGenericConnectedStateTypeId;
    m_connectedStateTypesMap[shellyLightDeviceClassId] = shellyLightConnectedStateTypeId;
    m_connectedStateTypesMap[shellySocketDeviceClassId] = shellySocketConnectedStateTypeId;
    m_connectedStateTypesMap[shellyGenericPMDeviceClassId] = shellyGenericPMConnectedStateTypeId;
    m_connectedStateTypesMap[shellyLightPMDeviceClassId] = shellyLightPMConnectedStateTypeId;
    m_connectedStateTypesMap[shellySocketPMDeviceClassId] = shellySocketPMConnectedStateTypeId;
    m_connectedStateTypesMap[shellyRollerDeviceClassId] = shellyRollerConnectedStateTypeId;

    m_powerStateTypeMap[shellyPlugDeviceClassId] = shellyPlugPowerStateTypeId;
    m_powerStateTypeMap[shellyRgbw2DeviceClassId] = shellyRgbw2PowerStateTypeId;
    m_powerStateTypeMap[shellyDimmerDeviceClassId] = shellyDimmerPowerStateTypeId;
    m_powerStateTypeMap[shellyGenericDeviceClassId] = shellyGenericPowerStateTypeId;
    m_powerStateTypeMap[shellyLightDeviceClassId] = shellyLightPowerStateTypeId;
    m_powerStateTypeMap[shellySocketDeviceClassId] = shellySocketPowerStateTypeId;
    m_powerStateTypeMap[shellyGenericPMDeviceClassId] = shellyGenericPMPowerStateTypeId;
    m_powerStateTypeMap[shellyLightPMDeviceClassId] = shellyLightPMPowerStateTypeId;
    m_powerStateTypeMap[shellySocketPMDeviceClassId] = shellySocketPMPowerStateTypeId;

    m_currentPowerStateTypeMap[shellyPlugDeviceClassId] = shellyPlugCurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellyRgbw2DeviceClassId] = shellyRgbw2CurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellyDimmerDeviceClassId] = shellyDimmerCurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellyGenericPMDeviceClassId] = shellyGenericPMCurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellyLightPMDeviceClassId] = shellyLightPMCurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellySocketPMDeviceClassId] = shellySocketPMCurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellyRollerDeviceClassId] = shellyRollerCurrentPowerStateTypeId;

    m_totalEnergyConsumedStateTypeMap[shellyPlugDeviceClassId] = shellyPlugTotalEnergyConsumedStateTypeId;
    m_totalEnergyConsumedStateTypeMap[shellyGenericPMDeviceClassId] = shellyGenericPMTotalEnergyConsumedStateTypeId;
    m_totalEnergyConsumedStateTypeMap[shellyLightPMDeviceClassId] = shellyLightPMTotalEnergyConsumedStateTypeId;
    m_totalEnergyConsumedStateTypeMap[shellySocketPMDeviceClassId] = shellySocketPMTotalEnergyConsumedStateTypeId;
    m_totalEnergyConsumedStateTypeMap[shellyRollerDeviceClassId] = shellyRollerTotalEnergyConsumedStateTypeId;

    m_colorStateTypeMap[shellyRgbw2DeviceClassId] = shellyRgbw2ColorStateTypeId;
    m_colorTemperatureStateTypeMap[shellyRgbw2DeviceClassId] = shellyRgbw2ColorTemperatureStateTypeId;

    m_brightnessStateTypeMap[shellyRgbw2DeviceClassId] = shellyRgbw2BrightnessStateTypeId;
    m_brightnessStateTypeMap[shellyDimmerDeviceClassId] = shellyDimmerBrightnessStateTypeId;

    // Actions and their params
    m_rebootActionTypeMap[shelly1RebootActionTypeId] = shelly1DeviceClassId;
    m_rebootActionTypeMap[shelly1pmRebootActionTypeId] = shelly1pmDeviceClassId;
    m_rebootActionTypeMap[shellyPlugRebootActionTypeId] = shellyPlugDeviceClassId;
    m_rebootActionTypeMap[shellyRgbw2RebootActionTypeId] = shellyRgbw2DeviceClassId;
    m_rebootActionTypeMap[shellyDimmerRebootActionTypeId] = shellyDimmerDeviceClassId;
    m_rebootActionTypeMap[shelly25RebootActionTypeId] = shelly25DeviceClassId;

    m_powerActionTypesMap[shellyPlugPowerActionTypeId] = shellyPlugDeviceClassId;
    m_powerActionTypesMap[shellyGenericPowerActionTypeId] = shellyGenericDeviceClassId;
    m_powerActionTypesMap[shellyLightPowerActionTypeId] = shellyLightDeviceClassId;
    m_powerActionTypesMap[shellySocketPowerActionTypeId] = shellySocketDeviceClassId;
    m_powerActionTypesMap[shellyGenericPMPowerActionTypeId] = shellyGenericPMDeviceClassId;
    m_powerActionTypesMap[shellyLightPMPowerActionTypeId] = shellyLightPMDeviceClassId;
    m_powerActionTypesMap[shellySocketPMPowerActionTypeId] = shellySocketPMDeviceClassId;

    m_powerActionParamTypesMap[shellyPlugPowerActionTypeId] = shellyPlugPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellyGenericPowerActionTypeId] = shellyGenericPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellyLightPowerActionTypeId] = shellyLightPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellySocketPowerActionTypeId] = shellySocketPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellyGenericPMPowerActionTypeId] = shellyGenericPMPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellyLightPMPowerActionTypeId] = shellyLightPMPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellySocketPMPowerActionTypeId] = shellySocketPMPowerActionPowerParamTypeId;

    m_colorPowerActionTypesMap[shellyRgbw2PowerActionTypeId] = shellyRgbw2DeviceClassId;
    m_colorPowerActionParamTypesMap[shellyRgbw2PowerActionPowerParamTypeId] = shellyRgbw2PowerActionTypeId;

    m_colorActionTypesMap[shellyRgbw2ColorActionTypeId] = shellyRgbw2DeviceClassId;
    m_colorActionParamTypesMap[shellyRgbw2ColorActionTypeId] = shellyRgbw2ColorActionTypeId;

    m_colorBrightnessActionTypesMap[shellyRgbw2BrightnessActionTypeId] = shellyRgbw2DeviceClassId;
    m_colorBrightnessActionParamTypesMap[shellyRgbw2BrightnessActionBrightnessParamTypeId] = shellyRgbw2BrightnessActionTypeId;

    m_colorTemperatureActionTypesMap[shellyRgbw2ColorTemperatureActionTypeId] = shellyRgbw2DeviceClassId;
    m_colorTemperatureActionParamTypesMap[shellyRgbw2ColorTemperatureActionTypeId] = shellyRgbw2ColorTemperatureActionColorTemperatureParamTypeId;

    m_dimmablePowerActionTypesMap[shellyDimmerPowerActionTypeId] = shellyDimmerDeviceClassId;
    m_dimmablePowerActionParamTypesMap[shellyDimmerPowerActionTypeId] = shellyDimmerPowerActionPowerParamTypeId;

    m_dimmableBrightnessActionTypesMap[shellyDimmerBrightnessActionTypeId] = shellyDimmerDeviceClassId;
    m_dimmableBrightnessActionParamTypesMap[shellyDimmerBrightnessActionTypeId] = shellyDimmerBrightnessActionBrightnessParamTypeId;

    m_rollerOpenActionTypeMap[shellyRollerOpenActionTypeId] = shellyRollerDeviceClassId;
    m_rollerCloseActionTypeMap[shellyRollerCloseActionTypeId] = shellyRollerDeviceClassId;
    m_rollerStopActionTypeMap[shellyRollerStopActionTypeId] = shellyRollerDeviceClassId;
}

DevicePluginShelly::~DevicePluginShelly()
{
}

void DevicePluginShelly::init()
{
    m_zeroconfBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_http._tcp");
}

void DevicePluginShelly::discoverDevices(DeviceDiscoveryInfo *info)
{
    foreach (const ZeroConfServiceEntry &entry, m_zeroconfBrowser->serviceEntries()) {
//        qCDebug(dcShelly()) << "Have entry" << entry;
        QRegExp namePattern;
        if (info->deviceClassId() == shelly1DeviceClassId) {
            namePattern = QRegExp("^shelly1-[0-9A-Z]+$");
        } else if (info->deviceClassId() == shelly1pmDeviceClassId) {
            namePattern = QRegExp("^shelly1pm-[0-9A-Z]+$");
        } else if (info->deviceClassId() == shellyPlugDeviceClassId) {
            namePattern = QRegExp("^shellyplug(-s)?-[0-9A-Z]+$");
        } else if (info->deviceClassId() == shellyRgbw2DeviceClassId) {
            namePattern = QRegExp("^shellyrgbw2-[0-9A-Z]+$");
        } else if (info->deviceClassId() == shellyDimmerDeviceClassId) {
            namePattern = QRegExp("^shellydimmer-[0-9A-Z]+$");
        } else if (info->deviceClassId() == shelly25DeviceClassId) {
            namePattern = QRegExp("^shellyswitch25-[0-9A-Z]+$");
        }
        if (!entry.name().contains(namePattern)) {
            continue;
        }

        DeviceDescriptor descriptor(info->deviceClassId(), entry.name(), entry.hostAddress().toString());
        ParamList params;
        params << Param(m_idParamTypeMap.value(info->deviceClassId()), entry.name());
        descriptor.setParams(params);

        Device *existingDevice = myDevices().findByParams(params);
        if (existingDevice) {
            descriptor.setDeviceId(existingDevice->id());
        }

        info->addDeviceDescriptor(descriptor);
        qCDebug(dcShelly()) << "Found shelly device!" << entry;

    }

    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginShelly::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == shelly1DeviceClassId
            || device->deviceClassId() == shelly1pmDeviceClassId
            || device->deviceClassId() == shellyPlugDeviceClassId
            || device->deviceClassId() == shellyRgbw2DeviceClassId
            || device->deviceClassId() == shellyDimmerDeviceClassId
            || device->deviceClassId() == shelly25DeviceClassId) {
        setupShellyGateway(info);
        return;
    }

    setupShellyChild(info);
}

void DevicePluginShelly::deviceRemoved(Device *device)
{
    if (m_mqttChannels.contains(device)) {
        hardwareManager()->mqttProvider()->releaseChannel(m_mqttChannels.take(device));
    }
    qCDebug(dcShelly()) << "Device removed" << device->name();
}

void DevicePluginShelly::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (m_rebootActionTypeMap.contains(action.actionTypeId())) {
        QUrl url;
        url.setScheme("http");
        url.setHost(getIP(info->device()));
        url.setPath("/reboot");
        url.setUserName(device->paramValue(m_usernameParamTypeMap.value(device->deviceClassId())).toString());
        url.setPassword(device->paramValue(m_passwordParamTypeMap.value(device->deviceClassId())).toString());
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply](){
            info->finish(reply->error() == QNetworkReply::NoError ? Device::DeviceErrorNoError : Device::DeviceErrorHardwareFailure);
        });
        return;
    }

    if (m_powerActionTypesMap.contains(action.actionTypeId())) {
        // If the main shelly has a power action (e.g. Shelly Plug, there is no parentId)
        Device *parentDevice = device->parentId().isNull() ? device : myDevices().findById(device->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(m_idParamTypeMap.value(parentDevice->deviceClassId())).toString();
        int relay = 1;
        if (m_channelParamTypeMap.contains(device->deviceClassId())) {
            relay = device->paramValue(m_channelParamTypeMap.value(device->deviceClassId())).toInt();
        }
        ParamTypeId powerParamTypeId = m_powerActionParamTypesMap.value(action.actionTypeId());
        bool on = action.param(powerParamTypeId).value().toBool();
        channel->publish(QString("shellies/%1/relay/%2/command").arg(shellyId).arg(relay - 1), on ? "on" : "off");
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    if (m_colorPowerActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(device);
        QString shellyId = info->device()->paramValue(m_idParamTypeMap.value(info->device()->deviceClassId())).toString();
        ParamTypeId colorPowerParamTypeId = m_colorPowerActionParamTypesMap.value(action.actionTypeId());
        bool on = action.param(colorPowerParamTypeId).value().toBool();
        channel->publish("shellies/" + shellyId + "/color/0/command", on ? "on" : "off");
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    if (m_colorActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(device);
        QString shellyId = info->device()->paramValue(m_idParamTypeMap.value(info->device()->deviceClassId())).toString();
        ParamTypeId colorParamTypeId = m_colorActionParamTypesMap.value(action.actionTypeId());
        QColor color = action.param(colorParamTypeId).value().value<QColor>();
        QVariantMap data;
        data.insert("turn", "on"); // Should we really?
        data.insert("red", color.red());
        data.insert("green", color.green());
        data.insert("blue", color.blue());
        data.insert("white", 0);
        QJsonDocument jsonDoc = QJsonDocument::fromVariant(data);
        channel->publish("shellies/" + shellyId + "/color/0/set", jsonDoc.toJson());
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    if (m_colorTemperatureStateTypeMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(device);
        QString shellyId = info->device()->paramValue(m_idParamTypeMap.value(info->device()->deviceClassId())).toString();
        ParamTypeId colorTemperatureParamTypeId = m_colorTemperatureActionParamTypesMap.value(action.actionTypeId());
        int ct = action.param(colorTemperatureParamTypeId).value().toInt();
        QVariantMap data;
        data.insert("turn", "on"); // Should we really?
        data.insert("red", qMin(255, ct * 255 / 50));
        data.insert("green", 0);
        data.insert("blue", qMax(0, ct - 50) * 255 / 50);
        data.insert("white", 255);
        QJsonDocument jsonDoc = QJsonDocument::fromVariant(data);
        channel->publish("shellies/" + shellyId + "/color/0/set", jsonDoc.toJson());
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    if (m_colorBrightnessActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(device);
        QString shellyId = info->device()->paramValue(m_idParamTypeMap.value(info->device()->deviceClassId())).toString();
        ParamTypeId brightnessParamTypeId = m_colorBrightnessActionParamTypesMap.value(action.actionTypeId());
        int brightness = action.param(brightnessParamTypeId).value().toInt();
        QVariantMap data;
        data.insert("turn", "on"); // Should we really?
        data.insert("gain", brightness);
        QJsonDocument jsonDoc = QJsonDocument::fromVariant(data);
        channel->publish("shellies/" + shellyId + "/color/0/set", jsonDoc.toJson());
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    qCDebug(dcShelly()) << "Power action";
    if (m_dimmablePowerActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(device);
        QString shellyId = info->device()->paramValue(m_idParamTypeMap.value(info->device()->deviceClassId())).toString();
        ParamTypeId powerParamTypeId = m_dimmablePowerActionParamTypesMap.value(action.actionTypeId());
        bool on = action.param(powerParamTypeId).value().toBool();
        channel->publish("shellies/" + shellyId + "/light/0/command", on ? "on" : "off");
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    if (m_dimmableBrightnessActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(device);
        QString shellyId = info->device()->paramValue(m_idParamTypeMap.value(info->device()->deviceClassId())).toString();
        ParamTypeId brightnessParamTypeId = m_dimmableBrightnessActionParamTypesMap.value(action.actionTypeId());
        int brightness = action.param(brightnessParamTypeId).value().toInt();
        QVariantMap data;
        data.insert("turn", "on"); // Should we really?
        data.insert("brightness", brightness);
        QJsonDocument jsonDoc = QJsonDocument::fromVariant(data);
        channel->publish("shellies/" + shellyId + "/light/0/set", jsonDoc.toJson());
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    if (m_rollerOpenActionTypeMap.contains(action.actionTypeId())) {
        Device *parentDevice = myDevices().findById(device->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(m_idParamTypeMap.value(parentDevice->deviceClassId())).toString();
        channel->publish("shellies/" + shellyId + "/roller/0/command", "open");
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    if (m_rollerCloseActionTypeMap.contains(action.actionTypeId())) {
        Device *parentDevice = myDevices().findById(device->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(m_idParamTypeMap.value(parentDevice->deviceClassId())).toString();
        channel->publish("shellies/" + shellyId + "/roller/0/command", "close");
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    if (m_rollerStopActionTypeMap.contains(action.actionTypeId())) {
        Device *parentDevice = myDevices().findById(device->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(m_idParamTypeMap.value(parentDevice->deviceClassId())).toString();
        channel->publish("shellies/" + shellyId + "/roller/0/command", "stop");
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    if (action.actionTypeId() == shellyRollerCalibrateActionTypeId) {
        Device *parentDevice = myDevices().findById(device->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(m_idParamTypeMap.value(parentDevice->deviceClassId())).toString();
        channel->publish("shellies/" + shellyId + "/roller/0/command", "rc");
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    qCWarning(dcShelly()) << "Unhandled execute action call for device" << device;
}

void DevicePluginShelly::onClientConnected(MqttChannel *channel)
{
    Device *device = m_mqttChannels.key(channel);
    if (!device) {
        qCWarning(dcShelly()) << "Received a client connect for a device we don't know!";
        return;
    }
    device->setStateValue(m_connectedStateTypesMap.value(device->deviceClassId()), true);

    foreach (Device *child, myDevices().filterByParentDeviceId(device->id())) {
        child->setStateValue(m_connectedStateTypesMap[child->deviceClassId()], true);
    }
}

void DevicePluginShelly::onClientDisconnected(MqttChannel *channel)
{
    Device *device = m_mqttChannels.key(channel);
    if (!device) {
        qCWarning(dcShelly()) << "Received a client disconnect for a device we don't know!";
        return;
    }
    device->setStateValue(m_connectedStateTypesMap.value(device->deviceClassId()), false);

    foreach (Device *child, myDevices().filterByParentDeviceId(device->id())) {
        child->setStateValue(m_connectedStateTypesMap[child->deviceClassId()], false);
    }
}

void DevicePluginShelly::onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload)
{
    Device *device = m_mqttChannels.key(channel);
    if (!device) {
        qCWarning(dcShelly()) << "Received a publish message for a device we don't know!";
        return;
    }

    qCDebug(dcShelly()) << "Publish received from" << device->name() << topic << payload;

    QString shellyId = device->paramValue(m_idParamTypeMap.value(device->deviceClassId())).toString();
    if (topic.startsWith("shellies/" + shellyId + "/input/")) {
        int channel = topic.split("/").last().toInt();
        // "1" or "0"
        // Emit event button pressed
        bool on = payload == "1";
        foreach (Device *child, myDevices().filterByParentDeviceId(device->id())) {
            if (child->deviceClassId() == shellySwitchDeviceClassId && child->paramValue(shellySwitchDeviceChannelParamTypeId).toInt() == channel + 1) {
                if (child->stateValue(shellySwitchPowerStateTypeId).toBool() != on) {
                    child->setStateValue(shellySwitchPowerStateTypeId, on);
                    emit emitEvent(Event(shellySwitchPressedEventTypeId, child->id()));
                }
            }
        }
    }

    QRegExp topicMatcher = QRegExp("shellies/" + shellyId + "/relay/[0-1]");
    if (topicMatcher.exactMatch(topic)) {
        QStringList parts = topic.split("/");
        int channel = parts.at(3).toInt();
        bool on = payload == "on";

        // If the shelly main device has a power state (e.g. Shelly Plug)
        if (m_powerStateTypeMap.contains(device->deviceClassId())) {
            device->setStateValue(m_powerStateTypeMap.value(device->deviceClassId()), on);
        }

        // And switch all childs of this shelly too
        foreach (Device *child, myDevices().filterByParentDeviceId(device->id())) {
            if (m_powerStateTypeMap.contains(child->deviceClassId())) {
                ParamTypeId channelParamTypeId = m_channelParamTypeMap.value(child->deviceClassId());
                if (child->paramValue(channelParamTypeId).toInt() == channel + 1) {
                    child->setStateValue(m_powerStateTypeMap.value(child->deviceClassId()), on);
                }
            }
        }
    }

    topicMatcher = QRegExp("shellies/" + shellyId + "/(relay|roller)/[0-1]/power");
    if (topicMatcher.exactMatch(topic)) {
        QStringList parts = topic.split("/");
        int channel = parts.at(3).toInt();
        double power = payload.toDouble();
        // If this gateway device supports power measuring (e.g. Shelly Plug S) set it directly here
        if (m_currentPowerStateTypeMap.contains(device->deviceClassId())) {
            device->setStateValue(m_currentPowerStateTypeMap.value(device->deviceClassId()), power);
        }
        // For multi-channel devices, power measurements are per-channel, so, find the child device
        foreach (Device *child, myDevices().filterByParentDeviceId(device->id()).filterByInterface("extendedsmartmeterconsumer")) {
            ParamTypeId channelParamTypeId = m_channelParamTypeMap.value(child->deviceClassId());
            if (child->paramValue(channelParamTypeId).toInt() == channel + 1) {
                child->setStateValue(m_currentPowerStateTypeMap.value(child->deviceClassId()), power);
            }
        }
    }

    topicMatcher = QRegExp("shellies/" + shellyId + "/(relay|roller)/[0-1]/energy");
    if (topicMatcher.exactMatch(topic)) {
        QStringList parts = topic.split("/");
        int channel = parts.at(3).toInt();
        // W/min => kW/h
        double energy = payload.toDouble() / 1000 / 60;
        // If this gateway device supports energy measuring (e.g. Shelly Plug S) set it directly here
        if (m_totalEnergyConsumedStateTypeMap.contains(device->deviceClassId())) {
            device->setStateValue(m_totalEnergyConsumedStateTypeMap.value(device->deviceClassId()), energy);
        }
        // For multi-channel devices, power measurements are per-channel, so, find the child device
        foreach (Device *child, myDevices().filterByParentDeviceId(device->id()).filterByInterface("extendedsmartmeterconsumer")) {
            ParamTypeId channelParamTypeId = m_channelParamTypeMap.value(child->deviceClassId());
            if (child->paramValue(channelParamTypeId).toInt() == channel + 1) {
                child->setStateValue(m_totalEnergyConsumedStateTypeMap.value(child->deviceClassId()), energy);
            }
        }
    }

    if (topic == "shellies/" + shellyId + "/color/0") {
        bool on = payload == "on";
        if (m_powerStateTypeMap.contains(device->deviceClassId())) {
            device->setStateValue(m_powerStateTypeMap.value(device->deviceClassId()), on);
        }
    }

    if (topic == "shellies/" + shellyId + "/color/0/status") {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcShelly()) << "Error parsing JSON from Shelly:" << error.error << error.errorString() << payload;
            return;
        }
        QVariantMap statusMap = jsonDoc.toVariant().toMap();
        if (m_colorStateTypeMap.contains(device->deviceClassId())) {
            QColor color = QColor(statusMap.value("red").toInt(), statusMap.value("green").toInt(), statusMap.value("blue").toInt());
            device->setStateValue(m_colorStateTypeMap.value(device->deviceClassId()), color);
        }
        if (m_brightnessStateTypeMap.contains(device->deviceClassId())) {
            int brightness = statusMap.value("gain").toInt();
            device->setStateValue(m_brightnessStateTypeMap.value(device->deviceClassId()), brightness);
        }
        if (m_currentPowerStateTypeMap.contains(device->deviceClassId())) {
            double power = statusMap.value("power").toDouble();
            device->setStateValue(m_currentPowerStateTypeMap.value(device->deviceClassId()), power);
        }
    }

    if (topic == "shellies/" + shellyId + "/light/0") {
        bool on = payload == "on";
        if (m_powerStateTypeMap.contains(device->deviceClassId())) {
            device->setStateValue(m_powerStateTypeMap.value(device->deviceClassId()), on);
        }
    }

    if (topic == "shellies/" + shellyId + "/light/0/status") {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcShelly()) << "Error parsing JSON from Shelly:" << error.error << error.errorString() << payload;
            return;
        }
        QVariantMap statusMap = jsonDoc.toVariant().toMap();
        if (m_brightnessStateTypeMap.contains(device->deviceClassId())) {
            int brightness = statusMap.value("brightness").toInt();
            device->setStateValue(m_brightnessStateTypeMap.value(device->deviceClassId()), brightness);
        }
    }

    if (topic == "shellies/" + shellyId + "/light/0/power") {
        if (m_currentPowerStateTypeMap.contains(device->deviceClassId())) {
            double power = payload.toDouble();
            device->setStateValue(m_currentPowerStateTypeMap.value(device->deviceClassId()), power);
        }
    }

    if (topic == "shellies/" + shellyId + "/roller/0") {
        // Roller shutters are always child devices...
        foreach (Device *child, myDevices().filterByParentDeviceId(device->id()).filterByInterface("extendedshutter")) {
            child->setStateValue(shellyRollerMovingStateTypeId, payload != "stop");
        }
    }
    if (topic == "shellies/" + shellyId + "/roller/0/pos") {
        // Roller shutters are always child devices...
        int pos = payload.toInt();
        foreach (Device *child, myDevices().filterByParentDeviceId(device->id()).filterByInterface("extendedshutter")) {
            child->setStateValue(shellyRollerPercentageStateTypeId, 100 - pos);
        }
    }
}

void DevicePluginShelly::setupShellyGateway(DeviceSetupInfo *info)
{
    Device *device = info->device();
    QString shellyId = info->device()->paramValue(m_idParamTypeMap.value(info->device()->deviceClassId())).toString();
    ZeroConfServiceEntry zeroConfEntry;
    foreach (const ZeroConfServiceEntry &entry, m_zeroconfBrowser->serviceEntries()) {
        if (entry.name() == shellyId) {
            zeroConfEntry = entry;
        }
    }
    QHostAddress address;
    pluginStorage()->beginGroup(info->device()->id().toString());
    if (zeroConfEntry.isValid()) {
        address = zeroConfEntry.hostAddress().toString();
        pluginStorage()->setValue("cachedAddress", address.toString());
    } else {
        qCWarning(dcShelly()) << "Could not find Shelly device on zeroconf. Trying cached address.";
        address = pluginStorage()->value("cachedAddress").toString();
    }
    pluginStorage()->endGroup();

    if (address.isNull()) {
        qCWarning(dcShelly()) << "Unable to determine Shelly's network address. Failed to set up device.";
        info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Unable to find the device in the network."));
        return;
    }

    // Validate params
    bool rollerMode = false;
    if (info->device()->deviceClassId() == shelly25DeviceClassId) {
        QString connectedDevice1 = info->device()->paramValue(shelly25DeviceConnectedDevice1ParamTypeId).toString();
        QString connectedDevice2 = info->device()->paramValue(shelly25DeviceConnectedDevice2ParamTypeId).toString();
        if (connectedDevice1.startsWith("Roller Shutter") && !connectedDevice2.startsWith("Roller Shutter")) {
            qCWarning(dcShelly()) << "Cannot mix roller and relay mode. This won't work..";
            info->finish(Device::DeviceErrorInvalidParameter, QT_TR_NOOP("Roller shutter mode can't be mixed with relay mode. Please configure both connected devices to control a shutter or relays."));
            return;
        }
        if (connectedDevice1 == "Roller Shutter Up" && connectedDevice2 != "Roller Shutter Down") {
            qCWarning(dcShelly()) << "Connected device 1 is shutter up but connected device 2 is not shutter down. This won't work..";
            info->finish(Device::DeviceErrorInvalidParameter, QT_TR_NOOP("For using a roller shutter, one channel must be set to up, the other to down."));
            return;
        }
        if (connectedDevice1 == "Roller Shutter Down" && connectedDevice2 != "Roller Shutter Up") {
            qCWarning(dcShelly()) << "Connected device 1 is shutter down but connected device 2 is not shutter up. This won't work..";
            info->finish(Device::DeviceErrorInvalidParameter, QT_TR_NOOP("For using a roller shutter, one channel must be set to up, the other to down."));
            return;
        }
        if (connectedDevice1.startsWith("Roller Shutter") && connectedDevice2.startsWith("Roller Shutter")) {
            rollerMode = true;
        }
    }

    MqttChannel *channel = hardwareManager()->mqttProvider()->createChannel(shellyId, QHostAddress(address), {"shellies"});
    if (!channel) {
        qCWarning(dcShelly()) << "Failed to create MQTT channel.";
        return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error creating MQTT channel. Please check MQTT server settings."));
    }
    connect(channel, &MqttChannel::clientConnected, this, &DevicePluginShelly::onClientConnected);
    connect(channel, &MqttChannel::clientDisconnected, this, &DevicePluginShelly::onClientDisconnected);
    connect(channel, &MqttChannel::publishReceived, this, &DevicePluginShelly::onPublishReceived);

    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());
    url.setPort(80);
    url.setPath("/settings");
    url.setUserName(info->device()->paramValue(m_usernameParamTypeMap.value(info->device()->deviceClassId())).toString());
    url.setPassword(info->device()->paramValue(m_passwordParamTypeMap.value(info->device()->deviceClassId())).toString());

    QUrlQuery query;
    query.addQueryItem("mqtt_server", channel->serverAddress().toString() + ":" + QString::number(channel->serverPort()));
    query.addQueryItem("mqtt_user", channel->username());
    query.addQueryItem("mqtt_pass", channel->password());
    query.addQueryItem("mqtt_enable", "true");

    // Make sure the shelly 2.5 is in the mode we expect it to be (roller or relay)
    if (info->device()->deviceClassId() == shelly25DeviceClassId) {
        query.addQueryItem("mode", rollerMode ? "roller" : "relay");
    }

    url.setQuery(query);
    QNetworkRequest request(url);

    qCDebug(dcShelly()) << "Connecting to" << url.toString();
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(info, &DeviceSetupInfo::aborted, channel, [this, channel](){
        hardwareManager()->mqttProvider()->releaseChannel(channel);
    });
    connect(reply, &QNetworkReply::finished, info, [this, info, reply, channel, address](){
        if (reply->error() != QNetworkReply::NoError) {
            hardwareManager()->mqttProvider()->releaseChannel(channel);
            qCWarning(dcShelly()) << "Error fetching device settings" << reply->error() << reply->errorString();
            if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
                info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("Username and password not set correctly."));
            } else {
                info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error connecting to Shelly device."));
            }
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcShelly()) << "Error parsing settings reply" << error.errorString() << "\n" << data;
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Unexpected data received from Shelly device."));
            hardwareManager()->mqttProvider()->releaseChannel(channel);
            return;
        }
        qCDebug(dcShelly()) << "Settings data" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));

        m_mqttChannels.insert(info->device(), channel);

        DeviceDescriptors autoChilds;

        // Autogenerate some childs if this device has no childs yet
        if (myDevices().filterByParentDeviceId(info->device()->id()).isEmpty()) {
            // Always create the switch device if we don't have one yet for shellies with input (1, 1pm etc)
            if (info->device()->deviceClassId() == shelly1DeviceClassId
                    || info->device()->deviceClassId() == shelly1pmDeviceClassId) {
                DeviceDescriptor switchChild(shellySwitchDeviceClassId, info->device()->name() + " switch", QString(), info->device()->id());
                autoChilds.append(switchChild);
            }

            // Create 2 switches for shelly 2.5
            if (info->device()->deviceClassId() == shelly25DeviceClassId) {
                DeviceDescriptor switchChild(shellySwitchDeviceClassId, info->device()->name() + " switch 1", QString(), info->device()->id());
                switchChild.setParams(ParamList() << Param(shellySwitchDeviceChannelParamTypeId, 1));
                autoChilds.append(switchChild);
                DeviceDescriptor switch2Child(shellySwitchDeviceClassId, info->device()->name() + " switch 2", QString(), info->device()->id());
                switch2Child.setParams(ParamList() << Param(shellySwitchDeviceChannelParamTypeId, 2));
                autoChilds.append(switch2Child);
            }

            // Add connected devices as configured in params
            // No PM devices for shelly 1
            if (info->device()->deviceClassId() == shelly1DeviceClassId) {
                if (info->device()->paramValue(m_connectedDeviceParamTypeMap.value(info->device()->deviceClassId())).toString() == "Generic") {
                    DeviceDescriptor genericChild(shellyGenericDeviceClassId, info->device()->name() + " connected device", QString(), info->device()->id());
                    genericChild.setParams(ParamList() << Param(shellyGenericDeviceChannelParamTypeId, 1));
                    autoChilds.append(genericChild);
                }
                if (info->device()->paramValue(m_connectedDeviceParamTypeMap.value(info->device()->deviceClassId())).toString() == "Light") {
                    DeviceDescriptor lightChild(shellyLightDeviceClassId, info->device()->name() + " connected light", QString(), info->device()->id());
                    lightChild.setParams(ParamList() << Param(shellyLightDeviceChannelParamTypeId, 1));
                    autoChilds.append(lightChild);
                }
                if (info->device()->paramValue(m_connectedDeviceParamTypeMap.value(info->device()->deviceClassId())).toString() == "Socket") {
                    DeviceDescriptor socketChild(shellySocketDeviceClassId, info->device()->name() + " connected socket", QString(), info->device()->id());
                    socketChild.setParams(ParamList() << Param(shellySocketDeviceChannelParamTypeId, 1));
                    autoChilds.append(socketChild);
                }
            // PM devices for shelly 1 pm and 2.5
            } else if (info->device()->deviceClassId() == shelly1pmDeviceClassId
                       || info->device()->deviceClassId() == shelly25DeviceClassId) {
                if (info->device()->paramValue(m_connectedDeviceParamTypeMap.value(info->device()->deviceClassId())).toString() == "Generic") {
                    DeviceDescriptor genericChild(shellyGenericPMDeviceClassId, info->device()->name() + " connected device", QString(), info->device()->id());
                    genericChild.setParams(ParamList() << Param(shellyGenericPMDeviceChannelParamTypeId, 1));
                    autoChilds.append(genericChild);
                }
                if (info->device()->paramValue(m_connectedDeviceParamTypeMap.value(info->device()->deviceClassId())).toString() == "Light") {
                    DeviceDescriptor lightChild(shellyLightPMDeviceClassId, info->device()->name() + " connected light", QString(), info->device()->id());
                    lightChild.setParams(ParamList() << Param(shellyLightPMDeviceChannelParamTypeId, 1));
                    autoChilds.append(lightChild);
                }
                if (info->device()->paramValue(m_connectedDeviceParamTypeMap.value(info->device()->deviceClassId())).toString() == "Socket") {
                    DeviceDescriptor socketChild(shellySocketPMDeviceClassId, info->device()->name() + " connected socket", QString(), info->device()->id());
                    socketChild.setParams(ParamList() << Param(shellySocketPMDeviceChannelParamTypeId, 1));
                    autoChilds.append(socketChild);
                }

                // Add more for 2.5
                if (info->device()->deviceClassId() == shelly25DeviceClassId) {
                    if (info->device()->paramValue(m_connectedDevice2ParamTypeMap.value(info->device()->deviceClassId())).toString() == "Generic") {
                        DeviceDescriptor genericChild(shellyGenericPMDeviceClassId, info->device()->name() + " connected device 2", QString(), info->device()->id());
                        genericChild.setParams(ParamList() << Param(shellyGenericPMDeviceChannelParamTypeId, 2));
                        autoChilds.append(genericChild);
                    }
                    if (info->device()->paramValue(m_connectedDevice2ParamTypeMap.value(info->device()->deviceClassId())).toString() == "Light") {
                        DeviceDescriptor lightChild(shellyLightPMDeviceClassId, info->device()->name() + " connected light 2", QString(), info->device()->id());
                        lightChild.setParams(ParamList() << Param(shellyLightPMDeviceChannelParamTypeId, 2));
                        autoChilds.append(lightChild);
                    }
                    if (info->device()->paramValue(m_connectedDevice2ParamTypeMap.value(info->device()->deviceClassId())).toString() == "Socket") {
                        DeviceDescriptor socketChild(shellySocketPMDeviceClassId, info->device()->name() + " connected socket 2", QString(), info->device()->id());
                        socketChild.setParams(ParamList() << Param(shellySocketPMDeviceChannelParamTypeId, 2));
                        autoChilds.append(socketChild);
                    }

                    if (info->device()->paramValue(m_connectedDeviceParamTypeMap.value(info->device()->deviceClassId())).toString() == "Roller Shutter Up"
                            && info->device()->paramValue(m_connectedDevice2ParamTypeMap.value(info->device()->deviceClassId())).toString() == "Roller Shutter Down") {
                        DeviceDescriptor rollerShutterChild(shellyRollerDeviceClassId, info->device()->name() + " connected shutter", QString(), info->device()->id());
                        rollerShutterChild.setParams(ParamList() << Param(shellyRollerDeviceChannelParamTypeId, 1));
                        autoChilds.append(rollerShutterChild);
                    }
                }                
            }
        }

        info->finish(Device::DeviceErrorNoError);

        emit autoDevicesAppeared(autoChilds);

        // Make sure authentication is enalbed if the user wants it
        QString username = info->device()->paramValue(m_usernameParamTypeMap.value(info->device()->deviceClassId())).toString();
        QString password = info->device()->paramValue(m_passwordParamTypeMap.value(info->device()->deviceClassId())).toString();
        if (!username.isEmpty()) {
            QUrl url;
            url.setScheme("http");
            url.setHost(address.toString());
            url.setPort(80);
            url.setPath("/settings/login");
            url.setUserName(username);
            url.setPassword(password);

            QUrlQuery query;
            query.addQueryItem("username", username);
            query.addQueryItem("password", password);
            query.addQueryItem("enabled", "true");

            url.setQuery(query);

            QNetworkRequest request(url);
            qCDebug(dcShelly()) << "Enabling auth" << username << password;
            QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        }
    });

    // Handle device settings of gateway devices
    if (info->device()->deviceClassId() == shellyPlugDeviceClassId) {
        connect(info->device(), &Device::settingChanged, this, [this, device, shellyId](const ParamTypeId &paramTypeId, const QVariant &value) {
            pluginStorage()->beginGroup(device->id().toString());
            QString address = pluginStorage()->value("cachedAddress").toString();
            pluginStorage()->endGroup();

            QUrl url;
            url.setScheme("http");
            url.setHost(address);
            url.setPort(80);
            url.setPath("/settings/relay/0");
            url.setUserName(device->paramValue(m_usernameParamTypeMap.value(device->deviceClassId())).toString());
            url.setPassword(device->paramValue(m_passwordParamTypeMap.value(device->deviceClassId())).toString());

            QUrlQuery query;
            if (paramTypeId == shellyPlugSettingsDefaultStateParamTypeId) {
                query.addQueryItem("default_state", value.toString());
            }

            url.setQuery(query);

            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        });
    }

}

void DevicePluginShelly::setupShellyChild(DeviceSetupInfo *info)
{
    Device *device = info->device();

    // Connect to settings changes to store them to the device
    connect(info->device(), &Device::settingChanged, this, [this, device](const ParamTypeId &paramTypeId, const QVariant &value){
        Device *parentDevice = myDevices().findById(device->parentId());
        pluginStorage()->beginGroup(parentDevice->id().toString());
        QString address = pluginStorage()->value("cachedAddress").toString();
        pluginStorage()->endGroup();

        QUrl url;
        url.setScheme("http");
        url.setHost(address);
        url.setPort(80);
        url.setPath("/settings/relay/0");
        url.setUserName(parentDevice->paramValue(m_usernameParamTypeMap.value(parentDevice->deviceClassId())).toString());
        url.setPassword(parentDevice->paramValue(m_passwordParamTypeMap.value(parentDevice->deviceClassId())).toString());

        QUrlQuery query;
        if (paramTypeId == shellySwitchSettingsButtonTypeParamTypeId) {
            query.addQueryItem("btn_type", value.toString());
        }
        if (paramTypeId == shellySwitchSettingsInvertButtonParamTypeId) {
            query.addQueryItem("btn_reverse", value.toBool() ? "1" : "0");
        }
        if (paramTypeId == shellyGenericSettingsDefaultStateParamTypeId || paramTypeId == shellyLightSettingsDefaultStateParamTypeId) {
            query.addQueryItem("default_state", value.toString());
        }

        url.setQuery(query);

        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    });

    info->finish(Device::DeviceErrorNoError);
}

QString DevicePluginShelly::getIP(Device *device) const
{
    Device *d = device;
    if (!device->parentId().isNull()) {
        d = myDevices().findById(device->parentId());
    }
    pluginStorage()->beginGroup(d->id().toString());
    QString ip = pluginStorage()->value("cachedAddress").toString();
    pluginStorage()->endGroup();
    return ip;
}
