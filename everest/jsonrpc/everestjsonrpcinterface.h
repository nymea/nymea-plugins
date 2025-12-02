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

#ifndef EVERESTJSONRPCINTERFACE_H
#define EVERESTJSONRPCINTERFACE_H

#include <QObject>
#include <QWebSocket>

class EverestJsonRpcInterface : public QObject
{
    Q_OBJECT
public:
    explicit EverestJsonRpcInterface(QObject *parent = nullptr);
    ~EverestJsonRpcInterface();

    QUrl serverUrl() const;

    void sendData(const QByteArray &data);

public slots:
    void connectServer(const QUrl &serverUrl);
    void disconnectServer();

signals:
    void connectedChanged(bool connected);
    void dataReceived(const QByteArray &data);

private slots:
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void onStateChanged(QAbstractSocket::SocketState state);
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message);

private:
    QWebSocket *m_webSocket = nullptr;
    QUrl m_serverUrl;
    bool m_connected = false;

};

#endif // EVERESTJSONRPCINTERFACE_H
