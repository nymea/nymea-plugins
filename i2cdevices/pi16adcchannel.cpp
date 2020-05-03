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

