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

#include <QDebug>
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
        QList<i2c_adap> busses = getI2cBusses();
        foreach(i2c_adap adapter, busses) {
            DeviceDescriptor descriptor(i2cInterfaceDeviceClassId, QString("i2c-%0").arg(adapter.nr), QString(adapter.algo));
            ParamList params;
            params.append(Param(i2cInterfaceDeviceInterfaceParamTypeId, adapter.nr));
            descriptor.setParams(params);
            info->addDeviceDescriptor(descriptor);
        }
        info->finish(Device::DeviceErrorNoError);
        return;
    } else if (deviceClassId == i2cReadRegisterDeviceClassId) {


        foreach (int file, m_i2cDeviceFiles) {
            DeviceId parentDeviceId = m_i2cDeviceFiles.key(file);
            QString interfaceName = myDevices().findById(parentDeviceId)->name();
            QList<int> addresses = scanI2cBus(file, ScanModeAuto, m_fileFuncs.value(file), 0, 254);
            foreach(int address, addresses) {
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
        int file;
        char filename[20];
        file = open_i2c_dev(device->paramValue(i2cInterfaceDeviceInterfaceParamTypeId).toInt(), filename, sizeof(filename), 0);
        if (file < 0) {
            qWarning(dcI2cTools()) << "Could not open I2C file";
            info->finish(Device::DeviceErrorSetupFailed);
            return;
        }

        if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
            qWarning(dcI2cTools()) << "Could not get the adapter functionality matrix" << strerror(errno);
            close(file);
            info->finish(Device::DeviceErrorSetupFailed);
            return ;
        }
        m_i2cDeviceFiles.insert(device->id(), file);
        m_fileFuncs.insert(file, funcs);

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
                Q_UNUSED(device);
            }
        });
    }

    if (device->deviceClassId() == i2cInterfaceDeviceClassId) {
        int file = m_i2cDeviceFiles.value(device->id());
        unsigned long funcs = m_fileFuncs.value(file);
        QList<int> addresses = scanI2cBus(file, ScanModeAuto,funcs, 0, 254);
        QString state;
        foreach(int address, addresses) {
            state.append("0x");
            state.append(QString::number(address, 16));
            state.append(", ");
        }
        if (state.size() > 2) {
            state.resize(state.size() - 2); //remove last colon
        } else {
            state = "--";
        }
        device->setStateValue(i2cInterfaceAvailableDevicesStateTypeId, state);

        if(lookup_i2c_bus(device->paramValue(i2cInterfaceDeviceInterfaceParamTypeId).toByteArray()) == -1) {
            device->setStateValue(i2cInterfaceConnectedStateTypeId, false);
        } else {
            device->setStateValue(i2cInterfaceConnectedStateTypeId, true);
        }
    } else if (device->deviceClassId() == i2cReadRegisterDeviceClassId) {
        int file = m_i2cDeviceFiles.value(device->parentId());
        int slaveAddress = device->paramValue(i2cReadRegisterDeviceAddressParamTypeId).toInt();
        uint8_t command = device->paramValue(i2cReadRegisterDeviceRegisterParamTypeId).toUInt();
        if (ioctl(file, I2C_SLAVE, slaveAddress) < 0) {
            qWarning(dcI2cTools()) << "Could not access slave" << slaveAddress;
            device->setStateValue(i2cReadRegisterConnectedStateTypeId, false);
            return;
        }
        int value = i2c_smbus_read_word_data(file, command);
        device->setStateValue(i2cReadRegisterConnectedStateTypeId, true);
        device->setStateValue(i2cReadRegisterValueStateTypeId, value);
    }
}


void DevicePluginI2cTools::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == i2cInterfaceDeviceClassId) {
        int file = m_i2cDeviceFiles.take(device->id());
        m_fileFuncs.remove(file);
        close(file);
    }

    if (myDevices().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}


