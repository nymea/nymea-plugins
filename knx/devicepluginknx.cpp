/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "plugininfo.h"
#include "devicepluginknx.h"

#include <QObject>
#include <QDateTime>
#include <QKnxAddress>
#include <QKnx2ByteFloat>
#include <QNetworkInterface>
#include <QKnx8BitUnsignedValue>
#include <QKnxGroupAddressInfos>

DevicePluginKnx::DevicePluginKnx()
{

}

void DevicePluginKnx::init()
{
    m_discovery = new KnxServerDiscovery(this);
    connect(m_discovery, &KnxServerDiscovery::discoveryFinished, this, &DevicePluginKnx::onDiscoveryFinished);

    connect(this, &DevicePluginKnx::configValueChanged, this, &DevicePluginKnx::onPluginConfigurationChanged);
}

void DevicePluginKnx::startMonitoringAutoDevices()
{
    // Start seaching for devices which can be discovered and added automatically
}

DeviceManager::DeviceError DevicePluginKnx::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)

    if (deviceClassId == knxNetIpServerDeviceClassId) {
        if (!m_discovery->startDisovery()) {
            return DeviceManager::DeviceErrorDeviceInUse;
        } else {
            return DeviceManager::DeviceErrorAsync;
        }
    }

    return DeviceManager::DeviceErrorNoError;
}

DeviceManager::DeviceSetupStatus DevicePluginKnx::setupDevice(Device *device)
{
    qCDebug(dcKnx()) << "Setup device" << device->name() << device->params();

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(300);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginKnx::onPluginTimerTimeout);
    }

    if (device->deviceClassId() == knxNetIpServerDeviceClassId) {
        QHostAddress remoteAddress = QHostAddress(device->paramValue(knxNetIpServerDeviceAddressParamTypeId).toString());
        KnxTunnel *tunnel = new KnxTunnel(remoteAddress, this);
        connect(tunnel, &KnxTunnel::connectedChanged, this, &DevicePluginKnx::onTunnelConnectedChanged);
        connect(tunnel, &KnxTunnel::frameReceived, this, &DevicePluginKnx::onTunnelFrameReceived);
        m_tunnels.insert(tunnel, device);
    }

    if (device->deviceClassId() == knxShutterDeviceClassId) {
        QHostAddress tunnelAddress =  QHostAddress(device->paramValue(knxShutterDeviceAddressParamTypeId).toString());
        KnxTunnel *tunnel = nullptr;
        foreach (KnxTunnel *knxTunnel, m_tunnels.keys()) {
            if (knxTunnel->remoteAddress() == tunnelAddress) {
                tunnel = knxTunnel;
            }
        }

        if (!tunnel) {
            qCWarning(dcKnx()) << "Could not find tunnel for address" << tunnelAddress.toString();
            return DeviceManager::DeviceSetupStatusFailure;
        }

        device->setParentId(m_tunnels.value(tunnel)->id());
    }

    if (device->deviceClassId() == knxLightDeviceClassId) {
        QHostAddress tunnelAddress =  QHostAddress(device->paramValue(knxLightDeviceAddressParamTypeId).toString());
        KnxTunnel *tunnel = nullptr;
        foreach (KnxTunnel *knxTunnel, m_tunnels.keys()) {
            if (knxTunnel->remoteAddress() == tunnelAddress) {
                tunnel = knxTunnel;
            }
        }

        if (!tunnel) {
            qCWarning(dcKnx()) << "Could not find tunnel for address" << tunnelAddress.toString();
            return DeviceManager::DeviceSetupStatusFailure;
        }

        device->setParentId(m_tunnels.value(tunnel)->id());
    }

    if (device->deviceClassId() == knxDimmableLightDeviceClassId) {
        QHostAddress tunnelAddress =  QHostAddress(device->paramValue(knxDimmableLightDeviceAddressParamTypeId).toString());
        KnxTunnel *tunnel = nullptr;
        foreach (KnxTunnel *knxTunnel, m_tunnels.keys()) {
            if (knxTunnel->remoteAddress() == tunnelAddress) {
                tunnel = knxTunnel;
            }
        }

        if (!tunnel) {
            qCWarning(dcKnx()) << "Could not find tunnel for address" << tunnelAddress.toString();
            return DeviceManager::DeviceSetupStatusFailure;
        }

        device->setParentId(m_tunnels.value(tunnel)->id());
    }

    return DeviceManager::DeviceSetupStatusSuccess;
}

