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

void DevicePluginYeelight::init()
{
    m_connectedStateTypeIds.insert(colorBulbDeviceClassId, colorBulbConnectedStateTypeId);
    m_connectedStateTypeIds.insert(dimmableBulbDeviceClassId, dimmableBulbConnectedStateTypeId);

    m_powerStateTypeIds.insert(colorBulbDeviceClassId, colorBulbPowerStateTypeId);
    m_powerStateTypeIds.insert(dimmableBulbDeviceClassId, dimmableBulbPowerStateTypeId);

    m_brightnessStateTypeIds.insert(colorBulbDeviceClassId, colorBulbBrightnessStateTypeId);
    m_brightnessStateTypeIds.insert(dimmableBulbDeviceClassId, dimmableBulbBrightnessStateTypeId);

    m_ssdp = new Ssdp(this);
    m_ssdp->enable();
    m_ssdp->discover();
}

void DevicePluginYeelight::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (info->deviceClassId() == colorBulbDeviceClassId) {


        qCDebug(dcYeelight()) << "Starting UPnP discovery...";
        /*
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
        });*/
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
        connect(yeelight, &Yeelight::colorModeNotificationReceived, this, &DevicePluginYeelight::onColorModeNotificationReceived);
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
                    QList<Yeelight::YeelightProperty> properties;
                    if (device->deviceClassId() == colorBulbDeviceClassId) {
                        properties << Yeelight::YeelightProperty::Ct;
                        properties << Yeelight::YeelightProperty::Rgb;
                        properties << Yeelight::YeelightProperty::Power;
                        properties << Yeelight::YeelightProperty::Bright;
                        properties << Yeelight::YeelightProperty::ColorMode;
                    } else if (device->deviceClassId() == colorBulbDeviceClassId) {
                        properties << Yeelight::YeelightProperty::Power;
                        properties << Yeelight::YeelightProperty::Bright;
                    }
                    yeelight->getParam(properties);
                }
            }
        });
    }
    m_pluginTimer->timeout();
}

void DevicePluginYeelight::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();
    Yeelight *yeelight = m_yeelightConnections.value(device->id());
    if (!yeelight)
        return info->finish(Device::DeviceErrorHardwareFailure);

    if (device->deviceClassId() == colorBulbDeviceClassId) {

        if (action.actionTypeId() == colorBulbPowerActionTypeId) {
            bool power = action.param(colorBulbPowerActionPowerParamTypeId).value().toBool();
            int requestId = yeelight->setPower(power);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbBrightnessActionTypeId) {

            if (!device->stateValue(colorBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);

            if (m_pendingBrightnessAction.contains(device->id())) {
                //Scrap old info and insert new one
                m_pendingBrightnessAction.insert(device->id(), info);
            } else {
                m_pendingBrightnessAction.insert(device->id(), info);
                    QTimer::singleShot(500, this, [yeelight, info, this]{
                    int brightness = info->action().param(colorBulbBrightnessActionBrightnessParamTypeId).value().toInt();
                    m_pendingBrightnessAction.remove(info->device()->id());
                    int requestId = yeelight->setBrightness(brightness);
                    connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                    m_asyncActions.insert(requestId, info);
                });
            }
        } else if (action.actionTypeId() == colorBulbColorActionColorParamTypeId) {
            QRgb color = QColor(action.param(colorBulbColorActionColorParamTypeId).value().toString()).rgba();
            if (!device->stateValue(colorBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);
            int requestId = yeelight->setRgb(color);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbColorTemperatureActionTypeId) {
            int colorTemperature = 6500 - (action.param(colorBulbColorTemperatureActionColorTemperatureParamTypeId).value().toUInt() * -11.12);
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
        } else if (action.actionTypeId() == colorBulbDefaultActionTypeId) {
            int requestId = yeelight->setDefault();
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else {
            qCWarning(dcYeelight()) << "Unhandled action";
        }
    } else  if (device->deviceClassId() == dimmableBulbDeviceClassId) {
        if (action.actionTypeId() == dimmableBulbPowerActionTypeId) {
            bool power = action.param(dimmableBulbPowerActionPowerParamTypeId).value().toBool();
            int requestId = yeelight->setPower(power);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == dimmableBulbBrightnessActionTypeId) {
            int brightness = action.param(dimmableBulbBrightnessActionBrightnessParamTypeId).value().toInt();
            if (!device->stateValue(dimmableBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);
            int requestId = yeelight->setBrightness(brightness);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == dimmableBulbAlertActionTypeId) {
            QString alertType = action.param(dimmableBulbAlertActionAlertParamTypeId).value().toString();
            if (!device->stateValue(dimmableBulbPowerStateTypeId).toBool())
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
        } else if (action.actionTypeId() == dimmableBulbDefaultActionTypeId) {
            int requestId = yeelight->setDefault();
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else {
            qCWarning(dcYeelight()) << "Unhandled action";
        }
    } else {
        qCWarning(dcYeelight()) << "Unhandled device class";
    }
}

void DevicePluginYeelight::deviceRemoved(Device *device)
{
    if ((device->deviceClassId() == colorBulbDeviceClassId) ||
            (device->deviceClassId() == dimmableBulbDeviceClassId)){
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

    device->setStateValue(m_connectedStateTypeIds.value(device->deviceClassId()), connected);
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

void DevicePluginYeelight::onPowerNotificationReceived(bool status)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Device * device = myDevices().findById(m_yeelightConnections.key(yeelight));
    if(!device)
        return;

    device->setStateValue(m_powerStateTypeIds.value(device->deviceClassId()), status);
}

void DevicePluginYeelight::onBrightnessNotificationReceived(int percentage)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Device * device = myDevices().findById(m_yeelightConnections.key(yeelight));
    if(!device)
        return;

    device->setStateValue(m_brightnessStateTypeIds.value(device->deviceClassId()), percentage);
}

void DevicePluginYeelight::onColorTemperatureNotificationReceived(int kelvin)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Device * device = myDevices().findById(m_yeelightConnections.key(yeelight));
    if(!device)
        return;

    device->setStateValue(colorBulbColorTemperatureStateTypeId, kelvin/11.12); //TODO needs proper conversion
}

void DevicePluginYeelight::onRgbNotificationReceived(QRgb rgbColor)
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

void DevicePluginYeelight::onColorModeNotificationReceived(Yeelight::YeelightColorMode colorMode)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Device * device = myDevices().findById(m_yeelightConnections.key(yeelight));
    if(!device)
        return;

    switch (colorMode) {
    case Yeelight::RGB:
        device->setStateValue(colorBulbColorModeStateTypeId, "RGB");
        break;
    case Yeelight::HSV:
        device->setStateValue(colorBulbColorModeStateTypeId, "HSV");
        break;
    case Yeelight::ColorTemperature:
        device->setStateValue(colorBulbColorModeStateTypeId, "Color temperature");
        break;
    }
}
