// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "owfs.h"
#include "owcapi.h"

#include "extern-plugininfo.h"

#include <QVariant>

Owfs::Owfs(QObject *parent) :
    QObject(parent)
{

}

Owfs::~Owfs()
{
    OW_finish();
}

bool Owfs::init(const QByteArray &owfsInitArguments)
{
    //QByteArray initArguments;
    //Test OWFS arguments
    //initArguments.append("--fake 28 --fake 10"); //fake temperature sensors
    //initArguments.append("--fake 29 --fake 12 --fake 05"); //fake temperature sensor

    //Test i2c
    //initArguments.append("--i2c=ALL:ALL");

    // W1 Kernel Module
    //inifArguments.append("--w1");

    if (OW_init(owfsInitArguments) < 0) {
        qCWarning(dcOneWire()) << "ERROR initialising one wire" << strerror(errno);
        return false;
    }
    m_path = "/";
    return true;
}

bool Owfs::discoverDevices()
{
    char *dirBuffer = nullptr;
    size_t dirLength ;

    if (OW_get(m_path, &dirBuffer, &dirLength) < 0) {
        qCWarning(dcOneWire()) << "DIRECTORY ERROR" << strerror(errno);
        return false;
    }
    qCDebug(dcOneWire()) << "Directory has members" << dirBuffer;

    QList<QByteArray> dirMembers ;
    dirMembers = QByteArray(dirBuffer, dirLength).split(',');
    free(dirBuffer);

    QList<OwfsDevice> owfsDevices;
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
            OwfsDevice thing;
            thing.family = family;
            thing.address = member;
            thing.id = member.split('.').last();
            thing.type = getValue(member, "type");
            owfsDevices.append(thing);
        }
    }
    emit devicesDiscovered(owfsDevices);
    return true;
}

bool Owfs::interfaceIsAvailable()
{
    return true;
    //TODO
    //QByteArray fullPath;
    //fullPath.append(m_path);
    //if(OW_present(fullPath) < 0)
    //   return false;
    //return true;
}

bool Owfs::isConnected(const QByteArray &address)
{
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
QByteArray Owfs::getValue(const QByteArray &address, const QByteArray &type)
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
        qCWarning(dcOneWire()) << "ERROR reading" << devicePath << strerror(errno);
    }

    qCDebug(dcOneWire()) << "Device value" << devicePath << getBuffer;

    QByteArray value = QByteArray(getBuffer, getLength);
    free(getBuffer);
    return value;
}

void Owfs::setValue(const QByteArray &address, const QByteArray &type, const QByteArray &value)
{
    Q_UNUSED(value)
    QByteArray devicePath;
    devicePath.append(m_path);
    if(!m_path.endsWith('/'))
        devicePath.append('/');
    devicePath.append(address);
    devicePath.append('/');
    devicePath.append(type);
    devicePath.append('\0');

    if (OW_put(devicePath, value, value.length()) < 0) {
        qCWarning(dcOneWire()) << "ERROR reading" << devicePath << strerror(errno);
    }
}

double Owfs::getTemperature(const QByteArray &address, bool *ok)
{
    QByteArray temperature = getValue(address, "temperature");
    qCDebug(dcOneWire()) << "Temperature" << temperature << temperature.replace(',','.').toDouble();
    return temperature.toDouble(ok);
}

double Owfs::getHumidity(const QByteArray &address, bool *ok)
{
    QByteArray humidity = getValue(address, "humidity");
    qCDebug(dcOneWire()) << "Humidity" << humidity << humidity.replace(',','.').toDouble();
    return humidity.toDouble(ok);
}

QByteArray Owfs::getType(const QByteArray &address)
{
    QByteArray type = getValue(address, "type");
    return type;
}

bool Owfs::getSwitchOutput(const QByteArray &address, SwitchChannel channel, bool *ok)
{
    QByteArray c;
    c.append("PIO.");
    switch (channel) {
    case PIO_A:
        c.append('A');
        break;
    case PIO_B:
        c.append('B');
        break;
    case PIO_C:
        c.append('C');
        break;
    case PIO_D:
        c.append('D');
        break;
    case PIO_E:
        c.append('E');
        break;
    case PIO_F:
        c.append('F');
        break;
    case PIO_G:
        c.append('G');
        break;
    case PIO_H:
        c.append('H');
        break;
    }
    QByteArray state = getValue(address, c);
    qCDebug(dcOneWire()) << "Switch state" << state.toInt();
    return state.toInt(ok);
}

bool Owfs::getSwitchInput(const QByteArray &address, SwitchChannel channel, bool *ok)
{
    QByteArray c;
    c.append("sensed.");
    switch (channel) {
    case PIO_A:
        c.append('A');
        break;
    case PIO_B:
        c.append('B');
        break;
    case PIO_C:
        c.append('C');
        break;
    case PIO_D:
        c.append('D');
        break;
    case PIO_E:
        c.append('E');
        break;
    case PIO_F:
        c.append('F');
        break;
    case PIO_G:
        c.append('G');
        break;
    case PIO_H:
        c.append('H');
        break;
    }
    QByteArray state = getValue(address, c);
    qCDebug(dcOneWire()) << "Switch state" << state.toInt();
    return state.toInt(ok);
}

void Owfs::setSwitchOutput(const QByteArray &address, SwitchChannel channel, bool state)
{
    QByteArray c;
    c.append("PIO.");
    switch (channel) {
    case PIO_A:
        c.append('A');
        break;
    case PIO_B:
        c.append('B');
        break;
    case PIO_C:
        c.append('C');
        break;
    case PIO_D:
        c.append('D');
        break;
    case PIO_E:
        c.append('E');
        break;
    case PIO_F:
        c.append('F');
        break;
    case PIO_G:
        c.append('G');
        break;
    case PIO_H:
        c.append('H');
        break;
    }
    setValue(address, c, QVariant(state).toByteArray());
}


