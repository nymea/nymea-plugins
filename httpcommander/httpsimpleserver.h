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

#ifndef HTTPSIMPLESERVER1_H
#define HTTPSIMPLESERVER1_H

#include <QTcpServer>
#include <QUuid>
#include <QDateTime>
#include <QUrl>

#include <typeutils.h>

class HttpSimpleServer : public QTcpServer
{
    Q_OBJECT
public:

    HttpSimpleServer(quint16 port, QObject* parent = nullptr);
    ~HttpSimpleServer() override;

    void incomingConnection(qintptr socket) override;

signals:
    void disappear();
    void reconfigureAutodevice();
    //void getRequestReceied(QUrl url);

    void requestReceived(const QString &type, const QString &path, const QString &body);

private slots:
    void readClient();
    void discardClient();

private:
    QString generateHeader();

};

#endif // HttpSimpleServer_H
