/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#include "deviceplugini2ctools.h"
#include "devices/device.h"
#include "plugininfo.h"
#include <QDir>

extern "C" {
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
}

DevicePluginI2cTools::DevicePluginI2cTools()
{
}

void DevicePluginI2cTools::discoverDevices(DeviceDiscoveryInfo *info)
{
    DeviceClassId deviceClassId = info->deviceClassId();

    if (deviceClassId == i2cInterfaceDeviceClassId) {
        QList<QString> busses = getI2cBusses();
        foreach (QString adapter, busses) {
            DeviceDescriptor descriptor(i2cInterfaceDeviceClassId, adapter, "");
            ParamList params;
            params.append(Param(i2cInterfaceDeviceInterfaceParamTypeId, adapter));
            descriptor.setParams(params);
            info->addDeviceDescriptor(descriptor);
        }
        info->finish(Device::DeviceErrorNoError);
        return;
    } else if (deviceClassId == i2cReadRegisterDeviceClassId) {

        foreach (QFile *file, m_i2cDeviceFiles) {
            DeviceId parentDeviceId = m_i2cDeviceFiles.key(file);
            QString interfaceName = myDevices().findById(parentDeviceId)->name();
            QList<int> addresses = scanI2cBus(file->handle(), ScanModeAuto, m_fileFuncs.value(file), 0, 254);
            foreach (int address, addresses) {
                DeviceDescriptor descriptor(i2cReadRegisterDeviceClassId, QString("0x%0").arg(QString::number(address, 16)), "I2C interface: " + interfaceName, parentDeviceId);
                ParamList params;
                params.append(Param(i2cReadRegisterDeviceAddressParamTypeId, address));
                descriptor.setParams(params);
                info->addDeviceDescriptor(descriptor);
            }
        }
        info->finish(Device::DeviceErrorNoError);
        return;
    }
    qCWarning(dcI2cTools()) << "Discovery called for a deviceclass which does not support discovery? Device class ID:" << info->deviceClassId().toString();
    info->finish(Device::DeviceErrorDeviceClassNotFound);
}


void DevicePluginI2cTools::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    if (device->deviceClassId() == i2cInterfaceDeviceClassId) {
        qCDebug(dcI2cTools) << "Setup i2c interface";
        unsigned long funcs;

        QFile *i2cFile = new QFile("/dev/" + device->paramValue(i2cInterfaceDeviceInterfaceParamTypeId).toString());
        if (!i2cFile->exists()) {
            qCWarning(dcI2cTools()) << "Could not open I2C file";
            info->finish(Device::DeviceErrorSetupFailed);
            return;
        }

        if (!i2cFile->open(QFile::ReadWrite)) {
            qCWarning(dcI2cTools()) << "Could not open the given I2C file descriptor:" << i2cFile->fileName() << i2cFile->errorString();
            info->finish(Device::DeviceErrorSetupFailed);
            return;
        }

        if (ioctl(i2cFile->handle(), I2C_FUNCS, &funcs) < 0) {
            qCWarning(dcI2cTools()) << "Could not get the adapter functionality matrix" << strerror(errno);
            i2cFile->close();
            info->finish(Device::DeviceErrorSetupFailed);
            return ;
        }
        m_i2cDeviceFiles.insert(device->id(), i2cFile);
        m_fileFuncs.insert(i2cFile, funcs);

        info->finish(Device::DeviceErrorNoError);
        return;

    } else if (device->deviceClassId() == i2cReadRegisterDeviceClassId) {
        qCDebug(dcI2cTools) << "Setup i2c device";
        info->finish(Device::DeviceErrorNoError);
        return;
    }
    return info->finish(Device::DeviceErrorDeviceNotFound);
}


