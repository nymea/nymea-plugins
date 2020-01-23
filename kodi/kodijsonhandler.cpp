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

#include "kodijsonhandler.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>

KodiJsonHandler::KodiJsonHandler(KodiConnection *connection, QObject *parent) :
    QObject(parent),
    m_connection(connection),
    m_id(0)
{
    connect(m_connection, &KodiConnection::dataReady, this, &KodiJsonHandler::processResponse);
}

int KodiJsonHandler::sendData(const QString &method, const QVariantMap &params)
{
    m_id++;

    QVariantMap package;
    package.insert("id", m_id);
    package.insert("method", method);
    package.insert("params", params);
    package.insert("jsonrpc", "2.0");

    m_replys.insert(m_id, KodiReply(method, params));

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(package);
    m_connection->sendData(jsonDoc.toJson());
    //qCDebug(dcKodi) << "sending data" << jsonDoc.toJson();
    return m_id;
}

void KodiJsonHandler::processResponse(const QByteArray &data)
{
    m_dataBuffer.append(data);

    QByteArray packet = m_dataBuffer;
    int pos = packet.indexOf("}{");
    if (pos > 0) {
        packet = m_dataBuffer.left(pos + 1);
    }

    if (!packet.endsWith("}")) {
        return; // Won't parse for sure, likely not enough data yet
    }

    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(packet, &error);

    if(error.error != QJsonParseError::NoError) {
        qCWarning(dcKodi) << "failed to parse JSON data:" << data << ":" << error.errorString();
        return;
    }

    // Ok, we managed to parse a complete packet, remove it from the input buffer
    m_dataBuffer.remove(0, packet.length());

    //qCDebug(dcKodi) << "data received:" << jsonDoc.toJson();

    QVariantMap message = jsonDoc.toVariant().toMap();

    // check jsonrpc value
    if (!message.contains("jsonrpc") || message.value("jsonrpc").toString() != "2.0") {
        qCWarning(dcKodi) << "jsonrpc 2.0 value missing in message" << data;
    }

    // check id (if there is no id, it's an notification from kodi)
    if (!message.contains("id")) {

        // check method
        if (!message.contains("method")) {
            qCWarning(dcKodi) << "method missing in message" << data;
        }

        emit notificationReceived(message.value("method").toString(), message.value("params").toMap());
        return;
    }

    int id = message.value("id").toInt();
    KodiReply reply = m_replys.take(id);

    emit replyReceived(id, reply.method(), message);
}
