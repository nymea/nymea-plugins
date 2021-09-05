/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
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

#include "sht30.h"
#include "extern-plugininfo.h"

#include <unistd.h>
#include <QThread>

SHT30::SHT30(const QString &portName, int address, QObject *parent) :
    I2CDevice(portName, address, parent)
{

}

QByteArray SHT30::readData(int fileDescriptor)
{
    // Send measurement command: 0x2C
    // High repeatability measurement: 0x06
    char config[2] = {0};
    config[0] = 0x2C;
    config[1] = 0x06;
    if (write(fileDescriptor, config, 2) != 2) {
        qCWarning(dcI2cDevices()) << "SHT30: could not configure sensor.";
        QThread::msleep(500);
        return QByteArray();
    }
    QThread::msleep(500);

    // Read 6 bytes of data
    // Temperature msb, Temperature lsb, Temperature CRC, Humididty msb, Humidity lsb, Humidity CRC
    char data[6] = {0};
    if (read(fileDescriptor, data, 6) != 6) {
        qCWarning(dcI2cDevices()) << "SHT30: could not read sensor values.";
        QThread::msleep(500);
        return QByteArray();
    }

    // Convert the data
    int temperatureRaw = (data[0] << 8) | data[1];
    double temperature = -45 + (175 * temperatureRaw / 65535.0);
    double humidity = 100 * ((data[3] << 8) | data[4]) / 65535.0;
    emit measurementAvailable(temperature, humidity);
    return QByteArray(data, 6);
}