void DevicePluginI2cTools::onPluginTimer()
{
    foreach(Device *device, myDevices().filterByDeviceClassId(i2cInterfaceDeviceClassId)) {
        int file = m_i2cDeviceFiles.value(device->id());
        unsigned long funcs = m_fileFuncs.value(file);
        QList<int> addresses = scanI2cBus(file, ScanModeAuto,funcs, 0, 254);
        QString state;
        foreach(int address, addresses) {
            state.append("0x");
            state.append(QString::number(address, 16));
            state.append(", ");
        }
        if (state.size() > 2) {
            state.resize(state.size() - 2); //remove last colon
        } else {
            state = "--";
        }

        device->setStateValue(i2cInterfaceAvailableDevicesStateTypeId, state);

        if(lookup_i2c_bus(device->paramValue(i2cInterfaceDeviceInterfaceParamTypeId).toByteArray()) == -1) {
            device->setStateValue(i2cInterfaceConnectedStateTypeId, false);
        } else {
            device->setStateValue(i2cInterfaceConnectedStateTypeId, true);
        }
    }
}

QList<i2c_adap> DevicePluginI2cTools::getI2cBusses(void)
{
    struct i2c_adap *adapters;
    int count;
    QList<i2c_adap> i2cBusses;

    adapters = gather_i2c_busses();
    if (adapters == nullptr) {
        qWarning(dcI2cTools()) << "Error: Out of memory!";
        return i2cBusses;
    }

    for (count = 0; adapters[count].name; count++) {
        qDebug(dcI2cTools()) << "i2c adapter nr:" << adapters[count].nr;
        i2cBusses.append(adapters[count]);
    }
    free_adapters(adapters);
    return i2cBusses;
}

QList<int> DevicePluginI2cTools::scanI2cBus(int file, ScanMode mode, unsigned long funcs, int first, int last)
{
    int i, j;
    int cmd, res;

    QList<int> deviceAddresses;

    qDebug(dcI2cTools()) << "Start scanning i2c bus";

    for (i = 0; i < 128; i += 16) {
        for(j = 0; j < 16; j++) {
            fflush(stdout);

            /* Select detection command for this address */
            switch (mode) {
            default:
                cmd = mode;
                break;
            case ScanModeAuto:
                if ((i+j >= 0x30 && i+j <= 0x37)
                        || (i+j >= 0x50 && i+j <= 0x5F))
                    cmd = ScanModeRead;
                else
                    cmd = ScanModeQuick;
                break;
            }

            /* Skip unwanted addresses */
            if (i+j < first || i+j > last
                    || (cmd == ScanModeRead &&
                        !(funcs & I2C_FUNC_SMBUS_READ_BYTE))
                    || (cmd == ScanModeQuick &&
                        !(funcs & I2C_FUNC_SMBUS_QUICK))) {
                continue;
            }

            /* Set slave address */
            if (ioctl(file, I2C_SLAVE, i+j) < 0) {
                if (errno == EBUSY) {
                    qDebug(dcI2cTools()) << "Device found but it seems busy <UU>";
                    continue;
                } else {
                    qWarning(dcI2cTools()) << "Error: Could not set address" << strerror(errno);
                    return deviceAddresses;
                }
            }

            /* Probe this address */
            switch (cmd) {
            default: /* MODE_QUICK */
                /* This is known to corrupt the Atmel AT24RF08
                   EEPROM */
                res = i2c_smbus_write_quick(file, I2C_SMBUS_WRITE);
                break;
            case ScanModeRead:
                /* This is known to lock SMBus on various
                   write-only chips (mainly clock chips) */
                res = i2c_smbus_read_byte(file);
                break;
            }

            if (res < 0) {
                //qDebug(dcI2cTools()) << "No device on this address";
            } else {
                deviceAddresses.append(i+j);
                qDebug(dcI2cTools()) << "Found device with address:" << i+j;
            }
        }
    }
    return deviceAddresses;
}
