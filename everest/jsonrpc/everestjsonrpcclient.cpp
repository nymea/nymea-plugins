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

#include <QMetaEnum>
#include <QJsonDocument>
#include <QJsonParseError>

EverestJsonRpcClient::EverestJsonRpcClient(QObject *parent)
    : QObject{parent},
    m_interface{new EverestJsonRpcInterface(this)}
{
    connect(m_interface, &EverestJsonRpcInterface::dataReceived, this, &EverestJsonRpcClient::processDataPacket);
    connect(m_interface, &EverestJsonRpcInterface::connectedChanged, this, [this](bool connected){

        qCDebug(dcEverest()) << "Interface is" << (connected ? "now connected" : "not connected any more");

        if (connected) {

            // The interface is connected, fetch initial data and mark the client as available if successfull,
            // otherwise emit connection error signal and close the connection

            EverestJsonRpcReply *reply = apiHello();
            connect(reply, &EverestJsonRpcReply::finished, reply, &EverestJsonRpcReply::deleteLater);
            connect(reply, &EverestJsonRpcReply::finished, this, [this, reply](){
                qCDebug(dcEverest()) << "Reply finished" << m_interface->serverUrl().toString() << reply->method();
                if (reply->error()) {
                    qCWarning(dcEverest()) << "JsonRpc reply finished with error" << reply->method() << reply->error();
                    disconnectFromServer();
                    emit connectionErrorOccurred();
                    return;
                }

                // Verify data format and API version
                QVariantMap result = reply->response().value("result").toMap();
                if (!result.contains("api_version") || !result.contains("everest_version") || !result.contains("charger_info")) {
                    qCWarning(dcEverest()) << "Missing expected properties in JsonRpc response" << reply->method();
                    disconnectFromServer();
                    emit connectionErrorOccurred();
                    return;
                }

                //D | Everest: <-- {"id":0,"jsonrpc":"2.0","result":{"api_version":"0.0.1","authentication_required":false,"charger_info":{"firmware_version":"unknown","model":"unknown","serial":"unknown","vendor":"unknown"},"everest_version":""}
                m_apiVersion = result.value("api_version").toString();
                m_everestVersion = result.value("everest_version").toString();
                m_authenticationRequired = result.value("authentication_required").toBool();

                QVariantMap chargerInfoMap = result.value("charger_info").toMap();
                m_chargerInfo.vendor = chargerInfoMap.value("vendor").toString();
                m_chargerInfo.model = chargerInfoMap.value("model").toString();
                m_chargerInfo.serialNumber = chargerInfoMap.value("serial").toString();
                m_chargerInfo.firmwareVersion = chargerInfoMap.value("firmware_version").toString();

                EverestJsonRpcReply *reply = chargePointGetEVSEInfos();
                connect(reply, &EverestJsonRpcReply::finished, reply, &EverestJsonRpcReply::deleteLater);
                connect(reply, &EverestJsonRpcReply::finished, this, [this, reply](){
                    qCDebug(dcEverest()) << "Reply finished" << m_interface->serverUrl().toString() << reply->method()
                    << qUtf8Printable(QJsonDocument::fromVariant(reply->response()).toJson(QJsonDocument::Indented));

                    if (reply->error()) {
                        qCWarning(dcEverest()) << "JsonRpc reply finished with error" << reply->method() << reply->error();
                        disconnectFromServer();
                        emit connectionErrorOccurred();
                        return;
                    }

                    QVariantMap result = reply->response().value("result").toMap();
                    QString errorString = result.value("error").toString();
                    if (errorString != "NoError") {
                        qCWarning(dcEverest()) << "Error requesting" << reply->method() << errorString;
                        disconnectFromServer();
                        emit connectionErrorOccurred();
                        return;
                    }

                    m_evseInfos.clear();
                    foreach (const QVariant &evseInfoVariant, result.value("infos").toList()) {
                        m_evseInfos.append(parseEvseInfo(evseInfoVariant.toMap()));
                    }

                    // We are done with the init and the client is now available
                    if (!m_available) {
                        m_available = true;
                        emit availableChanged(m_available);
                    }

                });
            });
        } else {
            // Client disconnected. Clean up any pending replies
            qCDebug(dcEverest()) << "Lost connection to the server. Finish any pending replies ...";
            foreach (EverestJsonRpcReply *reply, m_replies) {
                reply->finishReply(EverestJsonRpcReply::ErrorConnectionError);
            }

            if (m_available) {
                m_available = false;
                emit availableChanged(m_available);
            }
        }
    });

}

QUrl EverestJsonRpcClient::serverUrl()
{
    return m_interface->serverUrl();
}

bool EverestJsonRpcClient::available() const
{
    return m_available;
}

QString EverestJsonRpcClient::apiVersion() const
{
    return m_apiVersion;
}

QString EverestJsonRpcClient::everestVersion() const
{
    return m_everestVersion;
}

QList<EverestJsonRpcClient::EVSEInfo> EverestJsonRpcClient::evseInfos() const
{
    return m_evseInfos;
}

