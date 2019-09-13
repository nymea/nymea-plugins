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

extern "C" {
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
}

MCP23008::MCP23008(const QString &i2cPortName, int i2cAddress, QObject *parent) :
    QObject(parent),
    m_i2cPortName(i2cPortName),
    m_i2cAddress(i2cAddress)
{

}

MCP23008::~MCP23008()
{
    m_i2cFile.close();
}

bool MCP23008::init()
{
    qCDebug(dcUniPi()) << "MCP23008: initialize I2C port" << m_i2cPortName << QString("0x%1").arg(m_i2cAddress, 0, 16);

    m_i2cFile.setFileName("/dev/" + m_i2cPortName);
    if (!m_i2cFile.exists()) {
        qCWarning(dcUniPi()) << "MCP23008: The given I2C file descriptor does not exist:" << m_i2cFile.fileName();
        return false;
    }

    if (!m_i2cFile.open(QFile::ReadWrite)) {
        qCWarning(dcUniPi()) << "MCP23008: Could not open the given I2C file descriptor:" << m_i2cFile.fileName() << m_i2cFile.errorString();
        return false;
    }
    m_fileDescriptor = m_i2cFile.handle();
    return true;

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


bool MCP23008::readRegister(RegisterAddress registerAddress, uint8_t *value)
{
#ifdef __arm__
    if (ioctl(m_fileDescriptor, I2C_SLAVE, m_i2cAddress) < 0) {
        qCWarning(dcUniPi()) << "MCP23008: Could not set I2C into slave mode" << m_i2cPortName << QString("0x%1").arg(m_i2cAddress, 0, 16);
        return false;
    }

    *value = i2c_smbus_read_byte_data(m_fileDescriptor, registerAddress);
    return true;
#else
    Q_UNUSED(registerAddress)
    Q_UNUSED(value)
    return false;
#endif // __arm__
}
