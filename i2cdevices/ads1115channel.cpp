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

