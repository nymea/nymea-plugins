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

Q_DECLARE_LOGGING_CATEGORY(dcEverestTraffic)

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
                    qCDebug(dcEverest()) << "Reply finished" << m_interface->serverUrl().toString() << reply->method();

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

EverestJsonRpcReply *EverestJsonRpcClient::evseGetInfo(int evseIndex)
{
    QVariantMap params;
    params.insert("evse_index", evseIndex);

    EverestJsonRpcReply *reply = createReply("EVSE.GetInfo", params);
    qCDebug(dcEverest()) << "Calling" << reply->method() << params;
    sendRequest(reply);
    return reply;
}

EverestJsonRpcReply *EverestJsonRpcClient::evseGetStatus(int evseIndex)
{
    QVariantMap params;
    params.insert("evse_index", evseIndex);

    EverestJsonRpcReply *reply = createReply("EVSE.GetStatus", params);
    qCDebug(dcEverest()) << "Calling" << reply->method() << params;
    sendRequest(reply);
    return reply;
}

EverestJsonRpcReply *EverestJsonRpcClient::evseGetHardwareCapabilities(int evseIndex)
{
    QVariantMap params;
    params.insert("evse_index", evseIndex);

    EverestJsonRpcReply *reply = createReply("EVSE.GetHardwareCapabilities", params);
    qCDebug(dcEverest()) << "Calling" << reply->method() << params;
    sendRequest(reply);
    return reply;
}

EverestJsonRpcReply *EverestJsonRpcClient::evseGetMeterData(int evseIndex)
{
    QVariantMap params;
    params.insert("evse_index", evseIndex);

    EverestJsonRpcReply *reply = createReply("EVSE.GetMeterData", params);
    qCDebug(dcEverest()) << "Calling" << reply->method() << params;
    sendRequest(reply);
    return reply;
}

EverestJsonRpcReply *EverestJsonRpcClient::evseSetChargingAllowed(int evseIndex, bool allowed)
{
    QVariantMap params;
    params.insert("evse_index", evseIndex);
    params.insert("charging_allowed", allowed);

    EverestJsonRpcReply *reply = createReply("EVSE.SetChargingAllowed", params);
    qCDebug(dcEverest()) << "Calling" << reply->method() << params;
    sendRequest(reply);
    return reply;
}

EverestJsonRpcReply *EverestJsonRpcClient::evseSetACChargingCurrent(int evseIndex, double current)
{
    QVariantMap params;
    params.insert("evse_index", evseIndex);
    params.insert("max_current", current);

    EverestJsonRpcReply *reply = createReply("EVSE.SetACChargingCurrent", params);
    qCDebug(dcEverest()) << "Calling" << reply->method() << params;
    sendRequest(reply);
    return reply;
}

EverestJsonRpcReply *EverestJsonRpcClient::evseSetACChargingPhaseCount(int evseIndex, int phaseCount)
{
    QVariantMap params;
    params.insert("evse_index", evseIndex);
    params.insert("phase_count", phaseCount);

    EverestJsonRpcReply *reply = createReply("EVSE.SetACChargingPhaseCount", params);
    qCDebug(dcEverest()) << "Calling" << reply->method() << params;
    sendRequest(reply);
    return reply;
}

EverestJsonRpcClient::ResponseError EverestJsonRpcClient::parseResponseError(const QString &responseErrorString)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<ResponseError>();
    return static_cast<ResponseError>(metaEnum.keyToValue(QString("ResponseError").append(responseErrorString).toUtf8()));
}

EverestJsonRpcClient::ConnectorType EverestJsonRpcClient::parseConnectorType(const QString &connectorTypeString)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<ConnectorType>();
    return static_cast<ConnectorType>(metaEnum.keyToValue(QString("ConnectorType").append(connectorTypeString).toUtf8()));
}

EverestJsonRpcClient::ChargeProtocol EverestJsonRpcClient::parseChargeProtocol(const QString &chargeProtocolString)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<ChargeProtocol>();
    return static_cast<ChargeProtocol>(metaEnum.keyToValue(QString("ChargeProtocol").append(chargeProtocolString).toUtf8()));
}

EverestJsonRpcClient::EvseState EverestJsonRpcClient::parseEvseState(const QString &evseStateString)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<EvseState>();
    return static_cast<EvseState>(metaEnum.keyToValue(QString("EvseState").append(evseStateString).toUtf8()));
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

    // optional
    if (connectorInfoMap.contains("description"))
        connectorInfo.description = connectorInfoMap.value("description").toString();

    return connectorInfo;
}

