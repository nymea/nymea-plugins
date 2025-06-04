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

#include "everestjsonrpcclient.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QJsonParseError>

EverestJsonRpcClient::EverestJsonRpcClient(QObject *parent)
    : QObject{parent}
{
    m_interface = new EverestJsonRpcInterface(this);
    connect(m_interface, &EverestJsonRpcInterface::connectedChanged, this, [this](bool connected){
        if (connected) {
            EverestJsonRpcReply *reply = hello();
            connect(reply, &EverestJsonRpcReply::finished, this, [this, reply](){
                qCDebug(dcEverest()) << "Reply finished" << m_interface->serverUrl().toString() << reply->method();
            });
        } else {
            // Client not available any more
        }
    });

    connect(m_interface, &EverestJsonRpcInterface::dataReceived, this, &EverestJsonRpcClient::processDataPacket);
}

QUrl EverestJsonRpcClient::serverUrl()
{
    return m_interface->serverUrl();
}

void EverestJsonRpcClient::setSeverUrl(const QUrl &serverUrl)
{
    m_interface->connectServer(serverUrl);
}

EverestJsonRpcReply *EverestJsonRpcClient::hello()
{
    EverestJsonRpcReply *reply = new EverestJsonRpcReply(m_commandId, "API.Hello", QVariantMap(), this);
    qCDebug(dcEverest()) << "Calling" << reply->method();
    sendRequest(reply->requestMap());
    m_replies.insert(m_commandId, reply);
    m_commandId++;
    return reply;
}

void EverestJsonRpcClient::sendRequest(const QVariantMap &request)
{
    QByteArray data = QJsonDocument::fromVariant(request).toJson(QJsonDocument::Compact) + '\n';
    qCDebug(dcEverest()) << "-->" << m_interface->serverUrl().toString() << qUtf8Printable(data);
    m_interface->sendData(data);
}

void EverestJsonRpcClient::processDataPacket(const QByteArray &data)
{
    qCDebug(dcEverest()) << "<--" << m_interface->serverUrl().toString() << qUtf8Printable(data);

    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcEverest()) << "Invalid JSON data recived" << m_interface->serverUrl().toString() << error.errorString();
        return;
    }

    QVariantMap dataMap = jsonDoc.toVariant().toMap();

    int commandId = dataMap.value("id").toInt();
    EverestJsonRpcReply *reply = m_replies.take(commandId);
    if (reply) {
        qCDebug(dcEverest()) << QString("Got response for %1: %2").arg(reply->method(), QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Indented)));

        // if (dataMap.value("status").toString() == "error") {
        //     qCWarning(dcEverest()) << "Api error happend" << dataMap.value("error").toString();
        //     // FIMXME: handle json layer errors
        // }

        reply->setResponse(dataMap);
        emit reply->finished();
        return;
    }
}
