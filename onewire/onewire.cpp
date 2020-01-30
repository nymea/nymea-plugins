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

#include "onewire.h"
#include "extern-plugininfo.h"

OneWire::OneWire(QObject *parent) :
    QObject(parent)
{

}

OneWire::~OneWire()
{
    OW_finish();
}

bool OneWire::init(const QByteArray &owfsInitArguments)
{
    //QByteArray initArguments;
    //Test OWFS arguments
    //initArguments.append("--fake 28 --fake 10"); //fake temperature sensors
    //initArguments.append("--fake 29 --fake 12 --fake 05"); //fake temperature sensor

    //Test i2c
    //initArguments.append("--i2c=ALL:ALL");

    if (OW_init(owfsInitArguments) < 0) {
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

void OneWire::setValue(const QByteArray &address, const QByteArray &type, const QByteArray &value)
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
        qWarning(dcOneWire()) << "ERROR reading" << devicePath << strerror(errno);
    }
}

double OneWire::getTemperature(const QByteArray &address)
{
    QByteArray temperature = getValue(address, "temperature");
    qDebug(dcOneWire()) << "Temperature" << temperature << temperature.replace(',','.').toDouble();
    return temperature.toDouble();
}

QByteArray OneWire::getType(const QByteArray &address)
{
    QByteArray type = getValue(address, "type");
    return type;
}

bool OneWire::getSwitchOutput(const QByteArray &address, SwitchChannel channel)
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
    qDebug(dcOneWire()) << "Switch state" << state.toInt();
    return state.toInt();
}

bool OneWire::getSwitchInput(const QByteArray &address, SwitchChannel channel)
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
    qDebug(dcOneWire()) << "Switch state" << state.toInt();
    return state.toInt();
}

void OneWire::setSwitchOutput(const QByteArray &address, SwitchChannel channel, bool state)
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