EverestJsonRpcClient::ChargerInfo EverestJsonRpcClient::chargerInfo() const
{
    return m_chargerInfo;
}

EverestJsonRpcReply *EverestJsonRpcClient::apiHello()
{
    EverestJsonRpcReply *reply = new EverestJsonRpcReply(m_commandId, "API.Hello", QVariantMap(), this);
    qCDebug(dcEverest()) << "Calling" << reply->method();
    sendRequest(reply);
    return reply;
}

EverestJsonRpcReply *EverestJsonRpcClient::chargePointGetEVSEInfos()
{
    EverestJsonRpcReply *reply = new EverestJsonRpcReply(m_commandId, "ChargePoint.GetEVSEInfos", QVariantMap(), this);
    qCDebug(dcEverest()) << "Calling" << reply->method();
    sendRequest(reply);
    return reply;
}

EverestJsonRpcReply *EverestJsonRpcClient::evseGetInfo()
{
    EverestJsonRpcReply *reply = new EverestJsonRpcReply(m_commandId, "EVSE.GetInfo", QVariantMap(), this);
    qCDebug(dcEverest()) << "Calling" << reply->method();
    sendRequest(reply);
    return reply;
}

EverestJsonRpcReply *EverestJsonRpcClient::evseGetStatus(int evseIndex)
{
    QVariantMap params;
    params.insert("evse_index", evseIndex);

    EverestJsonRpcReply *reply = new EverestJsonRpcReply(m_commandId, "EVSE.GetStatus", params, this);
    qCDebug(dcEverest()) << "Calling" << reply->method();
    sendRequest(reply);
    return reply;
}

EverestJsonRpcReply *EverestJsonRpcClient::evseGetHardwareCapabilities(int evseIndex)
{
    QVariantMap params;
    params.insert("evse_index", evseIndex);

    EverestJsonRpcReply *reply = new EverestJsonRpcReply(m_commandId, "EVSE.GetStatus", params, this);
    qCDebug(dcEverest()) << "Calling" << reply->method();
    sendRequest(reply);
    return reply;
}

void EverestJsonRpcClient::connectToServer(const QUrl &serverUrl)
{
    m_interface->connectServer(serverUrl);
}

void EverestJsonRpcClient::disconnectFromServer()
{
    m_interface->disconnectServer();
}

void EverestJsonRpcClient::sendRequest(EverestJsonRpcReply *reply)
{
    QVariantMap requestMap = reply->requestMap();
    QByteArray data = QJsonDocument::fromVariant(requestMap).toJson(QJsonDocument::Compact) + '\n';

    qCDebug(dcEverest()) << "-->" << m_interface->serverUrl().toString() << qUtf8Printable(data);
    m_interface->sendData(data);

    m_replies.insert(m_commandId, reply);
    m_commandId++;

    reply->startWaiting();
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
    if (!dataMap.contains("id") || dataMap.value("jsonrpc").toString() != "2.0") {
        qCWarning(dcEverest()) << "Received valid JSON data but does not seem to be a JSON RPC 2.0 format" << m_interface->serverUrl().toString() << qUtf8Printable(data);
        return;
    }

    int commandId = dataMap.value("id").toInt();
    EverestJsonRpcReply *reply = m_replies.take(commandId);
    if (reply) {
        reply->setResponse(dataMap);

        // Verify if we received a json rpc error
        if (dataMap.contains("error")) {
            reply->finishReply(EverestJsonRpcReply::ErrorJsonRpcError);
        } else {
            reply->finishReply();
        }

        return;
    } else {
        // Data without reply, check if this is a notification
        qCDebug(dcEverest()) << "Received data without reply" << qUtf8Printable(data);
    }
}

EverestJsonRpcClient::EVSEInfo EverestJsonRpcClient::parseEvseInfo(const QVariantMap &evseInfoMap)
{
    EVSEInfo evseInfo;
    evseInfo.index = evseInfoMap.value("index").toInt();
    evseInfo.id = evseInfoMap.value("id").toString();
    evseInfo.bidirectionalCharging = evseInfoMap.value("bidi_charging").toBool();
    foreach (const QVariant &connectorInfoVariant, evseInfoMap.value("available_connectors").toList()) {
        evseInfo.availableConnectors.append(parseConnectorInfo(connectorInfoVariant.toMap()));
    }
    return evseInfo;
}

EverestJsonRpcClient::ConnectorInfo EverestJsonRpcClient::parseConnectorInfo(const QVariantMap &connectorInfoMap)
{
    ConnectorInfo connectorInfo;
    connectorInfo.connectorId = connectorInfoMap.value("id").toInt();
    connectorInfo.type = parseConnectorType(connectorInfoMap.value("type").toString());
    connectorInfo.description = connectorInfoMap.value("description").toString();
    return connectorInfo;
}

EverestJsonRpcClient::ConnectorType EverestJsonRpcClient::parseConnectorType(const QString &connectorTypeString)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<ConnectorType>();
    return static_cast<ConnectorType>(metaEnum.keyToValue(QString("ConnectorType").append(connectorTypeString).toUtf8()));
}

