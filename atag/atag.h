/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#ifndef ATAG_H
#define ATAG_H

#include "network/networkaccessmanager.h"
#include "devices/device.h"

#include <QObject>
#include <QTimer>
#include <QHostAddress>

class Atag : public QObject
{
    Q_OBJECT
public:

    enum InfoCategory {
        Control       = 1,
        Schedules     = 2,
        Configuration = 4,
        Report        = 8,
        Status        = 16,
        WifiScan      = 32
    };

    enum AuthResult {
        NA = 0,
        Pending = 1,
        Granted = 2,
        Denied  = 3
    };

    explicit Atag(NetworkAccessManager *networkManager, const QHostAddress &address, int port, const QString &macAddress, QObject *parent = nullptr);
    void requestAuthorization();
    void getInfo(InfoCategory infoCategory);

private:
    NetworkAccessManager *m_networkManager = nullptr;
    QHostAddress m_address;
    int m_port;
    QString m_macAddress;
    QString m_accessToken;

signals:
    void connectionChanged(bool connected);
    void authenticationStatusChanged(bool authenticated);
    void pairMessageReceived(AuthResult result);
    void infoReportReceived();

private slots:
    void onRefreshTimer();

};

#endif // ATAG_H
