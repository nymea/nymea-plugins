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

#ifndef TVEVENTHANDLER_H
#define TVEVENTHANDLER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QDateTime>
#include <QTextStream>
#include <QRegExp>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

class TvEventHandler : public QTcpServer
{
    Q_OBJECT
public:
    explicit TvEventHandler(const QHostAddress &host, quint16 eventHandlerPort = 9000, QObject *parent = 0);

    static uint getFreePort();

protected:
    void incomingConnection(qintptr socket) override;

private:
    QHostAddress m_host;
    quint16 m_port;
    bool m_expectingData;

signals:
    void eventOccured(const QByteArray &path);

private slots:
    void readClient();
    void onDisconnected();

};

#endif // TVEVENTHANDLER_H