void DevicePluginKnx::postSetupDevice(Device *device)
{
    qCDebug(dcKnx()) << "Post setup device" << device->name() << device->params();
    if (device->deviceClassId() == knxNetIpServerDeviceClassId) {
        KnxTunnel *tunnel = m_tunnels.key(device);
        tunnel->connectTunnel();
        if (configValue(knxPluginGenericDevicesEnabledParamTypeId).toBool()) {
            createGenericDevices(device);
        } else {
            destroyGenericDevices(device);
        }
    }

    if (device->deviceClassId() == knxGenericSwitchDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (!tunnel) return;
        device->setStateValue(knxGenericSwitchConnectedStateTypeId, tunnel->connected());

        // Read initial state
        if (tunnel->connected()) {
            QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxGenericSwitchDeviceKnxAddressParamTypeId).toString());
            tunnel->readKnxGroupValue(knxAddress);
        }
    }

    if (device->deviceClassId() == knxGenericUpDownDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (tunnel) device->setStateValue(knxGenericUpDownConnectedStateTypeId, tunnel->connected());
    }

    if (device->deviceClassId() == knxGenericScalingDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (tunnel) device->setStateValue(knxGenericScalingConnectedStateTypeId, tunnel->connected());
    }

    if (device->deviceClassId() == knxGenericTemperatureSensorDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (!tunnel) return;
        device->setStateValue(knxGenericTemperatureSensorConnectedStateTypeId, tunnel->connected());

        // Read initial state
        if (tunnel->connected()) {
            QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxGenericTemperatureSensorDeviceKnxAddressParamTypeId).toString());
            tunnel->readKnxGroupValue(knxAddress);
        }
    }

    if (device->deviceClassId() == knxGenericLightSensorDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (!tunnel) return;
        device->setStateValue(knxGenericLightSensorConnectedStateTypeId, tunnel->connected());

        // Read initial state
        if (tunnel->connected()) {
            QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxGenericLightSensorDeviceKnxAddressParamTypeId).toString());
            tunnel->readKnxGroupValue(knxAddress);
        }
    }

    if (device->deviceClassId() == knxGenericWindSpeedSensorDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (!tunnel) return;
        device->setStateValue(knxGenericWindSpeedSensorConnectedStateTypeId, tunnel->connected());

        // Read initial state
        if (tunnel->connected()) {
            QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxGenericWindSpeedSensorDeviceKnxAddressParamTypeId).toString());
            tunnel->readKnxGroupValue(knxAddress);
        }
    }


    if (device->deviceClassId() == knxShutterDeviceClassId) {
        QHostAddress tunnelAddress =  QHostAddress(device->paramValue(knxShutterDeviceAddressParamTypeId).toString());
        KnxTunnel *tunnel = nullptr;
        foreach (KnxTunnel *knxTunnel, m_tunnels.keys()) {
            if (knxTunnel->remoteAddress() == tunnelAddress) {
                tunnel = knxTunnel;
            }
        }

        if (!tunnel) return;
        device->setStateValue(knxShutterConnectedStateTypeId, tunnel->connected());
    }

    if (device->deviceClassId() == knxLightDeviceClassId) {
        QHostAddress tunnelAddress =  QHostAddress(device->paramValue(knxLightDeviceAddressParamTypeId).toString());
        KnxTunnel *tunnel = nullptr;
        foreach (KnxTunnel *knxTunnel, m_tunnels.keys()) {
            if (knxTunnel->remoteAddress() == tunnelAddress) {
                tunnel = knxTunnel;
            }
        }

        if (!tunnel) return;
        device->setStateValue(knxLightConnectedStateTypeId, tunnel->connected());

        // Read initial state
        if (tunnel->connected()) {
            QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxLightDeviceKnxAddressParamTypeId).toString());
            tunnel->readKnxGroupValue(knxAddress);
        }
    }

    if (device->deviceClassId() == knxDimmableLightDeviceClassId) {
        QHostAddress tunnelAddress =  QHostAddress(device->paramValue(knxDimmableLightDeviceAddressParamTypeId).toString());
        KnxTunnel *tunnel = nullptr;
        foreach (KnxTunnel *knxTunnel, m_tunnels.keys()) {
            if (knxTunnel->remoteAddress() == tunnelAddress) {
                tunnel = knxTunnel;
            }
        }

        if (tunnel) device->setStateValue(knxDimmableLightConnectedStateTypeId, tunnel->connected());

        // Read initial state
        if (tunnel->connected()) {
            QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxDimmableLightDeviceKnxSwitchAddressParamTypeId).toString());
            tunnel->readKnxGroupValue(knxAddress);
            // TODO: read brightness
        }
    }
}

