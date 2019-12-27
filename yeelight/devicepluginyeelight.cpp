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
        info->finish(Device::DeviceErrorNoError);
    }
}

void DevicePluginYeelight::postSetupDevice(Device *device)
{
    connect(device, &Device::nameChanged, this, &DevicePluginYeelight::onDeviceNameChanged);

    if (device->deviceClassId() == colorBulbDeviceClassId) {
    }

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this]() {
            foreach (Yeelight *yeelight, m_yeelightConnections.values()) {
                if (yeelight->isConnected()) {
                    QList<Yeelight::Property> properties;
                    properties << Yeelight::Property::Name;
                    properties << Yeelight::Property::Ct;
                    properties << Yeelight::Property::Rgb;
                    properties << Yeelight::Property::Power;
                    properties << Yeelight::Property::Bright;
                    properties << Yeelight::Property::ColorMode;
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
            yeelight->setPower(power);
        } else if (action.actionTypeId() == colorBulbBrightnessActionTypeId) {
            int brightness = action.param(colorBulbBrightnessActionBrightnessParamTypeId).value().toInt();
            yeelight->setBrightness(brightness);
        } else if (action.actionTypeId() == colorBulbColorActionColorParamTypeId) {
            QRgb color = action.param(colorBulbColorActionColorParamTypeId).value().toUInt();
            yeelight->setRgb(color);
        } else if (action.actionTypeId() == colorBulbColorTemperatureActionTypeId) {
            int colorTemperature = action.param(colorBulbColorTemperatureActionColorTemperatureParamTypeId).value().toUInt() * 11.12;
            yeelight->setColorTemperature(colorTemperature);
        }
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
