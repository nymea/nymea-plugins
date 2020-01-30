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

#include "devicepluginsnapd.h"
#include "devices/device.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

DevicePluginSnapd::DevicePluginSnapd()
{

}

DevicePluginSnapd::~DevicePluginSnapd()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_updateTimer);
}

void DevicePluginSnapd::init()
{
    // Initialize plugin configurations
    m_advancedMode = configValue(snapdPluginAdvancedModeParamTypeId).toBool();
    m_refreshTime = configValue(snapdPluginRefreshScheduleParamTypeId).toInt();
    connect(this, &DevicePluginSnapd::configValueChanged, this, &DevicePluginSnapd::onPluginConfigurationChanged);

    // Refresh timer for snapd checks
    m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
    connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginSnapd::onRefreshTimer);

    // Check all 4 hours if there is an update available
    m_updateTimer = hardwareManager()->pluginTimerManager()->registerTimer(14400);
    connect(m_updateTimer, &PluginTimer::timeout, this, &DevicePluginSnapd::onUpdateTimer);
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
        emit autoDevicesAppeared({descriptor});
    }
}

void DevicePluginSnapd::postSetupDevice(Device *device)
{
    if (m_snapdControl && m_snapdControl->device() == device) {
        m_snapdControl->update();
    }
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

void DevicePluginSnapd::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    qCDebug(dcSnapd()) << "Setup" << device->name() << device->params();

    if (device->deviceClassId() == snapdControlDeviceClassId) {
        if (m_snapdControl) {
            delete m_snapdControl;
            m_snapdControl = nullptr;
        }

        m_snapdControl = new SnapdControl(device, this);
        m_snapdControl->setPreferredRefreshTime(configValue(snapdPluginRefreshScheduleParamTypeId).toInt());
        connect(m_snapdControl, &SnapdControl::snapListUpdated, this, &DevicePluginSnapd::onSnapListUpdated);

    } else if (device->deviceClassId() == snapDeviceClassId) {
        device->setName(QString("%1").arg(device->paramValue(snapDeviceNameParamTypeId).toString()));
        m_snapDevices.insert(device->paramValue(snapDeviceIdParamTypeId).toString(), device);
    }

    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginSnapd::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == snapdControlDeviceClassId) {

        if (!m_snapdControl) {
            qCDebug(dcSnapd()) << "There is currently no snapd controller.";
            return info->finish(Device::DeviceErrorHardwareFailure);
        }

        if (!m_snapdControl->connected()) {
            qCDebug(dcSnapd()) << "Snapd controller not connected to to backend.";
            return info->finish(Device::DeviceErrorHardwareFailure);
        }

        if (action.actionTypeId() == snapdControlStartUpdateActionTypeId) {
            m_snapdControl->snapRefresh();
            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == snapdControlCheckUpdatesActionTypeId) {
            m_snapdControl->checkForUpdates();
            return info->finish(Device::DeviceErrorNoError);
        }

        return info->finish(Device::DeviceErrorActionTypeNotFound);

    } else if (device->deviceClassId() == snapDeviceClassId) {

        if (!m_snapdControl) {
            qCDebug(dcSnapd()) << "There is currently no snapd controller.";
            return info->finish(Device::DeviceErrorHardwareFailure);
        }

        if (!m_snapdControl->connected()) {
            qCDebug(dcSnapd()) << "Snapd controller not connected to the backend.";
            return info->finish(Device::DeviceErrorHardwareFailure);
        }

        if (action.actionTypeId() == snapChannelActionTypeId) {
            QString snapName = device->paramValue(snapDeviceNameParamTypeId).toString();
            m_snapdControl->changeSnapChannel(snapName, action.param(snapChannelActionChannelParamTypeId).value().toString());
            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == snapRevertActionTypeId) {
            QString snapName = device->paramValue(snapDeviceNameParamTypeId).toString();
            m_snapdControl->snapRevert(snapName);
            return info->finish(Device::DeviceErrorNoError);
        }

        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }

    return info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginSnapd::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    qCDebug(dcSnapd()) << "Plugin configuration changed";

    // Check advanced mode
    if (paramTypeId == snapdPluginAdvancedModeParamTypeId) {
        qCDebug(dcSnapd()) << "Advanced mode" << (value.toBool() ? "enabled." : "disabled.");
        m_advancedMode = value.toBool();

        // If advanced mode disabled, clean up all snap devices
        if (!m_advancedMode) {
            foreach (const QString deviceSnapId, m_snapDevices.keys()) {
                Device *device = m_snapDevices.take(deviceSnapId);
                qCDebug(dcSnapd()) << "Remove device for snap" << device->paramValue(snapDeviceNameParamTypeId).toString();
                emit autoDeviceDisappeared(device->id());
            }
        } else {
            if (!m_snapdControl)
                return;

            m_snapdControl->update();
        }
    }

    // Check refresh schedule
    if (paramTypeId == snapdPluginRefreshScheduleParamTypeId) {
        if (!m_snapdControl)
            return;

        m_refreshTime = value.toInt();
        qCDebug(dcSnapd()) << "Refresh schedule start time" << QTime(m_refreshTime, 0, 0).toString("hh:mm");
        m_snapdControl->setPreferredRefreshTime(m_refreshTime);
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
            params.append(Param(snapDeviceNameParamTypeId, snapMap.value("name")));
            params.append(Param(snapDeviceIdParamTypeId, snapMap.value("id")));
            params.append(Param(snapDeviceSummaryParamTypeId, snapMap.value("summary")));
            params.append(Param(snapDeviceDescriptionParamTypeId, snapMap.value("description")));
            params.append(Param(snapDeviceDeveloperParamTypeId, snapMap.value("developer")));
            descriptor.setParams(params);

            emit autoDevicesAppeared({descriptor});
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
            qCDebug(dcSnapd()) << "The snap" << device->paramValue(snapDeviceNameParamTypeId).toString() << "is not installed any more.";
            emit autoDeviceDisappeared(device->id());
        }
    }
}
