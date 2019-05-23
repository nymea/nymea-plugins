/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Simon Stürz <simon.stuerz@nymea.io>                 *
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
#include "devicepluginhoneywellscanner.h"

#include <QSerialPort>
#include <QSerialPortInfo>

DevicePluginHoneywellScanner::DevicePluginHoneywellScanner()
{

}

void DevicePluginHoneywellScanner::init()
{
    // Initialize/create objects
}

void DevicePluginHoneywellScanner::startMonitoringAutoDevices()
{
    // Start seaching for devices which can be discovered and added automatically
}

void DevicePluginHoneywellScanner::postSetupDevice(Device *device)
{
    qCDebug(dcHoneywellScanner()) << "Post setup device" << device->name() << device->params();

    if (device->deviceClassId() == scannerDeviceClassId) {
        HoneywellScanner *scanner = m_scanners.value(device);
        scanner->enable();
        //scanner->reset();
    }

}

void DevicePluginHoneywellScanner::deviceRemoved(Device *device)
{
    qCDebug(dcHoneywellScanner()) << "Remove device" << device->name() << device->params();
    if (device->deviceClassId() == scannerDeviceClassId) {
        HoneywellScanner *scanner = m_scanners.take(device);
        scanner->deleteLater();
    }
}

DeviceManager::DeviceError DevicePluginHoneywellScanner::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)

    QList<DeviceDescriptor> deviceDescriptors;
    if (deviceClassId == scannerDeviceClassId) {

        // Scan serial ports
        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
            qCDebug(dcHoneywellScanner()) << "Found serial port" << info.portName();
            qCDebug(dcHoneywellScanner()) << "   Description:" << info.description();
            qCDebug(dcHoneywellScanner()) << "   System location:" << info.systemLocation();
            qCDebug(dcHoneywellScanner()) << "   Manufacturer:" << info.manufacturer();
            qCDebug(dcHoneywellScanner()) << "   Serialnumber:" << info.serialNumber();
            if (info.hasProductIdentifier()) {
                qCDebug(dcHoneywellScanner()) << "   Product identifier:" << info.productIdentifier();
            }
            if (info.hasVendorIdentifier()) {
                qCDebug(dcHoneywellScanner()) << "   Vendor identifier:" << info.vendorIdentifier();
            }

            if (info.vendorIdentifier() != 3118 && info.productIdentifier() != 3786) {
                qCDebug(dcHoneywellScanner()) << "This device does not seem to be a Honeywell Scanner according to the vendor/product id. Not adding to result list.";
                continue;
            }

            uint baudrate = 115200;
            ParamList params;
            params.append(Param(scannerDeviceSerialNumberParamTypeId, info.serialNumber()));
            params.append(Param(scannerDeviceSerialPortParamTypeId, info.systemLocation()));
            params.append(Param(scannerDeviceBaudrateParamTypeId, baudrate));

            qCDebug(dcHoneywellScanner()) << "Using baudrate param" << params.paramValue(scannerDeviceBaudrateParamTypeId);

            DeviceDescriptor descriptor(scannerDeviceClassId);
            descriptor.setTitle(info.manufacturer().simplified() + " - " + info.description().simplified());
            descriptor.setDescription(info.systemLocation());
            descriptor.setParams(params);
            deviceDescriptors.append(descriptor);
        }

        emit devicesDiscovered(deviceClassId, deviceDescriptors);
        return DeviceManager::DeviceErrorAsync;
    }

    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

DeviceManager::DeviceSetupStatus DevicePluginHoneywellScanner::setupDevice(Device *device)
{
    qCDebug(dcHoneywellScanner()) << "Setup device" << device->name() << device->params();
    if (device->deviceClassId() == scannerDeviceClassId) {
        QString serialPortName = device->paramValue(scannerDeviceSerialPortParamTypeId).toString();
        qint32 serialBaudrate = static_cast<qint32>(device->paramValue(scannerDeviceBaudrateParamTypeId).toInt());
        HoneywellScanner *scanner = new HoneywellScanner(serialPortName, serialBaudrate, this);

        connect(scanner, &HoneywellScanner::connectedChanged, this, [device](bool connected) {
            qCDebug(dcHoneywellScanner()) << "Scanner" << device << "connected changed to" << connected;
            device->setStateValue(scannerConnectedStateTypeId, connected);
        });

        connect(scanner, &HoneywellScanner::codeScanned, this, [device, this](const QString &code) {
            qCDebug(dcHoneywellScanner()) << "Scanner" << device << "code scanned" << code;
            device->setStateValue(scannerScannedCodeStateTypeId, code);
            emit emitEvent(Event(scannerTriggeredEventTypeId, device->id()));
        });

        m_scanners.insert(device, scanner);
    }

    return DeviceManager::DeviceSetupStatusSuccess;
}

DeviceManager::DeviceError DevicePluginHoneywellScanner::executeAction(Device *device, const Action &action)
{
    qCDebug(dcHoneywellScanner()) << "Executing action for device" << device->name() << action.actionTypeId().toString() << action.params();

    if (device->deviceClassId() == scannerDeviceClassId) {

        HoneywellScanner *scanner = m_scanners.value(device);
        if (!scanner->connected()) {
            qCWarning(dcHoneywellScanner()) << "Could not execute action. The sensor is not connected.";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        // Reset
        if (action.actionTypeId() == scannerResetActionTypeId) {
            if (!scanner->reset()) {
                return DeviceManager::DeviceErrorHardwareFailure;
            } else {
                return DeviceManager::DeviceErrorNoError;
            }
        }

        // Defaults
        if (action.actionTypeId() == scannerDefaultsActionTypeId) {
            if (!scanner->configureDefaults()) {
                return DeviceManager::DeviceErrorHardwareFailure;
            } else {
                return DeviceManager::DeviceErrorNoError;
            }
        }

        // Trigger mode
        if (action.actionTypeId() == scannerModeActionTypeId) {
            QString modeString = action.param(scannerModeActionModeParamTypeId).value().toString();
            HoneywellScanner::Mode mode = HoneywellScanner::ModePresentation;
            if (modeString.toLower() == "manual") {
                mode = HoneywellScanner::ModeManual;
            }

            if (!scanner->configureMode(mode)) {
                return DeviceManager::DeviceErrorHardwareFailure;
            } else {
                return DeviceManager::DeviceErrorNoError;
            }
        }


        // Trigger
        if (action.actionTypeId() == scannerTriggerActionTypeId) {
            if (!scanner->configureTrigger(true)) {
                return DeviceManager::DeviceErrorHardwareFailure;
            } else {
                return DeviceManager::DeviceErrorNoError;
            }
        }

        // LED power
        if (action.actionTypeId() == scannerLedPowerActionTypeId) {
            if (!scanner->configureLedPower(action.param(scannerLedPowerActionLedPowerParamTypeId).value().toBool())) {
                return DeviceManager::DeviceErrorHardwareFailure;
            } else {
                return DeviceManager::DeviceErrorNoError;
            }
        }

        // Power up beep
        if (action.actionTypeId() == scannerPowerUpBeepActionTypeId) {
            if (!scanner->configurePowerUpBeep(action.param(scannerPowerUpBeepActionPowerUpBeepParamTypeId).value().toBool())) {
                return DeviceManager::DeviceErrorHardwareFailure;
            } else {
                return DeviceManager::DeviceErrorNoError;
            }
        }
    }

    return DeviceManager::DeviceErrorNoError;
}
