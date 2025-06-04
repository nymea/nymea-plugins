/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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