void DevicePluginKnx::deviceRemoved(Device *device)
{
    qCDebug(dcKnx()) << "Remove device" << device->name() << device->params();
    if (device->deviceClassId() == knxNetIpServerDeviceClassId) {
        KnxTunnel *tunnel = m_tunnels.key(device);
        m_tunnels.remove(tunnel);
        tunnel->disconnectTunnel();
        tunnel->deleteLater();
    }

    if (myDevices().isEmpty() && m_pluginTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

DeviceManager::DeviceError DevicePluginKnx::executeAction(Device *device, const Action &action)
{
    qCDebug(dcKnx()) << "Executing action for device" << device->name() << action.actionTypeId().toString() << action.params();

    // Knx NetIp server
    if (device->deviceClassId() == knxNetIpServerDeviceClassId) {
        if (action.actionTypeId() == knxNetIpServerAutoCreateDevicesActionTypeId) {
            autoCreateKnownDevices(device);
            return DeviceManager::DeviceErrorNoError;
        }
    }

    // Generic switch
    if (device->deviceClassId() == knxGenericSwitchDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (!tunnel) {
            qCWarning(dcKnx()) << "Could not find tunnel for this device";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (!tunnel->connected()) {
            qCWarning(dcKnx()) << "The corresponding tunnel is not connected.";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxGenericSwitchDeviceKnxAddressParamTypeId).toString());

        if (action.actionTypeId() == knxGenericSwitchPowerActionTypeId) {
            tunnel->sendKnxDpdSwitchFrame(knxAddress, action.param(knxGenericSwitchPowerActionPowerParamTypeId).value().toBool());
        }

        if (action.actionTypeId() == knxGenericSwitchReadActionTypeId) {
            tunnel->readKnxGroupValue(knxAddress);
        }
    }

    // Generic UpDown
    if (device->deviceClassId() == knxGenericUpDownDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (!tunnel) {
            qCWarning(dcKnx()) << "Could not find tunnel for this device";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (!tunnel->connected()) {
            qCWarning(dcKnx()) << "The corresponding tunnel is not connected.";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }
        QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxGenericUpDownDeviceKnxAddressParamTypeId).toString());

        if (action.actionTypeId() == knxGenericUpDownOpenActionTypeId) {
            tunnel->sendKnxDpdUpDownFrame(knxAddress, true);
        }

        if (action.actionTypeId() == knxGenericUpDownCloseActionTypeId) {
            QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxGenericUpDownDeviceKnxAddressParamTypeId).toString());
            tunnel->sendKnxDpdUpDownFrame(knxAddress, false);
        }
    }

    // Generic Scaling
    if (device->deviceClassId() == knxGenericScalingDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (!tunnel) {
            qCWarning(dcKnx()) << "Could not find tunnel for this device";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (!tunnel->connected()) {
            qCWarning(dcKnx()) << "The corresponding tunnel is not connected.";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxGenericScalingDeviceKnxAddressParamTypeId).toString());
        int scaling = action.param(knxGenericScalingScaleActionScaleParamTypeId).value().toInt();
        tunnel->sendKnxDpdScalingFrame(knxAddress, scaling);
    }

    // Generic Temperature
    if (device->deviceClassId() == knxGenericTemperatureSensorDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (!tunnel) {
            qCWarning(dcKnx()) << "Could not find tunnel for this device";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (!tunnel->connected()) {
            qCWarning(dcKnx()) << "The corresponding tunnel is not connected.";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxGenericTemperatureSensorDeviceKnxAddressParamTypeId).toString());

        if (action.actionTypeId() == knxGenericTemperatureSensorReadActionTypeId) {
            qCDebug(dcKnx()) << "Send temperature read request" << knxAddress.toString();
            tunnel->readKnxGroupValue(knxAddress);
        }
    }

    // Generic Light sensor
    if (device->deviceClassId() == knxGenericLightSensorDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (!tunnel) {
            qCWarning(dcKnx()) << "Could not find tunnel for this device";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (!tunnel->connected()) {
            qCWarning(dcKnx()) << "The corresponding tunnel is not connected.";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxGenericLightSensorDeviceKnxAddressParamTypeId).toString());

        if (action.actionTypeId() == knxGenericLightSensorReadActionTypeId) {
            qCDebug(dcKnx()) << "Send temperature read request" << knxAddress.toString();
            tunnel->readKnxGroupValue(knxAddress);
        }
    }

    // Generic wind speed sensor
    if (device->deviceClassId() == knxGenericWindSpeedSensorDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (!tunnel) {
            qCWarning(dcKnx()) << "Could not find tunnel for this device";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (!tunnel->connected()) {
            qCWarning(dcKnx()) << "The corresponding tunnel is not connected.";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxGenericWindSpeedSensorDeviceKnxAddressParamTypeId).toString());

        if (action.actionTypeId() == knxGenericWindSpeedSensorReadActionTypeId) {
            qCDebug(dcKnx()) << "Send wind speed read request" << knxAddress.toString();
            tunnel->readKnxGroupValue(knxAddress);
        }
    }

    // Shutter
    if (device->deviceClassId() == knxShutterDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (!tunnel) {
            qCWarning(dcKnx()) << "Could not find tunnel for this device";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (!tunnel->connected()) {
            qCWarning(dcKnx()) << "The corresponding tunnel is not connected.";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        QKnxAddress knxAddressStep = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxShutterDeviceKnxAddressStepParamTypeId).toString());
        QKnxAddress knxAddressUpDown = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxShutterDeviceKnxAddressUpDownParamTypeId).toString());

        if (action.actionTypeId() == knxShutterOpenActionTypeId) {
            // Note: first send step, then delayed the up/down command
            tunnel->sendKnxDpdStepFrame(knxAddressStep, false);
            tunnel->sendKnxDpdUpDownFrame(knxAddressUpDown, true);
            return DeviceManager::DeviceErrorNoError;
        }

        if (action.actionTypeId() == knxShutterCloseActionTypeId) {
            // Note: first send step, then delayed the up/down command
            tunnel->sendKnxDpdStepFrame(knxAddressStep, true);
            tunnel->sendKnxDpdUpDownFrame(knxAddressUpDown, false);
            return DeviceManager::DeviceErrorNoError;
        }

        if (action.actionTypeId() == knxShutterStopActionTypeId) {
            tunnel->sendKnxDpdStepFrame(knxAddressStep, true);
            return DeviceManager::DeviceErrorNoError;
        }
    }

    // Light
    if (device->deviceClassId() == knxLightDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (!tunnel) {
            qCWarning(dcKnx()) << "Could not find tunnel for this device";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (!tunnel->connected()) {
            qCWarning(dcKnx()) << "The corresponding tunnel is not connected.";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxLightDeviceKnxAddressParamTypeId).toString());
        if (action.actionTypeId() == knxLightPowerActionTypeId) {
            tunnel->sendKnxDpdSwitchFrame(knxAddress, action.param(knxLightPowerActionPowerParamTypeId).value().toBool());
        }

        if (action.actionTypeId() == knxLightReadActionTypeId) {
            tunnel->readKnxGroupValue(knxAddress);
        }
    }

    //  Dimmable Light
    if (device->deviceClassId() == knxDimmableLightDeviceClassId) {
        KnxTunnel *tunnel = getTunnelForDevice(device);
        if (!tunnel) {
            qCWarning(dcKnx()) << "Could not find tunnel for this device";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (!tunnel->connected()) {
            qCWarning(dcKnx()) << "The corresponding tunnel is not connected.";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        QKnxAddress knxSwitchAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxDimmableLightDeviceKnxSwitchAddressParamTypeId).toString());
        QKnxAddress knxScalingAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxDimmableLightDeviceKnxScalingAddressParamTypeId).toString());

        if (action.actionTypeId() == knxDimmableLightPowerActionTypeId) {
            tunnel->sendKnxDpdSwitchFrame(knxSwitchAddress, action.param(knxDimmableLightPowerActionPowerParamTypeId).value().toBool());
        }

        if (action.actionTypeId() == knxDimmableLightBrightnessActionTypeId) {
            int percentage = action.param(knxDimmableLightBrightnessActionBrightnessParamTypeId).value().toInt();
            int scaled = qRound(percentage * 255.0 / 100.0);
            qCDebug(dcKnx()) << "Percentage" << percentage << "-->" << scaled;
            device->setStateValue(knxDimmableLightBrightnessStateTypeId, percentage);
            tunnel->sendKnxDpdScalingFrame(knxScalingAddress, percentage);
        }

        if (action.actionTypeId() == knxDimmableLightReadActionTypeId) {
            tunnel->readKnxGroupValue(knxSwitchAddress);
            tunnel->readKnxGroupValue(knxScalingAddress);
        }
    }

    return DeviceManager::DeviceErrorNoError;
}

KnxTunnel *DevicePluginKnx::getTunnelForDevice(Device *device)
{
    Device *parentDevice = nullptr;
    foreach (Device *d, myDevices()) {
        if (d->id() == device->parentId()) {
            parentDevice = d;
        }
    }

    if (!parentDevice) {
        qCWarning(dcKnx()) << "Could not find parent device for" << device->name() << device->id().toString();
        return nullptr;
    }

    return m_tunnels.key(parentDevice);
}

