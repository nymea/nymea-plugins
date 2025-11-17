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

#include "ads1115channel.h"
#include "extern-plugininfo.h"

#include <unistd.h>

#define REGISTER_CONVERSATION 0x00
#define REGISTER_CONFIG     0x01

#define OPERATIONAL_STATE_IDLE         0x00
#define OPERATIONAL_STATE_CONVERSATION 0x80

#define CONVERSION_MODE_CONTINUOUS 0x00
#define CONVERSION_MODE_SINGLE     0x40

#define CHANNEL_MODE_DIFFERENTIAL 0x00
#define CHANNEL_MODE_SINGLE       0x01

ADS1115Channel::ADS1115Channel(const QString &portName, int address, int channel, Gain gain, QObject *parent):
    I2CDevice(portName, address, parent),
    m_channel(channel),
    m_gain(gain)
{

}


QByteArray ADS1115Channel::readData(int fd)
{
    // Start a configuration conversation
    unsigned char writeBuf[3] = {0};
    writeBuf[0] = REGISTER_CONFIG; // Select config register
    writeBuf[1] |= OPERATIONAL_STATE_CONVERSATION; // Set conversation bit
    writeBuf[1] |= CHANNEL_MODE_SINGLE;
    writeBuf[1] |= m_channel << 4;
    writeBuf[1] |= m_gain << 1;
    writeBuf[1] |= CONVERSION_MODE_SINGLE;
    writeBuf[2] = 0x85; // 0b10000101: Data rate: 100, not using comparator
    if (write(fd, writeBuf, 3) != 3) {
        qCWarning(dcI2cDevices()) << "ADS1115: could not write config register";
        return QByteArray();
    }

    // Wait for device to accept configuration (conversation bit is cleared)
    char readBuf[2] = {0};
    do {
        if (read(fd, readBuf, 2) != 2) {
            qCWarning(dcI2cDevices()) << "ADS1115: could not read ADC data";
            return QByteArray();
        }
    } while (!(readBuf[0] & OPERATIONAL_STATE_CONVERSATION));

    // Select conversation register
    writeBuf[0] = REGISTER_CONVERSATION;
    if (write(fd, writeBuf, 1) != 1) {
        qCWarning(dcI2cDevices()) << "ADS1115: could not write select register";
        return QByteArray();
    }

    // Read conversation register
    if (read(fd, readBuf, 2) != 2) {
        qCWarning(dcI2cDevices()) << "ADS1115: could not read ADC data";
        return QByteArray();
    }

    return QByteArray(readBuf, 2);
}

