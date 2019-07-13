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

#ifndef I2CPORT_P_H
#define I2CPORT_P_H

#include <QFile>
#include <QObject>
#include <QString>

#include "i2cport.h"

class I2CPortPrivate : public QObject
{
    Q_OBJECT
public:
    explicit I2CPortPrivate(I2CPort *q);
    I2CPort *q_ptr;

    QList<int> scanRegirsters();

    bool isOpen() const;
    bool isValid() const;

public slots:
    bool openPort(int address);
    void closePort();

public:
    QFile fileDescriptor;
    int deviceDescriptor = -1;
    int address;
    bool valid = false;
    QString portName;
    QString portDeviceName;


};

#endif // I2CPORT_P_H