void DevicePluginKnx::autoCreateKnownDevices(Device *parentDevice)
{
    // Load project file and compair name/DataPoint values
    if (parentDevice->deviceClassId() != knxNetIpServerDeviceClassId) {
        qCWarning(dcKnx()) << "Cannot create generic devices for" << parentDevice->name() << ". This is not a NetIpServer device";
        return;
    }

    // Load project file
    QString projectFilePath = parentDevice->paramValue(knxNetIpServerDeviceKnxProjectFileParamTypeId).toString();
    QKnxGroupAddressInfos groupInformation(projectFilePath);
    qCDebug(dcKnx()) << "Opening project file" << groupInformation.projectFile();
    if (!groupInformation.parse()) {
        qCWarning(dcKnx()) << "Could not parse project file" << groupInformation.projectFile() << groupInformation.errorString();
        return;
    }

    QHash<QString, QList<QKnxDatapointType::Type>> nameDatapointHash;

    qCDebug(dcKnx()) << "Found" << groupInformation.projectIds().count() << "project ids";
    foreach (const QString &projectId, groupInformation.projectIds()) {
        qCDebug(dcKnx()) << "Project id" << projectId << groupInformation.projectName(projectId);
        foreach (const QString &installation, groupInformation.installations(projectId)) {
            QVector<QKnxGroupAddressInfo> groupAddresses = groupInformation.addressInfos(projectId, installation);
            qCDebug(dcKnx()) << "Installation contains" << groupAddresses.count() << "group addresses.";
            foreach (const QKnxGroupAddressInfo &addressInfo, groupAddresses) {
                if (nameDatapointHash.keys().contains(addressInfo.name())) {
                    QList<QKnxDatapointType::Type> dataPointList = nameDatapointHash.value(addressInfo.name());
                    if (!dataPointList.contains(addressInfo.datapointType())) {
                        dataPointList.append(addressInfo.datapointType());
                        nameDatapointHash[addressInfo.name()] = dataPointList;
                    }
                } else {
                    nameDatapointHash.insert(addressInfo.name(), QList<QKnxDatapointType::Type>() << addressInfo.datapointType());
                }
            }
        }
    }

    qCDebug(dcKnx()) << "Show name datapoint list";
    foreach (const QString &name, nameDatapointHash.keys()) {
        qCDebug(dcKnx()) << "-->" << name;
        QList<QKnxDatapointType::Type> dataPointList = nameDatapointHash.value(name);

        foreach (const QKnxDatapointType::Type &type, dataPointList) {
            qCDebug(dcKnx()) << "    -" << type;
        }

        // TODO: find known data point combinations

    }
}

