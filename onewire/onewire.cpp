/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
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

#include "onewire.h"
#include "extern-plugininfo.h"

OneWire::OneWire(const QByteArray &deviceLocation, QObject *parent) :
    QObject(parent),
    m_deviceLocation(deviceLocation)
{

}

OneWire::~OneWire()
{
    OW_finish();
}

bool OneWire::init()
{
    QByteArray initArguments;
    //initArguments.append("--fake 28 --fake 10"); //fake temperature sensors
    //initArguments.append("--fake 29 --fake 12 --fake 05"); //fake temperature sensors
    initArguments.append("--i2c=ALL:ALL");
    if (OW_init(initArguments) < 0) {
        qWarning(dcOneWire()) << "ERROR initialising one wire" << strerror(errno);
        return false;
    }
    m_path = "/";
    return true;
}

bool OneWire::discoverDevices()
{
    char *dirBuffer = nullptr;
    size_t dirLength ;

    if (OW_get(m_path, &dirBuffer, &dirLength) < 0) {
        qWarning(dcOneWire()) << "DIRECTORY ERROR" << strerror(errno);
        return false;
    }
    qDebug(dcOneWire()) << "Directory has members" << dirBuffer;

    QList<QByteArray> dirMembers ;
    dirMembers = QByteArray(dirBuffer, dirLength).split(',');
    free(dirBuffer);

    QList<OneWireDevice> oneWireDevices;
    foreach(QByteArray member, dirMembers) {

        /* Other system members:
         * bus.0
         * uncached
         * settings
         * system
         * statistics
         * structure
         * simultaneous
         * alarm
         */

        int family = member.split('.').first().toInt(nullptr, 16);
        if (family != 0) {
            member.remove(member.indexOf('/'), 1);
            QByteArray type;
            OneWireDevice device;
            device.family = family;
            device.address = member;
            device.id = member.split('.').last();
            device.type = getValue(member, "type");
            oneWireDevices.append(device);
        }
    }
    if(!oneWireDevices.isEmpty()) {
        emit devicesDiscovered(oneWireDevices);
    }
    return true;
}

bool OneWire::interfaceIsAvailable()
{
    return true;
}

bool OneWire::isConnected(const QByteArray &address)
{
    Q_UNUSED(address)
    QByteArray fullPath;
    fullPath.append(m_path);
    fullPath.append(address);
    fullPath.append('\0');
    if(OW_present(fullPath) < 0)
        return false;
    return true;
}

/* Takes a path and filename and prints the 1-wire value */
/* makes sure the bridging "/" in the path is correct */
/* watches for total length and free allocated space */
QByteArray OneWire::getValue(const QByteArray &address, const QByteArray &type)
{
    char * getBuffer ;
    size_t getLength ;

    QByteArray devicePath;
    devicePath.append(m_path);
    if(!m_path.endsWith('/'))
        devicePath.append('/');
    devicePath.append(address);
    devicePath.append('/');
    devicePath.append(type);
    devicePath.append('\0');

    if (OW_get(devicePath, &getBuffer, &getLength) < 0) {
        qWarning(dcOneWire()) << "ERROR reading" << devicePath << strerror(errno);
    }

    qDebug(dcOneWire()) << "Device value" << devicePath << getBuffer;

    QByteArray value = QByteArray(getBuffer, getLength);
    free(getBuffer);
    return value;
}

void OneWire::setValue(const QByteArray &address, const QByteArray &deviceType, const QByteArray &value)
{
    Q_UNUSED(address)
    Q_UNUSED(deviceType)
    Q_UNUSED(value)
}

double OneWire::getTemperature(const QByteArray &address)
{
    QByteArray temperature = getValue(address, "temperature");
    qDebug(dcOneWire()) << "Temperature" << temperature << temperature.replace(',','.').toDouble();
    return temperature.toDouble();
}

QByteArray OneWire::readMemory(const QByteArray &address)
{
    //getValue
    return address; //TODDO
}

QByteArray OneWire::getType(const QByteArray &address)
{
    QByteArray type = getValue(address, "type");
    return type;
}

bool OneWire::getSwitchState(const QByteArray &address)
{
    QByteArray state = getValue(address, "switch_state");
    qDebug(dcOneWire()) << "Switch state" << state;
    return 0; //TODO
}

void OneWire::setSwitchState(const QByteArray &address, bool state)
{
    if (state) {
        setValue(address, "switch_state", "TRUE");
    } else {
        setValue(address, "switch_state", "FALSE");
    }
}


