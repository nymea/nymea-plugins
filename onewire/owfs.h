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

#ifndef OWFS_H
#define OWFS_H

#include "owcapi.h"

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
    void devicesDiscovered(QList<OwfsDevice> devices);
};

#endif // OWFS_H