EverestJsonRpcClient::EVSEStatus EverestJsonRpcClient::parseEvseStatus(const QVariantMap &evseStatusMap)
{
    EVSEStatus evseStatus;
    evseStatus.chargedEnergyWh = evseStatusMap.value("charged_energy_wh").toDouble();
    evseStatus.dischargedEnergyWh = evseStatusMap.value("discharged_energy_wh").toDouble();
    evseStatus.chargingDuration = evseStatusMap.value("charging_duration_s").toInt();
    evseStatus.chargingAllowed = evseStatusMap.value("charging_allowed").toBool();
    evseStatus.available = evseStatusMap.value("available").toBool();
    evseStatus.activeConnectorIndex = evseStatusMap.value("active_connector_index").toInt();
    evseStatus.errorPresent = evseStatusMap.value("error_present").toBool();
    evseStatus.chargeProtocol = parseChargeProtocol(evseStatusMap.value("charge_protocol").toString());
    evseStatus.evseState = parseEvseState(evseStatusMap.value("state").toString());
    evseStatus.evseStateString = evseStatusMap.value("state").toString();

    // optional
    if (evseStatusMap.contains("ac_charge_status"))
        evseStatus.acChargeStatus = EverestJsonRpcClient::parseACChargeStatus(evseStatusMap.value("ac_charge_status").toMap());

    // optional
    if (evseStatusMap.contains("ac_charge_param"))
        evseStatus.acChargeParameters = EverestJsonRpcClient::parseACChargeParameters(evseStatusMap.value("ac_charge_param").toMap());

    return evseStatus;
}

EverestJsonRpcClient::ACChargeStatus EverestJsonRpcClient::parseACChargeStatus(const QVariantMap &acChargeStatusMap)
{
    EverestJsonRpcClient::ACChargeStatus status;
    status.activePhaseCount = acChargeStatusMap.value("evse_active_phase_count").toInt();
    return status;
}

EverestJsonRpcClient::ACChargeParameters EverestJsonRpcClient::parseACChargeParameters(const QVariantMap &acChargeParametersMap)
{
    EverestJsonRpcClient::ACChargeParameters params;
    params.maxCurrent = acChargeParametersMap.value("evse_max_current").toInt();
    params.maxPhaseCount = acChargeParametersMap.value("evse_max_phase_count").toInt();
    return params;
}

EverestJsonRpcClient::HardwareCapabilities EverestJsonRpcClient::parseHardwareCapabilities(const QVariantMap &hardwareCapabilitiesMap)
{
    HardwareCapabilities hardwareCapabilities;
    hardwareCapabilities.maxCurrentExport = hardwareCapabilitiesMap.value("max_current_A_export").toDouble();
    hardwareCapabilities.maxCurrentImport = hardwareCapabilitiesMap.value("max_current_A_import").toDouble();
    hardwareCapabilities.maxPhaseCountExport = hardwareCapabilitiesMap.value("max_phase_count_export").toInt();
    hardwareCapabilities.maxPhaseCountImport = hardwareCapabilitiesMap.value("max_phase_count_import").toInt();
    hardwareCapabilities.minCurrentExport = hardwareCapabilitiesMap.value("min_current_A_export").toDouble();
    hardwareCapabilities.minCurrentImport = hardwareCapabilitiesMap.value("min_current_A_import").toDouble();
    hardwareCapabilities.minPhaseCountExport = hardwareCapabilitiesMap.value("min_phase_count_export").toInt();
    hardwareCapabilities.minPhaseCountImport = hardwareCapabilitiesMap.value("min_phase_count_import").toInt();
    hardwareCapabilities.phaseSwitchDuringCharging = hardwareCapabilitiesMap.value("phase_switch_during_charging").toBool();
    return hardwareCapabilities;
}

