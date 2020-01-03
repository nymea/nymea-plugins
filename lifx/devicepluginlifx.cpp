/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by copyright law, and
* remains the property of nymea GmbH. All rights, including reproduction, publication,
* editing and translation, are reserved. The use of this project is subject to the terms of a
* license agreement to be concluded with nymea GmbH in accordance with the terms
* of use of nymea GmbH, available under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* This project may also contain libraries licensed under the open source software license GNU GPL v.3.
* Alternatively, this project may be redistributed and/or modified under the terms of the GNU
* Lesser General Public License as published by the Free Software Foundation; version 3.
* this project is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along with this project.
* If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under contact@nymea.io
* or see our FAQ/Licensing Information on https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginlifx.h"

#include "devices/device.h"
#include "types/param.h"
#include "plugininfo.h"

#include <QDebug>
#include <QColor>
#include <QRgb>

DevicePluginLifx::DevicePluginLifx()
{

}

void DevicePluginLifx::init()
{
    m_connectedStateTypeIds.insert(colorBulbDeviceClassId, colorBulbConnectedStateTypeId);
    m_connectedStateTypeIds.insert(dimmableBulbDeviceClassId, dimmableBulbConnectedStateTypeId);

    m_powerStateTypeIds.insert(colorBulbDeviceClassId, colorBulbPowerStateTypeId);
    m_powerStateTypeIds.insert(dimmableBulbDeviceClassId, dimmableBulbPowerStateTypeId);

    m_brightnessStateTypeIds.insert(colorBulbDeviceClassId, colorBulbBrightnessStateTypeId);
    m_brightnessStateTypeIds.insert(dimmableBulbDeviceClassId, dimmableBulbBrightnessStateTypeId);
}

void DevicePluginLifx::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (!m_lifxConnection) {
        m_lifxConnection = new Lifx(this);
        m_lifxConnection->enable();
    }

    if (info->deviceClassId() == colorBulbDeviceClassId) {
    }
}

void DevicePluginLifx::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (!m_lifxConnection) {
        m_lifxConnection = new Lifx(this);
        m_lifxConnection->enable();
    }

    if (device->deviceClassId() == colorBulbDeviceClassId) {

        info->finish(Device::DeviceErrorNoError);
    }
}

void DevicePluginLifx::postSetupDevice(Device *device)
{
    connect(device, &Device::nameChanged, this, &DevicePluginLifx::onDeviceNameChanged);

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this]() {

        });
    }
    m_pluginTimer->timeout();
}

void DevicePluginLifx::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (!m_lifxConnection)
        return info->finish(Device::DeviceErrorHardwareFailure);

    if (device->deviceClassId() == colorBulbDeviceClassId) {

        if (action.actionTypeId() == colorBulbPowerActionTypeId) {
            bool power = action.param(colorBulbPowerActionPowerParamTypeId).value().toBool();
            int requestId = m_lifxConnection->setPower(power);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbBrightnessActionTypeId) {

            if (!device->stateValue(colorBulbPowerStateTypeId).toBool())
                m_lifxConnection->setPower(true);

            if (m_pendingBrightnessAction.contains(device->id())) {
                //Scrap old info and insert new one
                m_pendingBrightnessAction.insert(device->id(), info);
            } else {
                m_pendingBrightnessAction.insert(device->id(), info);
                    QTimer::singleShot(500, this, [info, this]{
                    int brightness = info->action().param(colorBulbBrightnessActionBrightnessParamTypeId).value().toInt();
                    m_pendingBrightnessAction.remove(info->device()->id());
                    int requestId = m_lifxConnection->setBrightness(brightness);
                    connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                    m_asyncActions.insert(requestId, info);
                });
            }
        } else if (action.actionTypeId() == colorBulbColorActionColorParamTypeId) {
            QRgb color = QColor(action.param(colorBulbColorActionColorParamTypeId).value().toString()).rgba();
            if (!device->stateValue(colorBulbPowerStateTypeId).toBool())
                m_lifxConnection->setPower(true);
            int requestId = m_lifxConnection->setColor(color);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbColorTemperatureActionTypeId) {
            int colorTemperature = 6500 - (action.param(colorBulbColorTemperatureActionColorTemperatureParamTypeId).value().toUInt() * -11.12);
            if (!device->stateValue(colorBulbPowerStateTypeId).toBool())
                m_lifxConnection->setPower(true);
            int requestId = m_lifxConnection->setColorTemperature(colorTemperature);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else {
            qCWarning(dcLifx()) << "Unhandled action";
        }
    } else  if (device->deviceClassId() == dimmableBulbDeviceClassId) {
        if (action.actionTypeId() == dimmableBulbPowerActionTypeId) {
            bool power = action.param(dimmableBulbPowerActionPowerParamTypeId).value().toBool();
            int requestId = m_lifxConnection->setPower(power);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == dimmableBulbBrightnessActionTypeId) {
            int brightness = action.param(dimmableBulbBrightnessActionBrightnessParamTypeId).value().toInt();
            if (!device->stateValue(dimmableBulbPowerStateTypeId).toBool())
                m_lifxConnection->setPower(true);
            int requestId = m_lifxConnection->setBrightness(brightness);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else {
            qCWarning(dcLifx()) << "Unhandled action";
        }
    } else {
        qCWarning(dcLifx()) << "Unhandled device class";
    }
}

void DevicePluginLifx::deviceRemoved(Device *device)
{
    Q_UNUSED(device)

    if (myDevices().isEmpty()) {
       m_lifxConnection->deleteLater();
       m_lifxConnection = nullptr;
    }
}

void DevicePluginLifx::onDeviceNameChanged()
{
    Device *device = static_cast<Device*>(sender());
    Q_UNUSED(device)
}

void DevicePluginLifx::onConnectionChanged(bool connected)
{
  Q_UNUSED(connected)
//    device->setStateValue(m_connectedStateTypeIds.value(device->deviceClassId()), connected);
}

void DevicePluginLifx::onRequestExecuted(int requestId, bool success)
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

/*void DevicePluginLifx::onPowerNotificationReceived(bool status)
{
    Q_UNUSED(status)
    Lifx *Lifx = static_cast<Lifx *>(sender());
    Device * device = myDevices().findById(m_LifxConnections.key(Lifx));
    if(!device)
        return;

    device->setStateValue(m_powerStateTypeIds.value(device->deviceClassId()), status);

}

void DevicePluginLifx::onBrightnessNotificationReceived(int percentage)
{
    Q_UNUSED(percentage)
    Lifx *lifx = static_cast<Lifx *>(sender());
    Device * device = myDevices().findById(m_lifxConnections.key(lifx));
    if(!device)
        return;

    device->setStateValue(m_brightnessStateTypeIds.value(device->deviceClassId()), percentage);
}*/
