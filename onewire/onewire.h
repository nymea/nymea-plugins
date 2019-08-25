/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
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

#ifndef ONEWIRE_H
#define ONEWIRE_H

#include "owcapi.h"

#include <QObject>

class OneWire : public QObject
{
    Q_OBJECT
public:
    enum OneWireProperty {
        Address,    //The entire 64-bit unique ID
        Crc,        //The 8-bit error correction
        Family,     //The 8-bit family code
        Id,         //The 48-bit middle portion of the unique ID number.
        Locator,    //Uses an extension of the 1-wire design from iButtonLink company that associated 1-wire physical connections with a unique 1-wire code.
        Type        //Part name assigned by Dallas Semi. E.g. DS2401
    };

    enum SwitchChannel {
        PIO_A,
        PIO_B,
        PIO_C,
        PIO_D,
        PIO_E,
        PIO_F,
        PIO_G,
        PIO_H
    };

    struct OneWireDevice {
        QByteArray address;
        int family;
        QByteArray id;
        QByteArray type;
    };

    explicit OneWire(QObject *parent = nullptr);
    ~OneWire();
    bool init(const QByteArray &owfsInitArguments);

    QByteArray getPath();
    bool discoverDevices();
    bool interfaceIsAvailable();
    bool isConnected(const QByteArray &address);

    double getTemperature(const QByteArray &address);
    QByteArray getType(const QByteArray &address);
    bool getSwitchOutput(const QByteArray &address, SwitchChannel channel);
    void setSwitchOutput(const QByteArray &address, SwitchChannel channel, bool state);
    bool getSwitchInput(const QByteArray &address, SwitchChannel channel);

private:
    QByteArray m_path;
    QByteArray getValue(const QByteArray &address, const QByteArray &deviceType);
    void setValue(const QByteArray &address, const QByteArray &deviceType, const QByteArray &value);

signals:
    void devicesDiscovered(QList<OneWireDevice> devices);
};

#endif // ONEWIRE_H
