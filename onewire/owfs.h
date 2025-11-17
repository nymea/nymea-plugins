// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef OWFS_H
#define OWFS_H

#include <QObject>

class Owfs : public QObject
{
    Q_OBJECT
public:
    enum OwfsProperty {
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

    struct OwfsDevice {
        QByteArray address;
        int family;
        QByteArray id;
        QByteArray type;
    };

    explicit Owfs(QObject *parent = nullptr);
    ~Owfs();
    bool init(const QByteArray &owfsInitArguments);

    QByteArray getPath();
    bool discoverDevices();
    bool interfaceIsAvailable();
    bool isConnected(const QByteArray &address);

    double getTemperature( const QByteArray &address, bool *ok);
    double getHumidity(const QByteArray &address, bool *ok);
    QByteArray getType(const QByteArray &address);
    bool getSwitchOutput(const QByteArray &address, SwitchChannel channel, bool *ok);
    void setSwitchOutput(const QByteArray &address, SwitchChannel channel, bool state);
    bool getSwitchInput(const QByteArray &address, SwitchChannel channel, bool *ok);

private:
    QByteArray m_path;
    QByteArray getValue(const QByteArray &address, const QByteArray &deviceType);
    void setValue(const QByteArray &address, const QByteArray &deviceType, const QByteArray &value);

signals:
    void devicesDiscovered(QList<Owfs::OwfsDevice> devices);
};

#endif // OWFS_H
