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

#include <stdio.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "extern-plugininfo.h"
#include "piface.h"

PiFace::PiFace(int busAddress,QObject *parent) :
    QObject (parent),
    m_busAddress(busAddress)
{

}

void PiFace::setBusAddress(int busAddress)
{
    m_busAddress = busAddress;
}

int PiFace::openPort()
{
    // open
    QFile file("/dev/spidev0.0");

    if (!file.open(QIODevice::ReadWrite)) {
        qCWarning(dcPiFace()) << "Could not open SPI device";
        return -1;
    }

    // initialise
    const uint8_t spiMode = 0;
    if (ioctl(file.handle(), SPI_IOC_WR_MODE, &spiMode) < 0) {
        qCWarning(dcPiFace()) << "Could not set SPI mode";
        file.close();
        return -1;
    }
    const uint8_t spiBitsPerWord = 8;
    if (ioctl(file.handle(), SPI_IOC_WR_BITS_PER_WORD, &spiBitsPerWord) < 0) {
        qCWarning(dcPiFace()) << "Could not set SPI bits per word";
        file.close();
        return -1;
    }
    const uint32_t spiSpeed = 10000000;
    if (ioctl(file.handle(), SPI_IOC_WR_MAX_SPEED_HZ, &spiSpeed) < 0) {
        qCWarning(dcPiFace()) << "Could not set SPI speed";
        file.close();
        return -1;
    }
    return 0;
}

int PiFace::closePort()
{
    if (m_fd.isOpen()) {
        m_fd.close();
    }
    return -1;
}

quint8 PiFace::readRegister(quint8 registerAddress)
{
    quint8 controlByte =getControlByte(CommandRead);
    quint8 tx_buf[3] = {controlByte, registerAddress, 0};
    quint8 rx_buf[sizeof tx_buf];

    struct spi_ioc_transfer spi;
    memset (&spi, 0, sizeof(spi));
    spi.tx_buf = (unsigned long) tx_buf;
    spi.rx_buf = (unsigned long) rx_buf;
    spi.len = sizeof tx_buf;
    spi.delay_usecs = 0;
    spi.speed_hz = 10000000; // 10MHz
    spi.bits_per_word = 8;

    // do the SPI transaction
    if ((ioctl(m_fd.handle(), SPI_IOC_MESSAGE(1), &spi) < 0)) {
        qCWarning(dcPiFace()) << "Could not read register address:" << registerAddress;
        return 0;
    }

    // return the data
    return rx_buf[2];
}

quint8 PiFace::getControlByte(Command command)
{
    return (0x40 | ((m_busAddress << 1) & 0xE) | (command & 0x01));
}
