/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

#ifndef KODIJSONHANDLER_H
#define KODIJSONHANDLER_H

#include <QObject>
#include <QVariant>
#include <QHash>

#include "kodiconnection.h"
#include "kodireply.h"
#include "typeutils.h"

class KodiJsonHandler : public QObject
{
    Q_OBJECT
public:
    explicit KodiJsonHandler(KodiConnection *connection, QObject *parent = nullptr);

    int sendData(const QString &method, const QVariantMap &params);

signals:
    void notificationReceived(const QString &method, const QVariantMap &params);
    void replyReceived(int id, const QString &method, const QVariantMap &params);

private slots:
    void processResponse(const QByteArray &data);

private:
    KodiConnection *m_connection;
    int m_id;
    QHash<int, KodiReply> m_replys;

};

#endif // KODIJSONHANDLER_H
