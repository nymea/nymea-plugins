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

#ifndef KECONTACTDATALAYER_H
#define KECONTACTDATALAYER_H

#include <QObject>
#include <QUdpSocket>

class KeContactDataLayer : public QObject
{
    Q_OBJECT
public:
    explicit KeContactDataLayer(QObject *parent = nullptr);
    ~KeContactDataLayer();

    bool init();
    bool initialized() const;

    void write(const QHostAddress &address, const QByteArray &data);

private:
    bool m_initialized = false;
    int m_port = 7090;
    QUdpSocket *m_udpSocket = nullptr;

signals:
    void datagramReceived(const QHostAddress &address, const QByteArray &data);

private slots:
    void readPendingDatagrams();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);

};

#endif // KECONTACTDATALAYER_H
