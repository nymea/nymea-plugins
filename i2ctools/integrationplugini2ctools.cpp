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

#include "integrationplugini2ctools.h"
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

IntegrationPluginI2cTools::IntegrationPluginI2cTools()
{
}

void IntegrationPluginI2cTools::discoverThings(ThingDiscoveryInfo *info)
{
    ThingClassId ThingClassId = info->thingClassId();

    if (ThingClassId == i2cInterfaceThingClassId) {
        QList<QString> busses = getI2cBusses();
        foreach (QString adapter, busses) {
            ThingDescriptor descriptor(i2cInterfaceThingClassId, adapter, "");
            ParamList params;
            params.append(Param(i2cInterfaceThingInterfaceParamTypeId, adapter));
            descriptor.setParams(params);
            info->addThingDescriptor(descriptor);
        }
        info->finish(Thing::ThingErrorNoError);
        return;
    } else if (ThingClassId == i2cReadRegisterThingClassId) {

        foreach (QFile *file, m_i2cDeviceFiles) {
            ThingId parentThingId = m_i2cDeviceFiles.key(file);
            QString interfaceName = myThings().findById(parentThingId)->name();
            QList<int> addresses = scanI2cBus(file->handle(), ScanModeAuto, m_fileFuncs.value(file), 0, 254);
            foreach (int address, addresses) {
                ThingDescriptor descriptor(i2cReadRegisterThingClassId, QString("0x%0").arg(QString::number(address, 16)), "I2C interface: " + interfaceName, parentThingId);
                ParamList params;
                params.append(Param(i2cReadRegisterThingAddressParamTypeId, address));
                descriptor.setParams(params);
                info->addThingDescriptor(descriptor);
            }
        }
        info->finish(Thing::ThingErrorNoError);
        return;
    }
    qCWarning(dcI2cTools()) << "Discovery called for a deviceclass which does not support discovery? Device class ID:" << info->thingClassId().toString();
    info->finish(Thing::ThingErrorThingClassNotFound);
}


void IntegrationPluginI2cTools::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == i2cInterfaceThingClassId) {
        qCDebug(dcI2cTools) << "Setup i2c interface";
        unsigned long funcs;

        QFile *i2cFile = new QFile("/dev/" + thing->paramValue(i2cInterfaceThingInterfaceParamTypeId).toString());
        if (!i2cFile->exists()) {
            qCWarning(dcI2cTools()) << "Could not open I2C file";
            info->finish(Thing::ThingErrorSetupFailed);
            return;
        }

        if (!i2cFile->open(QFile::ReadWrite)) {
            qCWarning(dcI2cTools()) << "Could not open the given I2C file descriptor:" << i2cFile->fileName() << i2cFile->errorString();
            info->finish(Thing::ThingErrorSetupFailed);
            return;
        }

        if (ioctl(i2cFile->handle(), I2C_FUNCS, &funcs) < 0) {
            qCWarning(dcI2cTools()) << "Could not get the adapter functionality matrix" << strerror(errno);
            i2cFile->close();
            info->finish(Thing::ThingErrorSetupFailed);
            return ;
        }
        m_i2cDeviceFiles.insert(thing->id(), i2cFile);
        m_fileFuncs.insert(i2cFile, funcs);

        info->finish(Thing::ThingErrorNoError);
        return;

    } else if (thing->thingClassId() == i2cReadRegisterThingClassId) {
        qCDebug(dcI2cTools) << "Setup i2c device";
        info->finish(Thing::ThingErrorNoError);
        return;
    }
    return info->finish(Thing::ThingErrorThingNotFound);
}


void IntegrationPluginI2cTools::postSetupThing(Thing *thing)
{
    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(5 * 60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this] {
            foreach (Thing *thing, myThings().filterByThingClassId(i2cInterfaceThingClassId)) {
                QFile *i2cFile = m_i2cDeviceFiles.value(thing->id());
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
                thing->setStateValue(i2cInterfaceAvailableDevicesStateTypeId, state);


                if(!QFile::exists("/dev/"+(thing->paramValue(i2cInterfaceThingInterfaceParamTypeId).toString()))) {
                    thing->setStateValue(i2cInterfaceConnectedStateTypeId, false);
                } else {
                    thing->setStateValue(i2cInterfaceConnectedStateTypeId, true);
                }
            }
        });
    }

    if (thing->thingClassId() == i2cInterfaceThingClassId) {
        QFile *i2cFile = m_i2cDeviceFiles.value(thing->id());
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
        thing->setStateValue(i2cInterfaceAvailableDevicesStateTypeId, state);

        if(!QFile::exists("/dev/"+(thing->paramValue(i2cInterfaceThingInterfaceParamTypeId).toByteArray()))) {
            thing->setStateValue(i2cInterfaceConnectedStateTypeId, false);
        } else {
            thing->setStateValue(i2cInterfaceConnectedStateTypeId, true);
        }

    } else if (thing->thingClassId() == i2cReadRegisterThingClassId) {
        QFile *i2cFile = m_i2cDeviceFiles.value(thing->parentId());
        int slaveAddress = thing->paramValue(i2cReadRegisterThingAddressParamTypeId).toInt();
        uint8_t command = thing->paramValue(i2cReadRegisterThingRegisterParamTypeId).toUInt();
        if (ioctl(i2cFile->handle(), I2C_SLAVE, slaveAddress) < 0) {
            qCWarning(dcI2cTools()) << "Could not access slave" << slaveAddress;
            thing->setStateValue(i2cReadRegisterConnectedStateTypeId, false);
            return;
        }
        int value = i2c_smbus_read_word_data(i2cFile->handle(), command);
        thing->setStateValue(i2cReadRegisterConnectedStateTypeId, true);
        thing->setStateValue(i2cReadRegisterValueStateTypeId, value);
    }
}


void IntegrationPluginI2cTools::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == i2cInterfaceThingClassId) {
        QFile *i2cFile = m_i2cDeviceFiles.take(thing->id());
        m_fileFuncs.remove(i2cFile);
        i2cFile->close();
    }

    if (myThings().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}


void IntegrationPluginI2cTools::onPluginTimer()
{
    foreach (Thing *thing, myThings().filterByThingClassId(i2cInterfaceThingClassId)) {
        QFile *i2cFile = m_i2cDeviceFiles.value(thing->id());
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

        thing->setStateValue(i2cInterfaceAvailableDevicesStateTypeId, state);

        if(!QFile::exists("/dev/"+(thing->paramValue(i2cInterfaceThingInterfaceParamTypeId).toString()))) {
            thing->setStateValue(i2cInterfaceConnectedStateTypeId, false);
        } else {
            thing->setStateValue(i2cInterfaceConnectedStateTypeId, true);
        }
    }
}

QStringList IntegrationPluginI2cTools::getI2cBusses(void)
{
    return QDir("/sys/class/i2c-adapter/").entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
}

QList<int> IntegrationPluginI2cTools::scanI2cBus(int file, ScanMode mode, unsigned long funcs, int first, int last)
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
