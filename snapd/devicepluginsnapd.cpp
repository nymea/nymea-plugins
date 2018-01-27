/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017-2018 Simon Stürz <simon.stuerz@guh.io               *
 *                                                                         *
 *  This file is part of guh.                                              *
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

#include "devicepluginsnapd.h"
#include "plugin/device.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

DevicePluginSnapd::DevicePluginSnapd()
{

}

void DevicePluginSnapd::init()
{
    // Check advanced mode
    m_advancedMode = configValue(SnapdAdvancedModeParamTypeId).toBool();
    connect(this, &DevicePluginSnapd::configValueChanged, this, &DevicePluginSnapd::onPluginConfigurationChanged);

    // Setup timers
    m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
    connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginSnapd::onRefreshTimer);

    // Check all 5 min if there is an update available
    m_updateTimer = hardwareManager()->pluginTimerManager()->registerTimer(300);
    connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginSnapd::onUpdateTimer);
}

void DevicePluginSnapd::startMonitoringAutoDevices()
{
    // Check if we already have a controller and if snapd is available
    if (m_snapdControl && !m_snapdControl->available()) {
        return;
    }

    // Check if we already have a device for the snapd control
    bool deviceAlreadyExists = false;
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == snapdControlDeviceClassId) {
            deviceAlreadyExists = true;
        }
    }

    // Add device if there isn't already one
    if (!deviceAlreadyExists) {
        DeviceDescriptor descriptor(snapdControlDeviceClassId, "Update manager");
        emit autoDevicesAppeared(snapdControlDeviceClassId, QList<DeviceDescriptor>() << descriptor);
    }
}

void DevicePluginSnapd::postSetupDevice(Device *device)
{
    if (m_snapdControl && m_snapdControl->device() == device)
        m_snapdControl->update();

}

void DevicePluginSnapd::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == snapdControlDeviceClassId) {
        qCWarning(dcSnapd()) << "User deleted snapd control.";

        // Clean up old device
        if (m_snapdControl) {
            delete m_snapdControl;
            m_snapdControl = nullptr;
        }

        // Respawn device immediately
        startMonitoringAutoDevices();

    } else if (device->deviceClassId() == snapDeviceClassId) {
        if (m_snapDevices.values().contains(device)) {
            QString snapId = m_snapDevices.key(device);
            m_snapDevices.remove(snapId);
        }
    }
}

DeviceManager::DeviceSetupStatus DevicePluginSnapd::setupDevice(Device *device)
{
    qCDebug(dcSnapd()) << "Setup" << device->name() << device->params();

    if (device->deviceClassId() == snapdControlDeviceClassId) {
        if (m_snapdControl) {
            delete m_snapdControl;
            m_snapdControl = nullptr;
        }

        m_snapdControl = new SnapdControl(device, this);
        connect(m_snapdControl, &SnapdControl::snapListUpdated, this, &DevicePluginSnapd::onSnapListUpdated);

    } else if (device->deviceClassId() == snapDeviceClassId) {
        device->setName(QString("%1").arg(device->paramValue(snapNameParamTypeId).toString()));
        m_snapDevices.insert(device->paramValue(snapIdParamTypeId).toString(), device);
    }

    return DeviceManager::DeviceSetupStatusSuccess;
}

