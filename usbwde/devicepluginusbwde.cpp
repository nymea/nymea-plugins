/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Alexander Lampret <alexander.lampret@gmail.com>     *
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

#include "devicepluginusbwde.h"

#include "plugin/device.h"
#include "devicemanager.h"
#include "plugininfo.h"

#include <QDateTime>

DevicePluginUsbWde::DevicePluginUsbWde()
{

}

DevicePluginUsbWde::~DevicePluginUsbWde()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void DevicePluginUsbWde::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginUsbWde::onPluginTimer);
}

DeviceManager::DeviceSetupStatus DevicePluginUsbWde::setupDevice(Device *device)
{
    if (device->deviceClassId() == wdeBridgeDeviceClassId) {
        if (m_bridgeDevice != 0) {
            qCWarning(dcUsbWde) << "Only one USB WDE device can be configured.";
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_serialPort = new QSerialPort(this);
        m_serialPort->setPortName(device->paramValue(wdeBridgeInterfaceParamTypeId).toString());
        m_serialPort->setBaudRate(device->paramValue(wdeBridgeBaudrateParamTypeId).toInt());
        if (!m_serialPort->open(QIODevice::ReadOnly)) {
            qCWarning(dcUsbWde) << device->name() << "can't bind to interface" << device->paramValue(wdeBridgeInterfaceParamTypeId);
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_bridgeDevice = device;
        connect(m_serialPort, SIGNAL(readyRead()), SLOT(handleReadyRead()));
        connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), SLOT(handleError(QSerialPort::SerialPortError)));
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == temperatureSensorDeviceClassId) {
        m_deviceList.insert(device->paramValue(temperatureSensorChannelParamTypeId).toInt(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == windRainSensorDeviceClassId) {
        m_deviceList.insert(device->paramValue(windRainSensorChannelParamTypeId).toInt(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}

void DevicePluginUsbWde::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == wdeBridgeDeviceClassId) {
        m_serialPort->close();
        m_bridgeDevice = 0;
    } else if (device->deviceClassId() == temperatureSensorDeviceClassId) {
        m_deviceList.remove(device->paramValue(temperatureSensorChannelParamTypeId).toInt());
    } else if (device->deviceClassId() == windRainSensorDeviceClassId) {
        m_deviceList.remove(device->paramValue(windRainSensorChannelParamTypeId).toInt());
    }
}

void DevicePluginUsbWde::handleReadyRead()
{
    m_readData.append(m_serialPort->readAll());
}

void DevicePluginUsbWde::handleError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError) {
        qCWarning(dcUsbWde) << "An I/O error occurred while reading the data from port " << m_serialPort->portName() << ", error: " << m_serialPort->errorString();
    }
}

void DevicePluginUsbWde::createNewSensor(int channel)
{
    DeviceClassId createClassId;
    QString deviceName;
    QList<DeviceDescriptor> deviceDescriptors;
    ParamList params;
    if (channel == 9) {
        createClassId = windRainSensorDeviceClassId;
        deviceName = "Weather station";
        params.append(Param(windRainSensorNameParamTypeId, "Sensor " + QString::number(channel)));
        params.append(Param(windRainSensorChannelParamTypeId, channel));
    } else {
        createClassId = temperatureSensorDeviceClassId;
        deviceName = "Sensor channel " + QString::number(channel);
        params.append(Param(temperatureSensorNameParamTypeId, "Sensor " + QString::number(channel)));
        params.append(Param(temperatureSensorChannelParamTypeId, channel));
    }
    DeviceDescriptor descriptor(createClassId, deviceName, deviceName);
    descriptor.setParams(params);
    deviceDescriptors.append(descriptor);
    emit autoDevicesAppeared(createClassId, deviceDescriptors);
}

void DevicePluginUsbWde::onPluginTimer()
{
    if (!m_readData.isEmpty()) {
        // Handle data
        QList<QByteArray> parts = m_readData.split(';');
        QLocale german(QLocale::German);
        // Check if received string is valid (should start with $1 and ends with 0)
        if (parts.size() != 25 || !parts.at(0).contains("$1") || !parts.at(24).contains("0")) {
            m_readData.clear();
            return;
        }
        // Loop through 8 possible sensor channels
        for (int i = 1; i < 9; i++) {
            if (!parts.at(2 + i).isEmpty()) {
                // Create new device if it does not exist
                if (!m_deviceList.contains(i)) {
                    createNewSensor(i);
                } else {
                    Device* device = m_deviceList.value(i);
                    device->setStateValue(temperatureSensorTemperatureStateTypeId, german.toDouble(parts.at(2+i)));
                    device->setStateValue(temperatureSensorHumidityStateTypeId, parts.at(2+i+8).toInt());
                    device->setStateValue(temperatureSensorLastUpdateStateTypeId, QDateTime::currentDateTime().toTime_t());
                }
            }
        }
        // Check if wind data is available
        if (!parts.at(19).isEmpty()) {
            // Create new device if it does not exist
            if (!m_deviceList.contains(9)) {
                createNewSensor(9);
            } else {
                Device* device = m_deviceList.value(9);
                device->setStateValue(windRainSensorTemperatureStateTypeId, german.toDouble(parts.at(19)));
                device->setStateValue(windRainSensorHumidityStateTypeId, parts.at(20).toInt());
                device->setStateValue(windRainSensorWindStrengthStateTypeId, german.toDouble(parts.at(21)));
                device->setStateValue(windRainSensorRainStrengthStateTypeId, german.toDouble(parts.at(22)));
                device->setStateValue(windRainSensorIsRainStateTypeId, (parts.at(23) == "1"));
                device->setStateValue(windRainSensorLastUpdateStateTypeId, QDateTime::currentDateTime().toTime_t());
            }
        }
        m_readData.clear();
    }
}
