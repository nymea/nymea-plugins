/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stuerz <simon.stuerz@nymea.io>                *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WALLBE_H
#define WALLBE_H

#include <QObject>
#include <QHostAddress>
#include <QProcess>

#include <modbus/modbus.h>

class WallBe : public QObject
{
    Q_OBJECT
public:

    WallBe(QHostAddress address, int port, QObject *parent = nullptr);
    ~WallBe();
    bool isAvailable();
    bool connect();

    int  getEvStatus();
    int  getChargingCurrent();
    bool getChargingStatus();
    int  getChargingTime();
    int  getErrorCode();

    void setChargingCurrent(int current);
    void setChargingStatus(bool enable);

private:
    modbus_t *m_device;
    QString m_macAddress;
    QString getMacAddress();
};

#endif // WALLBE_H