EverestJsonRpcClient::MeterData EverestJsonRpcClient::parseMeterData(const QVariantMap &meterDataMap)
{
    MeterData meterData;

    meterData.meterId = meterDataMap.value("meter_id").toString();

    meterData.energyImportedL1 = meterDataMap.value("energy_Wh_import").toMap().value("L1").toFloat();
    meterData.energyImportedL2 = meterDataMap.value("energy_Wh_import").toMap().value("L2").toFloat();
    meterData.energyImportedL3 = meterDataMap.value("energy_Wh_import").toMap().value("L3").toFloat();
    meterData.energyImportedTotal = meterDataMap.value("energy_Wh_import").toMap().value("total").toFloat();

    // optional
    if (meterDataMap.contains("serial_number"))
        meterData.serialNumber = meterDataMap.value("serial_number").toString();

    // optional
    if (meterDataMap.contains("phase_seq_error"))
        meterData.phaseSequenceError = meterDataMap.value("phase_seq_error").toBool();

    // optional
    if (meterDataMap.contains("power_W")) {
        meterData.powerL1 = meterDataMap.value("power_W").toMap().value("L1").toFloat();
        meterData.powerL2 = meterDataMap.value("power_W").toMap().value("L2").toFloat();
        meterData.powerL3 = meterDataMap.value("power_W").toMap().value("L3").toFloat();
        meterData.powerTotal = meterDataMap.value("power_W").toMap().value("total").toFloat();
    }

    // optional
    if (meterDataMap.contains("voltage_V")) {
        meterData.voltageL1 = meterDataMap.value("voltage_V").toMap().value("L1").toFloat();
        meterData.voltageL2 = meterDataMap.value("voltage_V").toMap().value("L2").toFloat();
        meterData.voltageL3 = meterDataMap.value("voltage_V").toMap().value("L3").toFloat();
    }

    // optional
    if (meterDataMap.contains("current_A")) {
        meterData.currentL1 = meterDataMap.value("current_A").toMap().value("L1").toFloat();
        meterData.currentL2 = meterDataMap.value("current_A").toMap().value("L2").toFloat();
        meterData.currentL3 = meterDataMap.value("current_A").toMap().value("L3").toFloat();
        meterData.currentN = meterDataMap.value("current_A").toMap().value("N").toFloat();
    }

    // optional
    if (meterDataMap.contains("energy_Wh_export")) {
        meterData.energyExportedL1 = meterDataMap.value("energy_Wh_export").toMap().value("L1").toFloat();
        meterData.energyExportedL2 = meterDataMap.value("energy_Wh_export").toMap().value("L2").toFloat();
        meterData.energyExportedL3 = meterDataMap.value("energy_Wh_export").toMap().value("L3").toFloat();
        meterData.energyExportedTotal = meterDataMap.value("energy_Wh_export").toMap().value("total").toFloat();
    }

    // optional
    if (meterDataMap.contains("frequency_Hz")) {
        meterData.frequencyL1 = meterDataMap.value("frequency_Hz").toMap().value("L1").toFloat();
        meterData.frequencyL2 = meterDataMap.value("frequency_Hz").toMap().value("L2").toFloat();
        meterData.frequencyL3 = meterDataMap.value("frequency_Hz").toMap().value("L3").toFloat();
    }

    return meterData;
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

    m_replies.insert(reply->commandId(), reply);

    connect(reply, &EverestJsonRpcReply::finished, this, [this, reply](){
        // Clean up internals after finished (in any error case),
        // it is up to the caller to delete the reply object itself
        m_replies.remove(reply->commandId());
    });

    qCDebug(dcEverestTraffic()) << "-->" << m_interface->serverUrl().toString() << qUtf8Printable(data);
    m_interface->sendData(data);
    reply->startWaiting();
}

void EverestJsonRpcClient::processDataPacket(const QByteArray &data)
{
    qCDebug(dcEverestTraffic()) << "<--" << m_interface->serverUrl().toString() << qUtf8Printable(data);

    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcEverest()) << "Invalid JSON data recived" << m_interface->serverUrl().toString() << error.errorString();
        return;
    }

    QVariantMap dataMap = jsonDoc.toVariant().toMap();
    if (dataMap.value("jsonrpc").toString() != "2.0") {
        qCWarning(dcEverest()) << "Received valid JSON data but does not seem to be a JSON RPC 2.0 format" << m_interface->serverUrl().toString() << qUtf8Printable(data);
        return;
    }

    if (dataMap.contains("id")) {

        // Response to a request
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
            qCDebug(dcEverest()) << "Received response data without any pending reply, maybe the reply timed out:" << qUtf8Printable(data);
        }
    } else {

        // A Notification is a Request object without an "id" member.
        QString notification = dataMap.value("method").toString();
        QVariantMap params = dataMap.value("params").toMap();

        qCDebug(dcEverest()) << "Received notification" << notification;

        if (notification == "EVSE.StatusChanged") {
            int evseIndex = params.value("evse_index").toInt();
            EVSEStatus evseStatus = EverestJsonRpcClient::parseEvseStatus(params.value("evse_status").toMap());
            emit evseStatusChanged(evseIndex, evseStatus);
        } else if (notification == "ChargePoint.ActiveErrorsChanged") {
            // TODO
            qCWarning(dcEverest()) << "Active errors changed" << qUtf8Printable(QJsonDocument::fromVariant(params).toJson());
        } else if (notification == "EVSE.HardwareCapabilitiesChanged") {
            int evseIndex = params.value("evse_index").toInt();
            HardwareCapabilities hardwareCapabilities = EverestJsonRpcClient::parseHardwareCapabilities(params.value("hardware_capabilities").toMap());
            emit hardwareCapabilitiesChanged(evseIndex, hardwareCapabilities);
        } else if (notification == "EVSE.MeterDataChanged") {
            int evseIndex = params.value("evse_index").toInt();
            MeterData meterData = EverestJsonRpcClient::parseMeterData(params.value("meter_data").toMap());
            emit meterDataChanged(evseIndex, meterData);
        }
    }
}

EverestJsonRpcReply *EverestJsonRpcClient::createReply(QString method, QVariantMap params)
{
    int commandId = m_commandId;
    m_commandId += 1;

    return new EverestJsonRpcReply(commandId, method, params, this);
}

EverestJsonRpcReply *EverestJsonRpcClient::apiHello()
{
    EverestJsonRpcReply *reply = createReply("API.Hello", QVariantMap());
    qCDebug(dcEverest()) << "Calling" << reply->method();
    sendRequest(reply);
    return reply;
}

EverestJsonRpcReply *EverestJsonRpcClient::chargePointGetEVSEInfos()
{
    EverestJsonRpcReply *reply = createReply("ChargePoint.GetEVSEInfos", QVariantMap());
    qCDebug(dcEverest()) << "Calling" << reply->method();
    sendRequest(reply);
    return reply;
}
