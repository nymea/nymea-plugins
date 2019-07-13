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

#ifndef I2CPORT_H
#define I2CPORT_H

#include <QObject>

class I2CPortPrivate;

class I2CPort : public QObject
{
    Q_OBJECT
public:
    explicit I2CPort(const QString &portName, QObject *parent = nullptr);

    static QStringList availablePorts();

    QList<int> scanRegirsters();

    int deviceDescriptor() const;
    int address() const;
    QString portName() const;
    QString portDeviceName() const;

    bool isOpen() const;
    bool isValid() const;

public slots:
    bool openPort(int i2cAddress = 0);
    void closePort();

private:
    I2CPortPrivate *d_ptr = nullptr;

};

#endif // I2CPORT_H
