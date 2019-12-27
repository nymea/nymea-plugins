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

#include "devicepluginyeelight.h"

#include "devices/device.h"
#include "types/param.h"
#include "plugininfo.h"
#include "ssdp.h"

#include <QDebug>
#include <QColor>
#include <QRgb>

DevicePluginYeelight::DevicePluginYeelight()
{

}

void DevicePluginYeelight::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (info->deviceClassId() == colorBulbDeviceClassId) {

        DiscoveryJob *discovery = new DiscoveryJob();
        m_discoveries.insert(info, discovery);

        qCDebug(dcYeelight()) << "Starting UPnP discovery...";
        Ssdp *ssdp = new Ssdp(this);
        ssdp->enable();
        ssdp->discover();
        connect(ssdp, &Ssdp::discovered, this,[info, this](const QString &address, int port, int id, const QString &model) {

            DeviceDescriptor descriptor(colorBulbDeviceClassId, "Yeelight"+ model + "Bulb", address);
            ParamList params;
            foreach (Device *existingDevice, myDevices()) {
                if (existingDevice->paramValue(colorBulbDeviceIdParamTypeId).toString() == id) {
                    descriptor.setDeviceId(existingDevice->id());
                    break;
                }
            }
            params.append(Param(colorBulbDeviceHostParamTypeId, address));
            params.append(Param(colorBulbDeviceIdParamTypeId, id));
            params.append(Param(colorBulbDevicePortParamTypeId, port));
            descriptor.setParams(params);
            qCDebug(dcYeelight()) << "UPnP: Found Yeelight device:" << id;
            info->finish(Device::DeviceErrorNoError);
        });
    }
}

void DevicePluginYeelight::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == colorBulbDeviceClassId) {
        QHostAddress address = QHostAddress(device->paramValue(colorBulbDeviceHostParamTypeId).toString());
        quint16 port = device->paramValue(colorBulbDevicePortParamTypeId).toUInt();
        Yeelight *yeelight = new Yeelight(hardwareManager()->networkManager(), address, port, this);
        m_yeelightConnections.insert(device->id(), yeelight);
        connect(yeelight, &Yeelight::connectionChanged, this, &DevicePluginYeelight::onConnectionChanged);
        connect(yeelight, &Yeelight::requestExecuted, this, &DevicePluginYeelight::onRequestExecuted);
        connect(yeelight, &Yeelight::propertyListReceived, this, &DevicePluginYeelight::onPropertyListReceived);
        connect(yeelight, &Yeelight::powerNotificationReceived, this, &DevicePluginYeelight::onPowerNotificationReceived);
        connect(yeelight, &Yeelight::brightnessNotificationReceived, this, &DevicePluginYeelight::onBrightnessNotificationReceived);
        connect(yeelight, &Yeelight::colorTemperatureNotificationReceived, this, &DevicePluginYeelight::onColorTemperatureNotificationReceived);
        connect(yeelight, &Yeelight::rgbNotificationReceived, this, &DevicePluginYeelight::onRgbNotificationReceived);
        connect(yeelight, &Yeelight::nameNotificationReceived, this, &DevicePluginYeelight::onNameNotificationReceived);
        info->finish(Device::DeviceErrorNoError);
    }
}

void DevicePluginYeelight::postSetupDevice(Device *device)
{
    connect(device, &Device::nameChanged, this, &DevicePluginYeelight::onDeviceNameChanged);

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this]() {
            foreach (DeviceId id, m_yeelightConnections.keys()) {
                Device *device = myDevices().findById(id);
                Yeelight *yeelight = m_yeelightConnections.value(id);
                if (yeelight->isConnected()) {
                    QList<Yeelight::Property> properties;
                    if (device->deviceClassId() == colorBulbDeviceClassId) {
                    properties << Yeelight::Property::Name;
                    properties << Yeelight::Property::Ct;
                    properties << Yeelight::Property::Rgb;
                    properties << Yeelight::Property::Power;
                    properties << Yeelight::Property::Bright;
                    properties << Yeelight::Property::ColorMode;
                    } //TODO add white color bulb with other property requests
                    yeelight->getParam(properties);
                } else {
                    yeelight->connectDevice();
                }
            }
        });
    }
}

