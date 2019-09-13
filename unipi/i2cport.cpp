/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "i2cport.h"
#include "i2cport_p.h"
#include "loggingcategories.h"

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

#include <QDir>

I2CPort::I2CPort(const QString &portName, QObject *parent) :
    QObject(parent),
    d_ptr(new I2CPortPrivate(this))
{
    d_ptr->portDeviceName = "/dev/" + portName;
    d_ptr->fileDescriptor.setFileName(d_ptr->portDeviceName);
}

QStringList I2CPort::availablePorts()
{
    return QDir("/sys/class/i2c-adapter/").entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
}

QList<int> I2CPort::scanRegirsters()
{
    return d_ptr->scanRegirsters();
}

int I2CPort::deviceDescriptor() const
{
    return d_ptr->deviceDescriptor;
}

int I2CPort::address() const
{
    return d_ptr->address;
}

QString I2CPort::portName() const
{
    return d_ptr->portName;
}

QString I2CPort::portDeviceName() const
{
    return d_ptr->portDeviceName;
}

bool I2CPort::isOpen() const
{
    return d_ptr->isOpen();
}

bool I2CPort::isValid() const
{
    return d_ptr->isValid();
}

bool I2CPort::openPort(int i2cAddress)
{
    return d_ptr->openPort(i2cAddress);
}

void I2CPort::closePort()
{
    return d_ptr->closePort();
}

I2CPortPrivate::I2CPortPrivate(I2CPort *q) :
    QObject(q),
    q_ptr(q)
{

}

QList<int> I2CPortPrivate::scanRegirsters()
{
    qCDebug(dcHardware()) << "Scanning I2C device" << portDeviceName;

    // Note: these methods are only available on arm since libi2c-dev package is different on amd64 and i386
    QList<int> addressList;
#ifdef __arm__
    for (int address = 0x3; address <= 0x77; address++) {
        // Check if ioctl possible
        if (ioctl(deviceDescriptor, I2C_SLAVE, address) >= 0) {
            i2c_smbus_write_byte(deviceDescriptor, 0x00);
            int id = i2c_smbus_read_byte(deviceDescriptor);
            if (id >= 0) {
                qCDebug(dcHardware()) << QString("   --> found address  = 0x%1").arg(address, 0, 16);
                addressList.append(address);
            }
        }
    }
#else
    qCWarning(dcHardware()) << "This hardware architecture does not support I2C.";
#endif

    return addressList;
}

bool I2CPortPrivate::isOpen() const
{
    return fileDescriptor.isOpen();
}

bool I2CPortPrivate::isValid() const
{
    return valid;
}

bool I2CPortPrivate::openPort(int i2cAddress)
{
    if (fileDescriptor.isOpen()) {
        qCWarning(dcHardware()) << "The given I2C file descriptor is already open:" << fileDescriptor.fileName();
        return false;
    }

    if (!fileDescriptor.exists()) {
        qCWarning(dcHardware()) << "The given I2C file descriptor does not exist:" << fileDescriptor.fileName();
        return false;
    }

    if (!fileDescriptor.open(QFile::ReadWrite)) {
        qCWarning(dcHardware()) << "Could not open the given I2C file descriptor:" << fileDescriptor.fileName();
        return false;
    }

    deviceDescriptor = fileDescriptor.handle();
//    if (ioctl(deviceDescriptor, I2C_SLAVE, i2cAddress) < 0) {
//        qCWarning(dcHardware()) << "Could not set I2C into slave mode" << portDeviceName << QString("0x%1").arg(i2cAddress, 0, 16);
//        closePort();
//        return false;
//    }

    address = i2cAddress;
    valid = true;
    return true;
}

void I2CPortPrivate::closePort()
{
    if (fileDescriptor.isOpen()) {
        fileDescriptor.close();
    }

    deviceDescriptor = -1;
    valid = false;
}
