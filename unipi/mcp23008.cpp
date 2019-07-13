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

#include "mcp23008.h"
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

MCP23008::MCP23008(const QString &i2cPortName, int i2cAddress, QObject *parent) :
    QThread(parent),
    m_i2cPortName(i2cPortName),
    m_i2cAddress(i2cAddress)
{

}

MCP23008::~MCP23008()
{
    disable();
    wait();
}

void MCP23008::run()
{
    qCDebug(dcUniPi()) << "MCP23008: initialize I2C port" << m_i2cPortName << QString("0x%1").arg(m_i2cAddress, 0, 16);

    QFile i2cFile("/dev/" + m_i2cPortName);
    if (!i2cFile.exists()) {
        qCWarning(dcUniPi()) << "MCP23008: The given I2C file descriptor does not exist:" << i2cFile.fileName();
        return;
    }

    if (!i2cFile.open(QFile::ReadWrite)) {
        qCWarning(dcUniPi()) << "MCP23008: Could not open the given I2C file descriptor:" << i2cFile.fileName() << i2cFile.errorString();
        return;
    }

    int m_fileDescriptor = i2cFile.handle();

    // Continuouse reading of the ADC values
    qCDebug(dcUniPi()) << "MCP23008: start reading values..." << this << "Process PID:" << syscall(SYS_gettid);
    while (true) {
        if (ioctl(m_fileDescriptor, I2C_SLAVE, m_i2cAddress) < 0) {
            qCWarning(dcUniPi()) << "MCP23008: Could not set I2C into slave mode" << m_i2cPortName << QString("0x%1").arg(m_i2cAddress, 0, 16);
            msleep(500);
            continue;
        }

        QMutexLocker valueLocker(&m_valueMutex);

        QMutexLocker stopLocker(&m_stopMutex);
        if (m_stop) break;
        msleep(500);
    }

    i2cFile.close();
    qCDebug(dcUniPi()) << "MCP23008: Reading thread finished.";
}

bool MCP23008::writeRegister(MCP23008::RegisterAddress registerAddress, uint8_t value)
{
#ifdef __arm__
    if (ioctl(m_fileDescriptor, I2C_SLAVE, m_i2cAddress) < 0) {
        qCWarning(dcUniPi()) << "MCP23008: Could not set I2C into slave mode" << m_i2cPortName << QString("0x%1").arg(m_i2cAddress, 0, 16);
        return false;
    }

    // Write command
    int length = i2c_smbus_write_byte_data(m_fileDescriptor, registerAddress, value);
    if (length < 0) {
        qCWarning(dcUniPi()) << "MCP23008: Could not sent command to I2C bus.";
        return false;
    }
    return true;
#else
    Q_UNUSED(value)
    Q_UNUSED(registerAddress)
    return false;
#endif // __arm__
}


bool MCP23008::readRegister(RegisterAddress registerAddress)
{
    #ifdef __arm__
    if (ioctl(m_fileDescriptor, I2C_SLAVE, m_i2cAddress) < 0) {
        qCWarning(dcUniPi()) << "MCP23008: Could not set I2C into slave mode" << m_i2cPortName << QString("0x%1").arg(m_i2cAddress, 0, 16);
        return false;
    }

    quint8 registerValue = i2c_smbus_read_byte_data  (m_fileDescriptor, registerAddress);
    Q_UNUSED(registerValue)
    return true;
#else
    Q_UNUSED(registerAddress)
    return false;
#endif // __arm__
}

bool MCP23008::enable()
{
    // Check if this address can be opened
    I2CPort port(m_i2cPortName);
    if (!port.openPort(m_i2cAddress)) {
        qCWarning(dcUniPi()) << "MCP23008 is not available on port" << port.portDeviceName() << QString("0x%1").arg(m_i2cAddress, 0, 16);
        return false;
    }
    port.closePort();

    // Start the reading thread
    QMutexLocker locker(&m_stopMutex);
    m_stop = false;
    start();
    return true;
}

void MCP23008::disable()
{
    // Stop the thread if not already disabled
    QMutexLocker locker(&m_stopMutex);
    if (m_stop)
        return;

    qCDebug(dcUniPi()) << "MCP23008: Disable measurements";
    m_stop = true;
}
