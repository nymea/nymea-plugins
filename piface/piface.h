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

#ifndef PIFACE_H
#define PIFACE_H

#include <QObject>
#include <QFile>

class PiFace : public QObject
{
    Q_OBJECT
public:
    explicit PiFace(int busAddress = 0, QObject *parent = nullptr);

    void setBusAddress(int busAddress);
    int openPort();
    int closePort();

private:
    enum Command {
        CommandWrite = 0,
        CommandRead  = 1
    };

    enum RegisterAddress {
        IODIRA   = 0x00,
        IODIRB   = 0x01,
        IPOLA    = 0x02,
        IPOLB    = 0x03,
        GPINTENA = 0x04,
        GPINTENB = 0x05,
        DEFVALA  = 0x06,
        DEFVALB  = 0x07,
        INTCONA  = 0x08,
        INTCONB  = 0x09,
        IOCON    = 0x0A,
        GPPUA    = 0x0C,
        GPPUB    = 0x0D,
        INTFA    = 0x0E,
        INTFB    = 0x0F,
        INTCAPA  = 0x10,
        INTCAPB  = 0x11,
        GPIOA    = 0x12,
        GPIOB    = 0x13,
        OLATA    = 0x14,
        OLATB    = 0x15
    };

    int m_busAddress;
    QFile m_fd;
    quint8 getControlByte(Command command);
    quint8 readRegister(quint8 registerAddress);
};

#endif // PIFACE_H