DeviceManager::DeviceError DevicePluginSnapd::executeAction(Device *device, const Action &action) {

    if (device->deviceClassId() == snapdControlDeviceClassId) {

        if (!m_snapdControl) {
            qCDebug(dcSnapd()) << "There is currently no snapd controller.";
            return DeviceManager::DeviceErrorHardwareFailure;
        }

        if (!m_snapdControl->connected()) {
            qCDebug(dcSnapd()) << "Snapd controller not connected to to backend.";
            return DeviceManager::DeviceErrorHardwareFailure;
        }

        if (action.actionTypeId() == snapdControlStartUpdateActionTypeId) {
            m_snapdControl->snapRefresh();
            return DeviceManager::DeviceErrorNoError;
        } else if (action.actionTypeId() == snapdControlCheckUpdatesActionTypeId) {
            m_snapdControl->checkForUpdates();
            return DeviceManager::DeviceErrorNoError;
        }

        return DeviceManager::DeviceErrorActionTypeNotFound;

    } else if (device->deviceClassId() == snapDeviceClassId) {

        if (!m_snapdControl) {
            qCDebug(dcSnapd()) << "There is currently no snapd controller.";
            return DeviceManager::DeviceErrorHardwareFailure;
        }

        if (!m_snapdControl->connected()) {
            qCDebug(dcSnapd()) << "Snapd controller not connected to the backend.";
            return DeviceManager::DeviceErrorHardwareFailure;
        }

        if (action.actionTypeId() == snapChannelActionTypeId) {
            QString snapName = device->paramValue(snapNameParamTypeId).toString();
            m_snapdControl->changeSnapChannel(snapName, action.param(snapChannelStateParamTypeId).value().toString());
            return DeviceManager::DeviceErrorNoError;
        } else if (action.actionTypeId() == snapRevertActionTypeId) {
            QString snapName = device->paramValue(snapNameParamTypeId).toString();
            m_snapdControl->snapRevert(snapName);
            return DeviceManager::DeviceErrorNoError;
        }

        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginSnapd::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    if (paramTypeId == SnapdAdvancedModeParamTypeId) {
        qCDebug(dcSnapd()) << "Advanced mode" << (value.toBool() ? "enabled." : "disabled.");
        m_advancedMode = value.toBool();

        // If advanced mode disabled, clean up all snap devices
        if (!m_advancedMode) {
            foreach (const QString deviceSnapId, m_snapDevices.keys()) {
                Device *device = m_snapDevices.take(deviceSnapId);
                qCDebug(dcSnapd()) << "Remove device for snap" << device->paramValue(snapNameParamTypeId).toString();
                emit autoDeviceDisappeared(device->id());
            }
        } else {
            if (!m_snapdControl)
                return;

            m_snapdControl->update();
        }
    }
}

void DevicePluginSnapd::onRefreshTimer()
{
    if (!m_snapdControl) {
        startMonitoringAutoDevices();
        return;
    }

    m_snapdControl->update();
}

void DevicePluginSnapd::onUpdateTimer()
{
    if (!m_snapdControl)
        return;

    m_snapdControl->checkForUpdates();
}

void DevicePluginSnapd::onSnapListUpdated(const QVariantList &snapList)
{
    // Check for new snap devices only in advanced mode
    if (!m_advancedMode)
        return;

    // Collect list of snap id's from snapList for remove checking
    QStringList snapIdList;

    // Check if we have to create a new device
    foreach (const QVariant &snapVariant, snapList) {
        QVariantMap snapMap = snapVariant.toMap();
        snapIdList.append(snapMap.value("id").toString());
        // If there is no device for this snap yet
        if (!m_snapDevices.contains(snapMap.value("id").toString())) {
            DeviceDescriptor descriptor(snapDeviceClassId, QString("Snap %1").arg(snapMap.value("name").toString()));
            ParamList params;
            params.append(Param(snapNameParamTypeId, snapMap.value("name")));
            params.append(Param(snapIdParamTypeId, snapMap.value("id")));
            params.append(Param(snapSummaryParamTypeId, snapMap.value("summary")));
            params.append(Param(snapDescriptionParamTypeId, snapMap.value("description")));
            params.append(Param(snapDeveloperParamTypeId, snapMap.value("developer")));
            descriptor.setParams(params);

            emit autoDevicesAppeared(snapDeviceClassId, QList<DeviceDescriptor>() << descriptor);
        } else {
            // Update the states
            Device *device = m_snapDevices.value(snapMap.value("id").toString(), nullptr);
            if (!device) {
                qCWarning(dcSnapd()) << "Holding invalid snap device. This should never happen. Please report a bug if you see this message.";
                continue;
            }

            device->setStateValue(snapChannelStateTypeId, snapMap.value("channel").toString());
            device->setStateValue(snapVersionStateTypeId, snapMap.value("version").toString());
            device->setStateValue(snapRevisionStateTypeId, snapMap.value("revision").toString());
        }
    }

    // Check if we have to remove a device
    foreach (const QString deviceSnapId, m_snapDevices.keys()) {
        if (!snapIdList.contains(deviceSnapId)) {
            Device *device = m_snapDevices.take(deviceSnapId);
            qCDebug(dcSnapd()) << "The snap" << device->paramValue(snapNameParamTypeId).toString() << "is not installed any more.";
            emit autoDeviceDisappeared(device->id());
        }
    }
}
