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

#ifndef WEBOSCONNECTION_H
#define WEBOSCONNECTION_H

#include <QObject>
#include <QWebSocket>

class WebosConnection : public QObject
{
    Q_OBJECT
public:
    explicit WebosConnection(QObject *parent = nullptr);

    QHostAddress hostAddress() const;
    void setHostAddress(const QHostAddress &hostAddress);

    QString apiKey() const;
    void setApiKey(const QString &apiKey);

    bool connected() const;

private:
    QHostAddress m_hostAddress;
    QWebSocket *m_websocket = nullptr;

    QString m_apiKey;
    int m_id = 0;

    void sendRequest(const QVariantMap &request);
    void getVolume();

signals:
    void connectedChanged(bool connected);

private slots:
    void onConnected();
    void onDisconnected();
    void onStateChanged(const QAbstractSocket::SocketState &state);
    void onTextMessageReceived(const QString &message);

public slots:
    void registerClient();
    void connectTv();


};

#endif // WEBOSCONNECTION_H
