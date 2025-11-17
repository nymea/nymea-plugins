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

#include "shellyjsonrpcclient.h"

#include <QJsonDocument>
#include <QTimer>
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(dcShelly)

ShellyRpcReply::ShellyRpcReply(const QVariantMap &requestBody, QObject *parent):
    QObject(parent),
    m_requestBody(requestBody)
{
    QTimer::singleShot(10000, this, [this]{finished(StatusTimeout, QVariantMap());});
    connect(this, &ShellyRpcReply::finished, this, &ShellyRpcReply::deleteLater);
}

int ShellyRpcReply::id() const
{
    return m_requestBody.value("id").toInt();
}

QVariantMap ShellyRpcReply::requestBody() const
{
    return m_requestBody;
}

ShellyJsonRpcClient::ShellyJsonRpcClient(QObject *parent)
    : QObject(parent)
{
    m_socket = new QWebSocket("nymea", QWebSocketProtocol::VersionLatest, this);
    connect(m_socket, &QWebSocket::stateChanged, this, &ShellyJsonRpcClient::stateChanged);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &ShellyJsonRpcClient::onTextMessageReceived);
}

void ShellyJsonRpcClient::open(const QHostAddress &address, const QString &user, const QString &password, const QString &shellyId)
{
    m_password = password;
    m_user = user;
    m_shellyId = shellyId;

    QUrl url;
    url.setScheme("ws");
    url.setHost(address.toString());
    url.setPath("/rpc");
    m_socket->open(url);
}

ShellyRpcReply *ShellyJsonRpcClient::sendRequest(const QString &method, const QVariantMap &params)
{
    int id = m_currentId++;

    QVariantMap data;
    data.insert("id", id);
    data.insert("src", "nymea");
    data.insert("method", method);
    if (!params.isEmpty()) {
        data.insert("params", params);
    }

    if (!m_password.isEmpty() && m_nonce != 0) {
        data.insert("auth", createAuthMap());
    }

    ShellyRpcReply *reply = new ShellyRpcReply(data, this);
    connect(reply, &ShellyRpcReply::finished, this, [this, id]{
        m_pendingReplies.remove(id);
    });
    m_pendingReplies.insert(id, reply);

    qCDebug(dcShelly) << "Sending request" << qUtf8Printable(QJsonDocument::fromVariant(data).toJson());
    m_socket->sendTextMessage(QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact));

    return reply;
}

void ShellyJsonRpcClient::onTextMessageReceived(const QString &message)
{
    qCDebug(dcShelly) << "Text message received from shelly:" << message;

    QJsonParseError error;
    QVariantMap data = QJsonDocument::fromJson(message.toUtf8(), &error).toVariant().toMap();
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcShelly()) << "Error parsing data from shelly";
        m_socket->close(QWebSocketProtocol::CloseCodeBadOperation);
        return;
    }

    if (data.value("method").toString() == "NotifyStatus") {
        emit notificationReceived(data.value("params").toMap());
        return;
    }

    int id = data.value("id").toInt();
    ShellyRpcReply *reply = m_pendingReplies.value(id);

    if (!reply) {
        qCDebug(dcShelly()) << "Received a message which is neither a notification nor a reply to a request:" << message;
        return;
    }

    ShellyRpcReply::Status status = ShellyRpcReply::StatusSuccess;
    if (data.contains("error")) {
        QVariantMap errorMap = data.value("error").toMap();
        qCWarning(dcShelly()) << "Error in shelly command:" << errorMap.value("code").toInt() << errorMap.value("message");
        status = static_cast<ShellyRpcReply::Status>(errorMap.value("code").toInt());

        if (status == ShellyRpcReply::StatusAuthenticationRequired) {
            if (m_nonce == 0) {
                qCInfo(dcShelly) << "Authentication required. Initializing nonce and retrying...";

                QJsonParseError error;
                QVariantMap authInfo = QJsonDocument::fromJson(errorMap.value("message").toByteArray(), &error).toVariant().toMap();
                if (error.error != QJsonParseError::NoError) {
                    qCWarning(dcShelly()) << "Unable to parse auth error message. Authentication will not work.";
                    emit reply->finished(status, QVariantMap());
                    return;
                }
                m_nonce = authInfo.value("nonce").toInt();
                m_nc = authInfo.value("nc").toInt();
                QVariantMap newBody = reply->requestBody();
                newBody.insert("auth", createAuthMap());
                qCDebug(dcShelly) << "Sending request with auth" << qUtf8Printable(QJsonDocument::fromVariant(newBody).toJson());
                m_socket->sendTextMessage(QJsonDocument::fromVariant(newBody).toJson(QJsonDocument::Compact));
                return;
            } else {
                qCWarning(dcShelly()) << "Username and password seem to be wrong.";
            }
        }
    }

    emit reply->finished(status, data.value("result").toMap());
}

QVariantMap ShellyJsonRpcClient::createAuthMap() const
{
    int cnonce = std::rand();
    QByteArray ha1 = QString("%1:%2:%3").arg(m_user).arg(m_shellyId.toLower()).arg(m_password).toUtf8();
    ha1 = QCryptographicHash::hash(ha1, QCryptographicHash::Sha256).toHex();
    QByteArray ha2 = QByteArrayLiteral("dummy_method:dummy_uri");
    ha2 = QCryptographicHash::hash(ha2, QCryptographicHash::Sha256).toHex();
    QByteArray response = QString("%1:%2:%3:%4:auth:%5").arg(QString::fromUtf8(ha1)).arg(m_nonce).arg(m_nc).arg(cnonce).arg(QString::fromUtf8(ha2)).toUtf8();
    response = QCryptographicHash::hash(response, QCryptographicHash::Sha256).toHex();

    QVariantMap auth;
    auth.insert("realm", m_shellyId.toLower());
    auth.insert("username", m_user);
    auth.insert("nonce", m_nonce);
    auth.insert("cnonce", cnonce);
    auth.insert("response", response);
    auth.insert("algorithm", "SHA-256");
    return auth;
}