void DevicePluginKnx::createGenericDevices(Device *parentDevice)
{
    // Make sure the feature is enabled
    if (!configValue(knxPluginGenericDevicesEnabledParamTypeId).toBool()) {
        qCDebug(dcKnx()) << "Do not scan the project file for autocreation of devices";
        return;
    }

    if (parentDevice->deviceClassId() != knxNetIpServerDeviceClassId) {
        qCWarning(dcKnx()) << "Cannot create generic devices for" << parentDevice->name() << ". This is not a NetIpServer device";
        return;
    }

    // Load project file
    QString projectFilePath = parentDevice->paramValue(knxNetIpServerDeviceKnxProjectFileParamTypeId).toString();
    QKnxGroupAddressInfos groupInformation(projectFilePath);
    qCDebug(dcKnx()) << "Opening project file" << groupInformation.projectFile();
    if (!groupInformation.parse()) {
        qCWarning(dcKnx()) << "Could not parse project file" << groupInformation.projectFile() << groupInformation.errorString();
        return;
    }

    // Create descriptor lists for generic devices
    QList<DeviceDescriptor> knxGenericSwitchDeviceDescriptors;
    QList<DeviceDescriptor> knxGenericUpDownDeviceDescriptors;
    QList<DeviceDescriptor> knxGenericScalingDeviceDescriptors;
    QList<DeviceDescriptor> knxGenericTemperatureSensorDeviceDescriptors;
    QList<DeviceDescriptor> knxGenericLightSensorDeviceDescriptors;
    QList<DeviceDescriptor> knxGenericWindSpeedSensorDeviceDescriptors;

    qCDebug(dcKnx()) << "Found" << groupInformation.projectIds().count() << "project ids";
    foreach (const QString &projectId, groupInformation.projectIds()) {
        qCDebug(dcKnx()) << "Project id" << projectId << groupInformation.projectName(projectId);
        foreach (const QString &installation, groupInformation.installations(projectId)) {
            QVector<QKnxGroupAddressInfo> groupAddresses = groupInformation.addressInfos(projectId, installation);
            qCDebug(dcKnx()) << "Installation contains" << groupAddresses.count() << "group addresses.";

            foreach (const QKnxGroupAddressInfo &addressInfo, groupAddresses) {
                qCDebug(dcKnx()) << "-" << addressInfo.name();
                qCDebug(dcKnx()) << "    Description:" << addressInfo.description();
                qCDebug(dcKnx()) << "    Address:" << addressInfo.address().toString();
                qCDebug(dcKnx()) << "    DataPoint Type:" << addressInfo.datapointType();
                switch (addressInfo.datapointType()) {
                case QKnxDatapointType::Type::DptSwitch: {
                    // Check if we already added a switch device
                    ParamList params;
                    params.append(Param(knxGenericSwitchDeviceKnxNameParamTypeId, addressInfo.name()));
                    params.append(Param(knxGenericSwitchDeviceKnxAddressParamTypeId, addressInfo.address().toString()));
                    Device *device = findDeviceByParams(params);

                    // If there is already a device with this params, continue
                    if (device) continue;

                    qCDebug(dcKnx()) << "Found new switch" << params;

                    DeviceDescriptor descriptor(knxGenericSwitchDeviceClassId, addressInfo.name() + " (" + tr("Generic Switch") + ")", addressInfo.address().toString(), parentDevice->id());
                    descriptor.setParams(params);
                    knxGenericSwitchDeviceDescriptors.append(descriptor);
                    break;
                }
                case QKnxDatapointType::Type::DptUpDown: {
                    // Check if we already added a shutter device
                    ParamList params;
                    params.append(Param(knxGenericUpDownDeviceKnxNameParamTypeId, addressInfo.name()));
                    params.append(Param(knxGenericUpDownDeviceKnxAddressParamTypeId, addressInfo.address().toString()));
                    Device *device = findDeviceByParams(params);

                    // If there is already a device with this params, continue
                    if (device) continue;

                    qCDebug(dcKnx()) << "Found new shutter" << params;

                    DeviceDescriptor descriptor(knxGenericUpDownDeviceClassId, addressInfo.name() + " (" + tr("Generic UpDown") + ")", addressInfo.address().toString(), parentDevice->id());
                    descriptor.setParams(params);
                    knxGenericUpDownDeviceDescriptors.append(descriptor);
                    break;
                }
                case QKnxDatapointType::Type::DptScaling: {
                    // Check if we already added a dimmer device
                    ParamList params;
                    params.append(Param(knxGenericScalingDeviceKnxNameParamTypeId, addressInfo.name()));
                    params.append(Param(knxGenericScalingDeviceKnxAddressParamTypeId, addressInfo.address().toString()));
                    Device *device = findDeviceByParams(params);

                    // If there is already a device with this params, continue
                    if (device) continue;

                    qCDebug(dcKnx()) << "Found new dimmer" << params;
                    DeviceDescriptor descriptor(knxGenericScalingDeviceClassId, addressInfo.name() + " (" + tr("Generic Scaling") + ")", addressInfo.address().toString(), parentDevice->id());
                    descriptor.setParams(params);
                    knxGenericScalingDeviceDescriptors.append(descriptor);
                    break;
                }
                case QKnxDatapointType::Type::DptTemperatureCelsius: {
                    // Check if we already added a generic temperature sensor device
                    ParamList params;
                    params.append(Param(knxGenericTemperatureSensorDeviceKnxNameParamTypeId, addressInfo.name()));
                    params.append(Param(knxGenericTemperatureSensorDeviceKnxAddressParamTypeId, addressInfo.address().toString()));
                    Device *device = findDeviceByParams(params);

                    // If there is already a device with this params, continue
                    if (device) continue;

                    qCDebug(dcKnx()) << "Found new temperature sensor" << params;
                    DeviceDescriptor descriptor(knxGenericTemperatureSensorDeviceClassId, addressInfo.name() + " (" + tr("Generic temperature sensor") + ")", addressInfo.address().toString(), parentDevice->id());
                    descriptor.setParams(params);
                    knxGenericTemperatureSensorDeviceDescriptors.append(descriptor);
                    break;
                }
                case QKnxDatapointType::Type::DptValueLux: {
                    // Check if we already added a generic light sensor device
                    ParamList params;
                    params.append(Param(knxGenericLightSensorDeviceKnxNameParamTypeId, addressInfo.name()));
                    params.append(Param(knxGenericLightSensorDeviceKnxAddressParamTypeId, addressInfo.address().toString()));
                    Device *device = findDeviceByParams(params);

                    // If there is already a device with this params, continue
                    if (device) continue;

                    qCDebug(dcKnx()) << "Found new light sensor" << params;
                    DeviceDescriptor descriptor(knxGenericLightSensorDeviceClassId, addressInfo.name() + " (" + tr("Generic light sensor") + ")", addressInfo.address().toString(), parentDevice->id());
                    descriptor.setParams(params);
                    knxGenericLightSensorDeviceDescriptors.append(descriptor);
                    break;
                }
                case QKnxDatapointType::Type::DptWindSpeed: {
                    // Check if we already added a generic wind speed sensor device
                    ParamList params;
                    params.append(Param(knxGenericWindSpeedSensorDeviceKnxNameParamTypeId, addressInfo.name()));
                    params.append(Param(knxGenericWindSpeedSensorDeviceKnxAddressParamTypeId, addressInfo.address().toString()));
                    Device *device = findDeviceByParams(params);

                    // If there is already a device with this params, continue
                    if (device) continue;

                    qCDebug(dcKnx()) << "Found new wind speed sensor" << params;
                    DeviceDescriptor descriptor(knxGenericWindSpeedSensorDeviceClassId, addressInfo.name() + " (" + tr("Generic wind speed sensor") + ")", addressInfo.address().toString(), parentDevice->id());
                    descriptor.setParams(params);
                    knxGenericWindSpeedSensorDeviceDescriptors.append(descriptor);
                    break;
                }
                default:
                    qCWarning(dcKnx()) << "Unkandled DataPoint Type" << addressInfo.datapointType();
                    break;
                }
            }
        }
    }

    if (!knxGenericSwitchDeviceDescriptors.isEmpty()) {
        qCDebug(dcKnx()) << "--> Found" << knxGenericSwitchDeviceDescriptors.count() << "new KNX switch devices";
        emit autoDevicesAppeared(knxGenericSwitchDeviceClassId, knxGenericSwitchDeviceDescriptors);
    }

    if (!knxGenericUpDownDeviceDescriptors.isEmpty()) {
        qCDebug(dcKnx()) << "--> Found" << knxGenericUpDownDeviceDescriptors.count() << "new KNX shutter devices";
        emit autoDevicesAppeared(knxGenericUpDownDeviceClassId, knxGenericUpDownDeviceDescriptors);
    }

    if (!knxGenericScalingDeviceDescriptors.isEmpty()) {
        qCDebug(dcKnx()) << "--> Found" << knxGenericScalingDeviceDescriptors.count() << "new KNX dimmer devices";
        emit autoDevicesAppeared(knxGenericScalingDeviceClassId, knxGenericScalingDeviceDescriptors);
    }

    if (!knxGenericTemperatureSensorDeviceDescriptors.isEmpty()) {
        qCDebug(dcKnx()) << "--> Found" << knxGenericTemperatureSensorDeviceDescriptors.count() << "new KNX temperature sensor devices";
        emit autoDevicesAppeared(knxGenericTemperatureSensorDeviceClassId, knxGenericTemperatureSensorDeviceDescriptors);
    }

    if (!knxGenericLightSensorDeviceDescriptors.isEmpty()) {
        qCDebug(dcKnx()) << "--> Found" << knxGenericLightSensorDeviceDescriptors.count() << "new KNX light sensor devices";
        emit autoDevicesAppeared(knxGenericLightSensorDeviceClassId, knxGenericLightSensorDeviceDescriptors);
    }

    if (!knxGenericWindSpeedSensorDeviceDescriptors.isEmpty()) {
        qCDebug(dcKnx()) << "--> Found" << knxGenericWindSpeedSensorDeviceDescriptors.count() << "new KNX wind speed sensor devices";
        emit autoDevicesAppeared(knxGenericWindSpeedSensorDeviceClassId, knxGenericWindSpeedSensorDeviceDescriptors);
    }
}

