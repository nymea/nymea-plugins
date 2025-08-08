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

#ifndef SIGNALRCONNECTION_H
#define SIGNALRCONNECTION_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QWebSocket>
#include <QTimer>
#include <network/networkaccessmanager.h>

class SignalRConnection : public QObject
{
    Q_OBJECT
public:
    explicit SignalRConnection(const QUrl &url, const QByteArray &accessToken, NetworkAccessManager *nam, QObject *parent = nullptr);

    void subscribe(const QString &chargerId);
    bool connected() const;

    void updateToken(const QByteArray &accessToken);

signals:
    void connectionStateChanged(bool connected);
    void dataReceived(const QVariantMap &data);

private:
    QByteArray encode(const QVariantMap &message);

private slots:
    void connectToHost();

private:
    QUrl m_url;
    QByteArray m_accessToken;
    NetworkAccessManager *m_nam = nullptr;
    QWebSocket *m_socket = nullptr;
    QTimer *m_watchdog = nullptr;

    bool m_waitingForHandshakeReply = false;
};

#endif // SIGNALRCONNECTION_H
