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

#ifndef SNAPDCONNECTION_H
#define SNAPDCONNECTION_H

#include <QQueue>
#include <QObject>
#include <QLocalSocket>

#include "snapdreply.h"

class SnapdConnection : public QLocalSocket
{
    Q_OBJECT
public:
    explicit SnapdConnection(QObject *parent = nullptr);
    ~SnapdConnection();

    SnapdReply *get(const QString &path, QObject *parent);
    SnapdReply *post(const QString &path, const QByteArray &payload, QObject *parent);
    SnapdReply *put(const QString &path, const QByteArray &payload, QObject *parent);

    bool isConnected() const;

private:
    bool m_chuncked = false;

    QByteArray m_header;
    QByteArray m_payload;

    bool m_connected = false;
    bool m_debug = false;

    SnapdReply *m_currentReply = nullptr;
    QQueue<SnapdReply *> m_replyQueue;

    void setConnected(const bool &connected);

    // Helper methods
    QByteArray createRequestHeader(const QString &method, const QString &path, const QByteArray &payload = QByteArray());
    QByteArray getChunckedPayload(const QByteArray &payload);

    void processData();
    void sendNextRequest();

signals:
    void connectedChanged(const bool &connected);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(const QLocalSocket::LocalSocketError &socketError);
    void onStateChanged(const QLocalSocket::LocalSocketState &state);
    void onReadyRead();

};

#endif // SNAPDCONNECTION_H