void DevicePluginKnx::destroyGenericDevices(Device *parentDevice)
{
    foreach (Device *d, myDevices()) {
        if (d->parentId() == parentDevice->id()) {
            if (d->deviceClassId() == knxGenericSwitchDeviceClassId
                    || d->deviceClassId() == knxGenericUpDownDeviceClassId
                    || d->deviceClassId() == knxGenericScalingDeviceClassId) {
                qCDebug(dcKnx()) << "--> Destroy generic knx device" << d->name() << d->id();
                emit autoDeviceDisappeared(d->id());
            }
        }
    }
}

void DevicePluginKnx::onPluginTimerTimeout()
{
    qCDebug(dcKnx()) << "Refresh sensor data from KNX devices";
    foreach (Device *device, myDevices()) {

        // Refresh temperature sensor data
        if (device->deviceClassId() == knxGenericTemperatureSensorDeviceClassId) {
            KnxTunnel *tunnel = getTunnelForDevice(device);
            if (!tunnel) {
                qCWarning(dcKnx()) << "Could not find tunnel for this device";
                return;
            }

            if (tunnel->connected()) {
                QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxGenericTemperatureSensorDeviceKnxAddressParamTypeId).toString());
                tunnel->readKnxGroupValue(knxAddress);
            }
        }

        // Refresh light sensor data
        if (device->deviceClassId() == knxGenericLightSensorDeviceClassId) {
            KnxTunnel *tunnel = getTunnelForDevice(device);
            if (!tunnel) {
                qCWarning(dcKnx()) << "Could not find tunnel for this device";
                return;
            }

            if (tunnel->connected()) {
                QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, device->paramValue(knxGenericLightSensorDeviceKnxAddressParamTypeId).toString());
                tunnel->readKnxGroupValue(knxAddress);
            }
        }
    }
}

void DevicePluginKnx::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    if (paramTypeId == knxPluginGenericDevicesEnabledParamTypeId) {
        if (value.toBool()) {
            qCDebug(dcKnx()) << "Generic Knx devices enabled.";
            // Add all generic devices
            foreach (Device *knxNetIpServer, m_tunnels.values()) {
                qCDebug(dcKnx()) << "Start adding generic knx devices from project file of" << knxNetIpServer->name() << knxNetIpServer->id().toString();
                createGenericDevices(knxNetIpServer);
            }

        } else {
            qCDebug(dcKnx()) << "Generic Knx devices disabled";
            // Remove all generic devices
            foreach (Device *knxNetIpServer, m_tunnels.values()) {
                qCDebug(dcKnx()) << "Start removing generic knx devices from project file of" << knxNetIpServer->name() << knxNetIpServer->id().toString();
                destroyGenericDevices(knxNetIpServer);
            }
        }
    }
}

void DevicePluginKnx::onDiscoveryFinished()
{
    qCDebug(dcKnx()) << "Discovery finished.";
    QList<DeviceDescriptor> deviceDescriptors;
    foreach (const QKnxNetIpServerInfo &serverInfo, m_discovery->discoveredServers()) {
        qCDebug(dcKnx()) << "Found server:" << QString("%1:%2").arg(serverInfo.controlEndpointAddress().toString()).arg(serverInfo.controlEndpointPort());
        KnxServerDiscovery::printServerInfo(serverInfo);
        DeviceDescriptor descriptor(knxNetIpServerDeviceClassId, "KNX NetIp Server", QString("%1:%2").arg(serverInfo.controlEndpointAddress().toString()).arg(serverInfo.controlEndpointPort()));
        ParamList params;
        params.append(Param(knxNetIpServerDeviceAddressParamTypeId, serverInfo.controlEndpointAddress().toString()));
        params.append(Param(knxNetIpServerDevicePortParamTypeId, serverInfo.controlEndpointPort()));
        descriptor.setParams(params);
        foreach (Device *existingDevice, myDevices()) {
            if (existingDevice->paramValue(knxNetIpServerDeviceAddressParamTypeId).toString() == serverInfo.controlEndpointAddress().toString()) {
                descriptor.setDeviceId(existingDevice->id());
                break;
            }
        }
        deviceDescriptors.append(descriptor);
    }

    emit devicesDiscovered(knxNetIpServerDeviceClassId, deviceDescriptors);
}