void DevicePluginYeelight::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == colorBulbDeviceClassId) {
        Yeelight *yeelight = m_yeelightConnections.value(device->id());

        if (action.actionTypeId() == colorBulbPowerActionTypeId) {
            bool power = action.param(colorBulbPowerActionPowerParamTypeId).value().toBool();
            int requestId = yeelight->setPower(power);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbBrightnessActionTypeId) {
            int brightness = action.param(colorBulbBrightnessActionBrightnessParamTypeId).value().toInt();
            if (!device->stateValue(colorBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);
            int requestId = yeelight->setBrightness(brightness);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbColorActionColorParamTypeId) {
            QRgb color = action.param(colorBulbColorActionColorParamTypeId).value().toUInt();
            if (!device->stateValue(colorBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);
            int requestId = yeelight->setRgb(color);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbColorTemperatureActionTypeId) {
            int colorTemperature = action.param(colorBulbColorTemperatureActionColorTemperatureParamTypeId).value().toUInt() * 11.12;
            if (!device->stateValue(colorBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);
            int requestId = yeelight->setColorTemperature(colorTemperature);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbEffectActionTypeId) {
            QString effect = action.param(colorBulbEffectActionEffectParamTypeId).value().toString();
            if (!device->stateValue(colorBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);
            if (effect.contains("non")) {
                int requestId = yeelight->stopColorFlow();
                connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                m_asyncActions.insert(requestId, info);
            } else {
                int requestId = yeelight->startColorFlow();
                connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                m_asyncActions.insert(requestId, info);
            }
        } else if (action.actionTypeId() == colorBulbAlertActionTypeId) {
            QString alertType = action.param(colorBulbAlertActionAlertParamTypeId).value().toString();
            if (!device->stateValue(colorBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);
            if (alertType.contains("15")) { //Flash 15 sec
                int requestId = yeelight->flash15s();
                connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                m_asyncActions.insert(requestId, info);
            } else {
                int requestId = yeelight->flash();
                connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                m_asyncActions.insert(requestId, info);
            }
        } else {
            qCWarning(dcYeelight()) << "Unhandled action";
        }
    } else {
        qCWarning(dcYeelight()) << "Unhandled device class";
    }
}

void DevicePluginYeelight::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == colorBulbDeviceClassId){
        Yeelight *yeelight = m_yeelightConnections.take(device->id());
        yeelight->deleteLater();
    }
}

void DevicePluginYeelight::onDeviceNameChanged()
{
    Device *device = static_cast<Device*>(sender());
    Yeelight *yeelight = m_yeelightConnections.value(device->id());
    yeelight->setName(device->name());
}

void DevicePluginYeelight::onConnectionChanged(bool connected)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Device * device = myDevices().findById(m_yeelightConnections.key(yeelight));
    if(!device)
        return;

    device->setStateValue(colorBulbConnectedStateTypeId, connected);
}

void DevicePluginYeelight::onRequestExecuted(int requestId, bool success)
{
    if (m_asyncActions.contains(requestId)) {
        DeviceActionInfo *info = m_asyncActions.take(requestId);
        if (success) {
            info->finish(Device::DeviceErrorNoError);
        } else {
            info->finish(Device::DeviceErrorHardwareFailure);
        }
    }
}

void DevicePluginYeelight::onPropertyListReceived(QVariantList value)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Device * device = myDevices().findById(m_yeelightConnections.key(yeelight));
    if(!device)
        return;

    /*
     * properties << Yeelight::Property::Name;
     * properties << Yeelight::Property::Ct;
     * properties << Yeelight::Property::Rgb;
     * properties << Yeelight::Property::Power;
     * properties << Yeelight::Property::Bright;
     * properties << Yeelight::Property::ColorMode;
    */

    if(value.length() < 6)
        return;

    device->setName(value[0].toString());
    device->setStateValue(colorBulbColorTemperatureStateTypeId, value[1].toInt());
    device->setStateValue(colorBulbColorStateTypeId, value[2].toString());
    device->setStateValue(colorBulbPowerStateTypeId, (value[3].toString() == "on"));
    device->setStateValue(colorBulbBrightnessStateTypeId, value[4].toInt());
    device->setStateValue(colorBulbColorModeStateTypeId, value[5].toString());
}

void DevicePluginYeelight::onPowerNotificationReceived(bool status)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Device * device = myDevices().findById(m_yeelightConnections.key(yeelight));
    if(!device)
        return;

    device->setStateValue(colorBulbPowerStateTypeId, status);
}

void DevicePluginYeelight::onBrightnessNotificationReceived(int percentage)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Device * device = myDevices().findById(m_yeelightConnections.key(yeelight));
    if(!device)
        return;

    device->setStateValue(colorBulbBrightnessStateTypeId, percentage);
}

void DevicePluginYeelight::onColorTemperatureNotificationReceived(int kelvin)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Device * device = myDevices().findById(m_yeelightConnections.key(yeelight));
    if(!device)
        return;

    device->setStateValue(colorBulbColorTemperatureStateTypeId, kelvin/11.12); //TODO needs proper conversion
}

void DevicePluginYeelight::onRgbNotificationReceived(int rgbColor)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Device * device = myDevices().findById(m_yeelightConnections.key(yeelight));
    if(!device)
        return;

    device->setStateValue(colorBulbColorStateTypeId, rgbColor);
}

void DevicePluginYeelight::onNameNotificationReceived(const QString &name)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Device * device = myDevices().findById(m_yeelightConnections.key(yeelight));
    if(!device)
        return;
    device->setName(name);
}
