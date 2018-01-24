/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017-2018 Simon Stürz <simon.stuerz@guh.io               *
 *                                                                         *
 *  This file is part of guh.                                              *
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

    SnapdReply *get(const QString &path, QObject *parent);
    SnapdReply *post(const QString &path, const QByteArray &payload, QObject *parent);

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