void DevicePluginKnx::onTunnelConnectedChanged()
{
    KnxTunnel *tunnel = static_cast<KnxTunnel *>(sender());
    Device *device = m_tunnels.value(tunnel);
    if (!device) return;
    device->setStateValue(knxNetIpServerConnectedStateTypeId, tunnel->connected());

    // Update child devices connected state
    foreach (Device *d, myDevices()) {
        if (d->parentId() == device->id()) {
            if (d->deviceClassId() == knxGenericSwitchDeviceClassId) {
                d->setStateValue(knxGenericSwitchConnectedStateTypeId, tunnel->connected());
                // Read initial state
                if (tunnel->connected()) {
                    QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, d->paramValue(knxGenericSwitchDeviceKnxAddressParamTypeId).toString());
                    tunnel->readKnxGroupValue(knxAddress);
                }
            } else if (d->deviceClassId() == knxGenericUpDownDeviceClassId) {
                d->setStateValue(knxGenericUpDownConnectedStateTypeId, tunnel->connected());
            } else if (d->deviceClassId() == knxGenericScalingDeviceClassId) {
                d->setStateValue(knxGenericScalingConnectedStateTypeId, tunnel->connected());
            } else if (d->deviceClassId() == knxGenericTemperatureSensorDeviceClassId) {
                d->setStateValue(knxGenericTemperatureSensorConnectedStateTypeId, tunnel->connected());
                // Read initial state
                if (tunnel->connected()) {
                    QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, d->paramValue(knxGenericTemperatureSensorDeviceKnxAddressParamTypeId).toString());
                    tunnel->readKnxGroupValue(knxAddress);
                }
            } else if (d->deviceClassId() == knxGenericLightSensorDeviceClassId) {
                d->setStateValue(knxGenericLightSensorConnectedStateTypeId, tunnel->connected());
                // Read initial state
                if (tunnel->connected()) {
                    QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, d->paramValue(knxGenericLightSensorDeviceKnxAddressParamTypeId).toString());
                    tunnel->readKnxGroupValue(knxAddress);
                }
            } else if (d->deviceClassId() == knxGenericWindSpeedSensorDeviceClassId) {
                d->setStateValue(knxGenericWindSpeedSensorConnectedStateTypeId, tunnel->connected());
                // Read initial state
                if (tunnel->connected()) {
                    QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, d->paramValue(knxGenericWindSpeedSensorDeviceKnxAddressParamTypeId).toString());
                    tunnel->readKnxGroupValue(knxAddress);
                }
            } else if (d->deviceClassId() == knxShutterDeviceClassId) {
                d->setStateValue(knxShutterConnectedStateTypeId, tunnel->connected());
            } else if (d->deviceClassId() == knxLightDeviceClassId) {
                d->setStateValue(knxLightConnectedStateTypeId, tunnel->connected());
                // Read initial state
                if (tunnel->connected()) {
                    QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, d->paramValue(knxLightDeviceKnxAddressParamTypeId).toString());
                    tunnel->readKnxGroupValue(knxAddress);
                }
            } else if (d->deviceClassId() == knxDimmableLightDeviceClassId) {
                d->setStateValue(knxDimmableLightConnectedStateTypeId, tunnel->connected());
                // Read initial state
                if (tunnel->connected()) {
                    QKnxAddress knxAddress = QKnxAddress(QKnxAddress::Type::Group, d->paramValue(knxDimmableLightDeviceKnxSwitchAddressParamTypeId).toString());
                    tunnel->readKnxGroupValue(knxAddress);
                    // TODO: read brightness
                }
            } else {
                qCWarning(dcKnx()) << "Unhandled device class on tunnel connected changed" << d->deviceClassId();
            }
        }
    }
}

