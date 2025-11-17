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

#include "pi16adcchannel.h"
#include "extern-plugininfo.h"

#include <QDebug>

#include <unistd.h>

static QHash<int, char> channelMap = {
    {0, 0xB0},
    {1, 0xB8},
    {2, 0xB1},
    {3, 0xB9},
    {4, 0xB2},
    {5, 0xBA},
    {6, 0xB3},
    {7, 0xBB},
    {8, 0xB4},
    {9, 0xBC},
    {10, 0xB5},
    {11, 0xBD},
    {12, 0xB6},
    {13, 0xBE},
    {14, 0xB7},
    {15, 0xBF}
};

Pi16ADCChannel::Pi16ADCChannel(const QString &portName, int address, int channel, QObject *parent):
    I2CDevice(portName, address, parent),
    m_channel(channel)
{
}

QByteArray Pi16ADCChannel::readData(int fd)
{
    // The chip requires a minimum of 200ms of wating between each call or it might just ignore it.
    // Make sure to wait here long enough for any previous access having settled
    QThread::msleep(200);

    // Select the wanted channel
    qint64 len = write(fd, &channelMap[m_channel], 1);
    if (len != 1) {
        qCWarning(dcI2cDevices()) << "Pi-16ADC: Error writing channel config to device";
        return QByteArray();
    }

    // Wait for write the chip to settle
    QThread::msleep(200);

    // ready to read the value
    char readBuf[3] = {0};
    if (read(fd, readBuf, 3) != 3) {
        qCWarning(dcI2cDevices()) << "Pi-16ADC: could not read ADC data" << readBuf;
        return QByteArray();
    }

    return QByteArray(readBuf, 3);
}

