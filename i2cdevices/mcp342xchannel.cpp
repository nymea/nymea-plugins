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

#include "mcp342xchannel.h"
#include "extern-plugininfo.h"

#include <unistd.h>

#define GENERAL_CALL_RESET      0x0006
#define GENERAL_CALL_LATCH      0x0004
#define GENERAL_CALL_CONVERSION 0x0008

#define CONFIGURATION_REGISTER


MCP342XChannel::MCP342XChannel(const QString &portName, int address, int channel, Gain gain, QObject *parent) :
    I2CDevice(portName, address, parent),
    m_channel(channel),
    m_gain(gain)
{

}


QByteArray MCP342XChannel::readData(int fd)
{
    // Start a configuration conversation
    unsigned char writeBuf[1] = {0};
    writeBuf[0] |= (m_channel & 0x0003) << ConfRegisterBits::C0;
    writeBuf[0] |= m_gain << ConfRegisterBits::G0;
    //writeBuf[0] |= SampleRateSelectionBit::Bits18 << ConfRegisterBits::S0;
    writeBuf[0] |= 1 << ConfRegisterBits::OC;   // one shot
    writeBuf[0] |= 1 << ConfRegisterBits::RDY;  // start conversoin
    if (write(fd, writeBuf, 1) != 1) {
        qCWarning(dcI2cDevices()) << "MCP342X: could not write config register";
        return QByteArray();
    }

    // Wait for device to accept configuration (conversation bit is cleared)
    char readBuf[2] = {0};
    do {
        if (read(fd, readBuf, 2) != 2) {
            qCWarning(dcI2cDevices()) << "MCP342X: could not read ADC data";
            return QByteArray();
        }
    } while (!(readBuf[0] & (1 << ConfRegisterBits::RDY)));

    return QByteArray(readBuf, 2);
}