void DevicePluginKnx::onTunnelFrameReceived(const QKnxLinkLayerFrame &frame)
{
    foreach (Device *device, myDevices()) {

        // Generic switch
        if (device->deviceClassId() == knxGenericSwitchDeviceClassId) {
            if (device->paramValue(knxGenericSwitchDeviceKnxAddressParamTypeId).toString() == frame.destinationAddress().toString()) {
                bool power = static_cast<bool>(frame.tpdu().data().toByteArray().at(0));
                qCDebug(dcKnx()) << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss")
                                 << "Generic Switch notification"
                                 << device->name()
                                 << frame.sourceAddress().toString() << "-->" << frame.destinationAddress().toString()
                                 << frame.tpdu().data().toHex().toByteArray()
                                 << (!frame.tpdu().data().toByteArray().isEmpty() ? QString("%1").arg(power) : "");

                if (!frame.tpdu().data().toByteArray().isEmpty()) {
                    device->setStateValue(knxGenericSwitchPowerStateTypeId, power);
                }
            }
        }

        // Generic UpDown
        if (device->deviceClassId() == knxGenericUpDownDeviceClassId) {
            if (device->paramValue(knxGenericUpDownDeviceKnxAddressParamTypeId).toString() == frame.destinationAddress().toString()) {
                qCDebug(dcKnx()) << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss")
                                 << "Generic UpDown notification"
                                 << device->name()
                                 << frame.sourceAddress().toString() << "-->" << frame.destinationAddress().toString()
                                 << frame.tpdu().data().toHex().toByteArray();
            }
        }

        // Generic Scaling
        if (device->deviceClassId() == knxGenericScalingDeviceClassId) {
            if (device->paramValue(knxGenericScalingDeviceKnxAddressParamTypeId).toString() == frame.destinationAddress().toString()) {
                QKnxScaling knxScaling;
                knxScaling.setBytes(frame.tpdu().data(), 0, 1);
                int scaling = static_cast<int>(knxScaling.value());
                int percentage = qRound(scaling * 100.0 / 255.0);
                qCDebug(dcKnx()) << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss")
                                 << "Generic Scaling notification"
                                 << device->name()
                                 << frame.sourceAddress().toString() << "-->" << frame.destinationAddress().toString()
                                 << frame.tpdu().data().toHex().toByteArray()
                                 << (!frame.tpdu().data().toByteArray().isEmpty() ? QString("%1 %2 [%]").arg(scaling).arg(percentage)  : "");

                if (!frame.tpdu().data().toByteArray().isEmpty()) {
                    device->setStateValue(knxGenericScalingScaleStateTypeId, percentage);
                }
            }
        }

        // Generic temperature sensor
        if (device->deviceClassId() == knxGenericTemperatureSensorDeviceClassId) {
            if (device->paramValue(knxGenericTemperatureSensorDeviceKnxAddressParamTypeId).toString() == frame.destinationAddress().toString()) {

                QKnxTemperatureCelsius knxTemperature;
                knxTemperature.setBytes(frame.tpdu().data(), 0, 2);
                double temperature = static_cast<double>(knxTemperature.value());
                qCDebug(dcKnx()) << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss")
                                 << "Generic Temperature sensor notification"
                                 << device->name()
                                 << frame.sourceAddress().toString() << "-->" << frame.destinationAddress().toString()
                                 << frame.tpdu().data().toHex().toByteArray()
                                 << (!frame.tpdu().data().toByteArray().isEmpty() ? QString("%1  [°C]").arg(temperature) : "");

                if (!frame.tpdu().data().toByteArray().isEmpty()) {
                    device->setStateValue(knxGenericTemperatureSensorTemperatureStateTypeId, temperature);
                }
            }
        }


        // Generic light sensor
        if (device->deviceClassId() == knxGenericLightSensorDeviceClassId) {
            if (device->paramValue(knxGenericLightSensorDeviceKnxAddressParamTypeId).toString() == frame.destinationAddress().toString()) {

                QKnxValueLux knxLightIntensity;
                knxLightIntensity.setBytes(frame.tpdu().data(), 0, 2);
                double lightIntensity = static_cast<double>(knxLightIntensity.value());
                qCDebug(dcKnx()) << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss")
                                 << "Generic light sensor notification"
                                 << device->name()
                                 << frame.sourceAddress().toString() << "-->" << frame.destinationAddress().toString()
                                 << frame.tpdu().data().toHex().toByteArray()
                                 << (!frame.tpdu().data().toByteArray().isEmpty() ? QString("%1 [Lux]").arg(lightIntensity) : "");

                if (!frame.tpdu().data().toByteArray().isEmpty()) {
                    device->setStateValue(knxGenericLightSensorLightIntensityStateTypeId, lightIntensity);
                }
            }
        }

        // Generic wind speed sensor
        if (device->deviceClassId() == knxGenericWindSpeedSensorDeviceClassId) {
            if (device->paramValue(knxGenericWindSpeedSensorDeviceKnxAddressParamTypeId).toString() == frame.destinationAddress().toString()) {

                QKnxWindSpeed knxWindSpeed;
                knxWindSpeed.setBytes(frame.tpdu().data(), 0, 2);
                double windSpeed = static_cast<double>(knxWindSpeed.value());
                qCDebug(dcKnx()) << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss")
                                 << "Generic light sensor notification"
                                 << device->name()
                                 << frame.sourceAddress().toString() << "-->" << frame.destinationAddress().toString()
                                 << frame.tpdu().data().toHex().toByteArray()
                                 << (!frame.tpdu().data().toByteArray().isEmpty() ? QString("%1 [m/s]").arg(windSpeed) : "");

                if (!frame.tpdu().data().toByteArray().isEmpty()) {
                    device->setStateValue(knxGenericWindSpeedSensorWindSpeedStateTypeId, windSpeed);
                }
            }
        }

        // Shutter
        if (device->deviceClassId() == knxShutterDeviceClassId) {
            if (device->paramValue(knxShutterDeviceKnxAddressStepParamTypeId).toString() == frame.destinationAddress().toString()) {
                qCDebug(dcKnx()) << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss")
                                 << "Shutter Step notification"
                                 << device->name()
                                 << frame.sourceAddress().toString() << "-->" << frame.destinationAddress().toString()
                                 << frame.tpdu().data().toHex().toByteArray();


            } else if (device->paramValue(knxShutterDeviceKnxAddressUpDownParamTypeId).toString() == frame.destinationAddress().toString()) {
                qCDebug(dcKnx()) << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss")
                                 << "Shutter UpDown notification"
                                 << device->name()
                                 << frame.sourceAddress().toString() << "-->" << frame.destinationAddress().toString()
                                 << frame.tpdu().data().toHex().toByteArray();
            }
        }

        // Light
        if (device->deviceClassId() == knxLightDeviceClassId) {
            if (device->paramValue(knxLightDeviceKnxAddressParamTypeId).toString() == frame.destinationAddress().toString()) {
                bool power = static_cast<bool>(frame.tpdu().data().toByteArray().at(0));
                qCDebug(dcKnx()) << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss")
                                 << "Light notification"
                                 << device->name()
                                 << frame.sourceAddress().toString() << "-->" << frame.destinationAddress().toString()
                                 << frame.tpdu().data().toHex().toByteArray()
                                 << (!frame.tpdu().data().toByteArray().isEmpty() ? QString("%1").arg(power) : "");

                if (!frame.tpdu().data().toByteArray().isEmpty()) {
                    device->setStateValue(knxLightPowerStateTypeId, power);
                }
            }
        }

        // Dimmable light
        if (device->deviceClassId() == knxDimmableLightDeviceClassId) {

            // Switch
            if (device->paramValue(knxDimmableLightDeviceKnxSwitchAddressParamTypeId).toString() == frame.destinationAddress().toString()) {
                bool power = static_cast<bool>(frame.tpdu().data().toByteArray().at(0));
                qCDebug(dcKnx()) << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss")
                                 << "Dimmable light Switch notification"
                                 << device->name()
                                 << frame.sourceAddress().toString() << "-->" << frame.destinationAddress().toString()
                                 << frame.tpdu().data().toHex().toByteArray()
                                 << (!frame.tpdu().data().toByteArray().isEmpty() ? QString("%1").arg(power) : "");

                if (!frame.tpdu().data().toByteArray().isEmpty()) {
                    device->setStateValue(knxDimmableLightPowerStateTypeId, power);
                }
            }

            // Scale
            if (device->paramValue(knxDimmableLightDeviceKnxScalingAddressParamTypeId).toString() == frame.destinationAddress().toString()) {

                QKnxScaling knxScaling;
                knxScaling.setBytes(frame.tpdu().data(), 0, 1);
                double scaling = knxScaling.value();
                int percentage = qRound(scaling * 100.0 / 255.0);
                qCDebug(dcKnx()) << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss")
                                 << "Dimmable light scaling notification"
                                 << device->name()
                                 << frame.sourceAddress().toString() << "-->" << frame.destinationAddress().toString()
                                 << frame.tpdu().data().toHex().toByteArray()
                                 << (!frame.tpdu().data().toByteArray().isEmpty() ? QString("%1 %2 [%]").arg(scaling).arg(percentage)  : "");

                //                if (!frame.tpdu().data().toByteArray().isEmpty()) {
                //                    device->setStateValue(knxDimmableLightBrightnessStateTypeId, percentage);
                //                }
            }

        }
    }
}