void DevicePluginI2cTools::postSetupDevice(Device *device)
{
    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(5 * 60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this] {
            foreach (Device *device, myDevices().filterByDeviceClassId(i2cInterfaceDeviceClassId)) {
                QFile *i2cFile = m_i2cDeviceFiles.value(device->id());
                unsigned long funcs = m_fileFuncs.value(i2cFile);
                QList<int> addresses = scanI2cBus(i2cFile->handle(), ScanModeAuto,funcs, 0, 254);
                QStringList states;
                foreach (int address, addresses) {
                  states.append(QString("0x%1").arg(address, 2, 16));
                }
                QString state = states.join(", ");
                if (state.isEmpty()) {
                  state = "--";
                }
                device->setStateValue(i2cInterfaceAvailableDevicesStateTypeId, state);


                if(!QFile::exists("/dev/"+(device->paramValue(i2cInterfaceDeviceInterfaceParamTypeId).toString()))) {
                    device->setStateValue(i2cInterfaceConnectedStateTypeId, false);
                } else {
                    device->setStateValue(i2cInterfaceConnectedStateTypeId, true);
                }
            }
        });
    }

    if (device->deviceClassId() == i2cInterfaceDeviceClassId) {
        QFile *i2cFile = m_i2cDeviceFiles.value(device->id());
        unsigned long funcs = m_fileFuncs.value(i2cFile);
        QList<int> addresses = scanI2cBus(i2cFile->handle(), ScanModeAuto,funcs, 0, 254);

        QStringList states;
        foreach (int address, addresses) {
          states.append(QString("0x%1").arg(address, 2, 16));
        }
        QString state = states.join(", ");
        if (state.isEmpty()) {
          state = "--";
        }
        device->setStateValue(i2cInterfaceAvailableDevicesStateTypeId, state);

        if(!QFile::exists("/dev/"+(device->paramValue(i2cInterfaceDeviceInterfaceParamTypeId).toByteArray()))) {
            device->setStateValue(i2cInterfaceConnectedStateTypeId, false);
        } else {
            device->setStateValue(i2cInterfaceConnectedStateTypeId, true);
        }

    } else if (device->deviceClassId() == i2cReadRegisterDeviceClassId) {
        QFile *i2cFile = m_i2cDeviceFiles.value(device->parentId());
        int slaveAddress = device->paramValue(i2cReadRegisterDeviceAddressParamTypeId).toInt();
        uint8_t command = device->paramValue(i2cReadRegisterDeviceRegisterParamTypeId).toUInt();
        if (ioctl(i2cFile->handle(), I2C_SLAVE, slaveAddress) < 0) {
            qCWarning(dcI2cTools()) << "Could not access slave" << slaveAddress;
            device->setStateValue(i2cReadRegisterConnectedStateTypeId, false);
            return;
        }
        int value = i2c_smbus_read_word_data(i2cFile->handle(), command);
        device->setStateValue(i2cReadRegisterConnectedStateTypeId, true);
        device->setStateValue(i2cReadRegisterValueStateTypeId, value);
    }
}


void DevicePluginI2cTools::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == i2cInterfaceDeviceClassId) {
        QFile *i2cFile = m_i2cDeviceFiles.take(device->id());
        m_fileFuncs.remove(i2cFile);
        i2cFile->close();
    }

    if (myDevices().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}


void DevicePluginI2cTools::onPluginTimer()
{
    foreach (Device *device, myDevices().filterByDeviceClassId(i2cInterfaceDeviceClassId)) {
        QFile *i2cFile = m_i2cDeviceFiles.value(device->id());
        unsigned long funcs = m_fileFuncs.value(i2cFile);
        QList<int> addresses = scanI2cBus(i2cFile->handle(), ScanModeAuto,funcs, 0, 254);

        QStringList states;
        foreach (int address, addresses) {
          states.append(QString("0x%1").arg(address, 2, 16));
        }
        QString state = states.join(", ");
        if (state.isEmpty()) {
          state = "--";
        }

        device->setStateValue(i2cInterfaceAvailableDevicesStateTypeId, state);

        if(!QFile::exists("/dev/"+(device->paramValue(i2cInterfaceDeviceInterfaceParamTypeId).toString()))) {
            device->setStateValue(i2cInterfaceConnectedStateTypeId, false);
        } else {
            device->setStateValue(i2cInterfaceConnectedStateTypeId, true);
        }
    }
}

QStringList DevicePluginI2cTools::getI2cBusses(void)
{
    return QDir("/sys/class/i2c-adapter/").entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
}

QList<int> DevicePluginI2cTools::scanI2cBus(int file, ScanMode mode, unsigned long funcs, int first, int last)
{
    int cmd, res;
    QList<int> deviceAddresses;
    qCDebug(dcI2cTools()) << "Start scanning i2c bus";

    for (int address = 0; address < 128; address++) {

        /* Select detection command for this address */
        if (mode == ScanModeAuto) {
            if ((address >= 0x30 && address <= 0x37)
                    || (address >= 0x50 && address <= 0x5F))
                cmd = ScanModeRead;
            else
                cmd = ScanModeQuick;
        } else {
            cmd = mode;
        }

        /* Skip unwanted addresses */
        if (address < first || address > last
                || (cmd == ScanModeRead &&
                    !(funcs & I2C_FUNC_SMBUS_READ_BYTE))
                || (cmd == ScanModeQuick &&
                    !(funcs & I2C_FUNC_SMBUS_QUICK))) {
            continue;
        }

        /* Set slave address */
        if (ioctl(file, I2C_SLAVE, address) < 0) {
            if (errno == EBUSY) {
                qCDebug(dcI2cTools()) << "Device found but it seems busy <UU>";
                continue;
            } else {
                qCWarning(dcI2cTools()) << "Error: Could not set address" << strerror(errno);
                continue;
            }
        }

        /* Probe this address */
        if(cmd == ScanModeRead) {
            /* This is known to lock SMBus on various
                   write-only chips (mainly clock chips) */
            res = i2c_smbus_read_byte(file);
        } else {
            /* MODE_QUICK */
            /* This is known to corrupt the Atmel AT24RF08 EEPROM */
            res = i2c_smbus_write_quick(file, I2C_SMBUS_WRITE);
        }

        if (res < 0) {
            //qCDebug(dcI2cTools()) << "No device on this address";
        } else {
            deviceAddresses.append(address);
            qCDebug(dcI2cTools()) << "Found device with address:" << address;
        }
    }
    return deviceAddresses;
}
