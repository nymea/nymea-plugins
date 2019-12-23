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

#include "devicepluginnanoleaf.h"
#include "plugininfo.h"

#include "network/zeroconf/zeroconfservicebrowser.h"
#include "platform/platformzeroconfcontroller.h"

#include <QHash>
#include <QDebug>

DevicePluginNanoleaf::DevicePluginNanoleaf()
{

}


void DevicePluginNanoleaf::init()
{
    m_zeroconfBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_nanoleafapi._tcp");
}

void DevicePluginNanoleaf::discoverDevices(DeviceDiscoveryInfo *info)
{
    QStringList serialNumbers;
    foreach (const ZeroConfServiceEntry &entry, m_zeroconfBrowser->serviceEntries()) {
        if (info->deviceClassId() == lightPanelsDeviceClassId) {

        }
        //TODO skip duplicated devices

        DeviceDescriptor descriptor(lightPanelsDeviceClassId, entry.name(), entry.hostAddress().toString());
        ParamList params;

        QString serialNo;
        QString model;
        QString firmwareVersion;

        foreach (QString value, entry.txt()) {
            if (value.contains("id=")) {
                serialNo = value.split("=").last();
            } else if (value.contains("md=")) {
                model = value.split("=").last();
            } else if (value.contains("srcvers=")) {
                firmwareVersion = value.split("=").last();
            }
        }
        if (serialNumbers.contains(serialNo)) {
            continue; //To avoid duplicated devices
        }

        Device *existingDevice = myDevices().findByParams(ParamList() << Param(lightPanelsDeviceSerialNoParamTypeId, serialNo));
        if (existingDevice) {
            descriptor.setDeviceId(existingDevice->id());
        }

        serialNumbers.append(serialNo);
        qCDebug(dcNanoleaf()) << "Have device" << entry.name() << serialNo << model << firmwareVersion;
        params << Param(lightPanelsDeviceAddressParamTypeId, entry.hostAddress().toString());
        params << Param(lightPanelsDevicePortParamTypeId, entry.port());
        params << Param(lightPanelsDeviceModelParamTypeId, model);
        params << Param(lightPanelsDeviceSerialNoParamTypeId, serialNo);
        params << Param(lightPanelsDeviceFirmwareVersionParamTypeId, firmwareVersion);
        descriptor.setParams(params);

        info->addDeviceDescriptor(descriptor);

    }
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginNanoleaf::startPairing(DevicePairingInfo *info)
{
    info->finish(Device::DeviceErrorNoError, tr("Please press the button"));
}

void DevicePluginNanoleaf::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)
    Q_UNUSED(secret)
    Nanoleaf *nanoleaf = new Nanoleaf(hardwareManager()->networkManager(), QHostAddress(info->params().paramValue(lightPanelsDeviceAddressParamTypeId).toString()), info->params().paramValue(lightPanelsDevicePortParamTypeId).toInt(), this);
    nanoleaf->addUser();
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginNanoleaf::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    if(device->deviceClassId() == lightPanelsDeviceClassId) {
        return info->finish(Device::DeviceErrorNoError);
    }
}

void DevicePluginNanoleaf::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == lightPanelsDeviceClassId) {
        //TODO get all the information and set the device states
    }

    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this]() {

        });
    }
}

void DevicePluginNanoleaf::deviceRemoved(Device *device)
{
    if(device->deviceClassId() == lightPanelsDeviceClassId) {

    }

    if (myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void DevicePluginNanoleaf::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == lightPanelsDeviceClassId) {
        Nanoleaf *nanoleaf = m_nanoleafConnections.value(device->id());
        if (action.actionTypeId() == lightPanelsPowerActionTypeId) {
            bool power = action.param(lightPanelsPowerActionPowerParamTypeId).value().toBool();
            QUuid requestId = nanoleaf->setPower(power);
            connect(info, &DeviceActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);

        } else if (action.actionTypeId() == lightPanelsBrightnessActionTypeId) {
            int brightness = action.param(lightPanelsBrightnessActionBrightnessParamTypeId).value().toInt();
            QUuid requestId = nanoleaf->setBrightness(brightness);
            connect(info, &DeviceActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);

        } else if (action.actionTypeId() == lightPanelsColorActionTypeId) {
            QColor color(action.param(lightPanelsColorActionColorParamTypeId).value().toString());
            QUuid requestId = nanoleaf->setHue(color);
            connect(info, &DeviceActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);

        } else if (action.actionTypeId() == lightPanelsColorTemperatureActionTypeId) {
            int colorTemperature = action.param(lightPanelsColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
            QUuid requestId = nanoleaf->setColorTemperature(colorTemperature);
            connect(info, &DeviceActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        }
    }
}

void DevicePluginNanoleaf::getDeviceStates(Nanoleaf *nanoleaf)
{
    nanoleaf->getPower();
    nanoleaf->getHue();
    nanoleaf->getColorMode();
    nanoleaf->getBrightness();
    nanoleaf->getSaturation();
    nanoleaf->getColorTemperature();
}

