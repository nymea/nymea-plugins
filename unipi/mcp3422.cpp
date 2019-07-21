/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernbard Trinnes <bernhard.trinnes@nymea.io         *
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

#include "mcp3422.h"
#include "i2cport.h"
#include "extern-plugininfo.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/i2c-dev.h>

#include <QFile>

MCP3422::MCP3422(const QString &i2cPortName, int i2cAddress, QObject *parent) :
    QThread(parent),
    m_i2cPortName(i2cPortName),
    m_i2cAddress(i2cAddress)
{

}

MCP3422::~MCP3422()
{
    disable();
    wait();
}

double MCP3422::getChannelVoltage(MCP3422::Channel channel)
{
    return static_cast<double>(getChannelValue(channel)) * 4.096 / 32767.0;
}

int MCP3422::getChannelValue(MCP3422::Channel channel)
{
    QMutexLocker locker(&m_valueMutex);
    int value = 0;
    switch (channel) {
    case Channel1:
        value = m_channel1Value;
        break;
    case Channel2:
        value =  m_channel2Value;
        break;
    }
    return value;
}

void MCP3422::run()
{
    qCDebug(dcUniPi()) << "MCP3422: initialize I2C port" << m_i2cPortName << QString("0x%1").arg(m_i2cAddress, 0, 16);

    QFile i2cFile("/dev/" + m_i2cPortName);
    if (!i2cFile.exists()) {
        qCWarning(dcUniPi()) << "MCP3422: The given I2C file descriptor does not exist:" << i2cFile.fileName();
        return;
    }

    if (!i2cFile.open(QFile::ReadWrite)) {
        qCWarning(dcUniPi()) << "MCP3422: Could not open the given I2C file descriptor:" << i2cFile.fileName() << i2cFile.errorString();
        return;
    }

    int fileDescriptor = i2cFile.handle();

    // Continuouse reading of the ADC values
    qCDebug(dcUniPi()) << "MCP3422: start reading values..." << this << "Process PID:" << syscall(SYS_gettid);
    while (true) {
        if (ioctl(fileDescriptor, I2C_SLAVE, m_i2cAddress) < 0) {
            qCWarning(dcUniPi()) << "MCP3422: Could not set I2C into slave mode" << m_i2cPortName << QString("0x%1").arg(m_i2cAddress, 0, 16);
            msleep(500);
            continue;
        }
        m_channel1Value = readInputValue(fileDescriptor, Channel1);
        m_channel2Value = readInputValue(fileDescriptor, Channel2);

        QMutexLocker valueLocker(&m_valueMutex);

        QMutexLocker stopLocker(&m_stopMutex);
        if (m_stop) break;
        msleep(500);
    }
    i2cFile.close();
    qCDebug(dcUniPi()) << "MCP3422: Reading thread finished.";
}

int MCP3422::readInputValue(int fd, MCP3422::Channel channel)
{
    unsigned char writeBuf;
    switch (channel) {
    case Channel1:
        writeBuf = 0xC3; // 0b11000011
        // Config register
        //bit 7 RDY: Ready Bit
        //bit 6-5 C1-C0: Channel Selection Bits
        //bit 4 O/C: Conversion Mode Bit
        //bit 3-2 S1-S0: Sample Rate Selection Bit
        //bit 1-0 G1-G0: PGA Gain Selection Bits

        break;
    case Channel2:
        writeBuf = 0xD3; // 0b11010011
        break;
    }
    write(fd, &writeBuf, 1);

    // Wait for conversion complete
    unsigned char readBuf[2] = {0};
    do {
        if (read(fd, readBuf, 2) != 2) {
            qCWarning(dcUniPi()) << "MCP3422: could not read ADC data";
            return 0;
        }
    } while (!(readBuf[0] & 0x80));

    // Write conversion register
    readBuf[0] = 0;
    if (write(fd, readBuf, 1) != 1) {
        qCWarning(dcUniPi()) << "MCP3422: could not write select register";
        return 0;
    }

    // Read value data
    if (read(fd, readBuf, 2) != 2) {
        qCWarning(dcUniPi()) << "MCP3422: could not read ADC data";
        return 0;
    }

    int value = static_cast<qint16>(readBuf[0]) * 256 + static_cast<qint16>(readBuf[1]);
    return value;
}


bool MCP3422::enable()
{
    // Check if this address can be opened
    I2CPort port(m_i2cPortName);
    if (!port.openPort(m_i2cAddress)) {
        qCWarning(dcUniPi()) << "MCP3422 is not available on port" << port.portDeviceName() << QString("0x%1").arg(m_i2cAddress, 0, 16);
        return false;
    }
    port.closePort();

    // Start the reading thread
    QMutexLocker locker(&m_stopMutex);
    m_stop = false;
    start();
    return true;
}

void MCP3422::disable()
{
    // Stop the thread if not already disabled
    QMutexLocker locker(&m_stopMutex);
    if (m_stop)
        return;

    qCDebug(dcUniPi()) << "MCP3422: Disable measurements";
    m_stop = true;
}
