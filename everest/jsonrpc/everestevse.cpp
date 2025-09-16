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

#include "everestevse.h"
#include "extern-plugininfo.h"

EverestEvse::EverestEvse(EverestJsonRpcClient *client, Thing *thing, QObject *parent)
    : QObject{parent},
    m_client{client},
    m_thing{thing}
{
    m_index = thing->paramValue("index").toInt();

    connect(m_client, &EverestJsonRpcClient::availableChanged, this, [this](bool available){
        if (available) {
            qCDebug(dcEverest()) << "Evse: The connection is now available";
            initialize();
        } else {
            qCDebug(dcEverest()) << "Evse: The connection is not available any more.";
            m_initialized = false;
            m_thing->setStateValue("connected", false);
        }
    });

    connect(m_client, &EverestJsonRpcClient::evseStatusChanged, this, [this](int evseIndex, const EverestJsonRpcClient::EVSEStatus &evseStatus){
        // We are only insterested in our own status
        if (m_index != evseIndex || !m_initialized)
            return;

        m_evseStatus = evseStatus;
        processEvseStatus();
    });

    connect(m_client, &EverestJsonRpcClient::hardwareCapabilitiesChanged, this, [this](int evseIndex, const EverestJsonRpcClient::HardwareCapabilities &hardwareCapabilities){
        // We are only insterested in our own status
        if (m_index != evseIndex || !m_initialized)
            return;

        m_hardwareCapabilities = hardwareCapabilities;
        processHardwareCapabilities();
    });

    connect(m_client, &EverestJsonRpcClient::meterDataChanged, this, [this](int evseIndex, const EverestJsonRpcClient::MeterData &meterData){
        // We are only insterested in our own status
        if (m_index != evseIndex || !m_initialized)
            return;

        m_meterData = meterData;
        processMeterData();
    });

    if (m_client->available()) {
        qCDebug(dcEverest()) << "Evse: The connection is already available. Initializing the instance...";
        initialize();
    }
}

int EverestEvse::index() const
{
    return m_index;
}

EverestJsonRpcReply *EverestEvse::setChargingAllowed(bool allowed)
{
    return m_client->evseSetChargingAllowed(m_index, allowed);
}

EverestJsonRpcReply *EverestEvse::setACChargingCurrent(double current)
{
    return m_client->evseSetACChargingCurrent(m_index, current);
}

EverestJsonRpcReply *EverestEvse::setACChargingPhaseCount(int phaseCount)
{
    return m_client->evseSetACChargingPhaseCount(m_index, phaseCount);
}

void EverestEvse::initialize()
{
    qCDebug(dcEverest()) << "Evse: Initializing data for" << m_thing->name();

    // Fetch all initial data for this device, once done we get notifications on data changes

    EverestJsonRpcReply *reply = nullptr;

    reply = m_client->evseGetInfo(m_index);
    m_pendingInitReplies.append(reply);
    connect(reply, &EverestJsonRpcReply::finished, reply, &EverestJsonRpcReply::deleteLater);
    connect(reply, &EverestJsonRpcReply::finished, this, [this, reply](){
        qCDebug(dcEverest()) << "Evse: Reply finished" << m_client->serverUrl().toString() << reply->method();
        if (reply->error()) {
            qCWarning(dcEverest()) << "Evse: JsonRpc reply finished with error" << reply->method() << reply->method() << reply->error();
            // FIXME: check what we do if an init call failes. Do we stay disconnected and show an error or do we ignore it...
        } else {
            QVariantMap result = reply->response().value("result").toMap();
            EverestJsonRpcClient::ResponseError error = EverestJsonRpcClient::parseResponseError(result.value("error").toString());
            if (error) {
                qCWarning(dcEverest()) << "Evse: Reply finished with an error" << reply->method() << error;
                // FIXME: check what we do if an init call failes. Do we stay disconnected and show an error or do we ignore it...
            } else {
                m_evseInfo = EverestJsonRpcClient::parseEvseInfo(result.value("info").toMap());
            }
        }

        // Check if we are done with the init process of this EVSE
        evaluateInitFinished(reply);
    });

    reply = m_client->evseGetHardwareCapabilities(m_index);
    m_pendingInitReplies.append(reply);
    connect(reply, &EverestJsonRpcReply::finished, reply, &EverestJsonRpcReply::deleteLater);
    connect(reply, &EverestJsonRpcReply::finished, this, [this, reply](){
        qCDebug(dcEverest()) << "Evse: Reply finished" << m_client->serverUrl().toString() << reply->method();
        if (reply->error()) {
            qCWarning(dcEverest()) << "Evse: JsonRpc reply finished with error" << reply->method() << reply->method() << reply->error();
            // FIXME: check what we do if an init call failes. Do we stay disconnected and show an error or do we ignore it...
        } else {
            QVariantMap result = reply->response().value("result").toMap();
            EverestJsonRpcClient::ResponseError error = EverestJsonRpcClient::parseResponseError(result.value("error").toString());
            if (error) {
                qCWarning(dcEverest()) << "Evse: Reply finished with an error" << reply->method() << error;
                // FIXME: check what we do if an init call failes. Do we stay disconnected and show an error or do we ignore it...
            } else {
                // Store data, thy will be processed once all replies arrived
                m_hardwareCapabilities = EverestJsonRpcClient::parseHardwareCapabilities(result.value("hardware_capabilities").toMap());
            }
        }

        // Check if we are done with the init process of this EVSE
        evaluateInitFinished(reply);
    });

    reply = m_client->evseGetStatus(m_index);
    m_pendingInitReplies.append(reply);
    connect(reply, &EverestJsonRpcReply::finished, reply, &EverestJsonRpcReply::deleteLater);
    connect(reply, &EverestJsonRpcReply::finished, this, [this, reply](){
        qCDebug(dcEverest()) << "Evse: Reply finished" << m_client->serverUrl().toString() << reply->method();
        if (reply->error()) {
            qCWarning(dcEverest()) << "Evse: JsonRpc reply finished with error" << reply->method() << reply->method() << reply->error();
            // FIXME: check what we do if an init call failes. Do we stay disconnected and show an error or do we ignore it...
        } else {
            QVariantMap result = reply->response().value("result").toMap();
            EverestJsonRpcClient::ResponseError error = EverestJsonRpcClient::parseResponseError(result.value("error").toString());
            if (error) {
                qCWarning(dcEverest()) << "Evse: Reply finished with an error" << reply->method() << error;
                // FIXME: check what we do if an init call failes. Do we stay disconnected and show an error or do we ignore it...
            } else {
                // Store data, thy will be processed once all replies arrived
                m_evseStatus = EverestJsonRpcClient::parseEvseStatus(result.value("status").toMap());
            }
        }

        // Check if we are done with the init process of this EVSE
        evaluateInitFinished(reply);
    });

    reply = m_client->evseGetMeterData(m_index);
    m_pendingInitReplies.append(reply);
    connect(reply, &EverestJsonRpcReply::finished, reply, &EverestJsonRpcReply::deleteLater);
    connect(reply, &EverestJsonRpcReply::finished, this, [this, reply](){
        qCDebug(dcEverest()) << "Evse: Reply finished" << m_client->serverUrl().toString() << reply->method();
        if (reply->error()) {
            qCWarning(dcEverest()) << "Evse: JsonRpc reply finished with error" << reply->method() << reply->method() << reply->error();
            // FIXME: check what we do if an init call failes. Do we stay disconnected and show an error or do we ignore it...
        } else {
            QVariantMap result = reply->response().value("result").toMap();
            EverestJsonRpcClient::ResponseError error = EverestJsonRpcClient::parseResponseError(result.value("error").toString());
            if (error) {
                if (error == EverestJsonRpcClient::ResponseErrorErrorNoDataAvailable) {
                    qCDebug(dcEverest()) << "Evse: There are no meter data available. Either there is no meter or the meter data are not available yet on EVSE side.";
                } else {
                    // FIXME: check what we do if an init call failes. Do we stay disconnected and show an error or do we ignore it...
                    qCWarning(dcEverest()) << "Evse: Reply finished with an error" << reply->method() << error;
                }
            } else {
                // Store data, thy will be processed once all replies arrived
                m_meterData = EverestJsonRpcClient::parseMeterData(result.value("meter_data").toMap());
            }
        }

        // Check if we are done with the init process of this EVSE
        evaluateInitFinished(reply);
    });
}

void EverestEvse::evaluateInitFinished(EverestJsonRpcReply *reply)
{
    if (m_initialized)
        return;

    m_pendingInitReplies.removeAll(reply);

    if (m_pendingInitReplies.isEmpty()) {
        qCDebug(dcEverest()) << "Evse: The initialization of" << m_thing->name() << "has finished, the charger is now connected.";
        m_initialized = true;

        // Set all initial states
        m_thing->setStateValue("connected", true);

        // Process all data after beeing conected
        processEvseStatus();
        processHardwareCapabilities();
        processMeterData();
    }
}

void EverestEvse::processEvseStatus()
{
    if (m_thing->thingClassId() == everestChargerAcThingClassId) {
        m_thing->setStateValue(everestChargerAcPowerStateTypeId, m_evseStatus.chargingAllowed);
        m_thing->setStateValue(everestChargerAcSessionEnergyStateTypeId, m_evseStatus.chargedEnergyWh / 1000.0);
        m_thing->setStateValue(everestChargerAcStateStateTypeId, m_evseStatus.evseStateString);
        m_thing->setStateValue(everestChargerAcChargingStateTypeId, m_evseStatus.evseState == EverestJsonRpcClient::EvseStateCharging);
        m_thing->setStateValue(everestChargerAcPluggedInStateTypeId, m_evseStatus.evseState != EverestJsonRpcClient::EvseStateUnplugged);

        m_thing->setStateValue(everestChargerAcPhaseCountStateTypeId, m_evseStatus.acChargeStatus.activePhaseCount);
        m_thing->setStateValue(everestChargerAcDesiredPhaseCountStateTypeId, m_evseStatus.acChargeStatus.activePhaseCount);

        m_thing->setStateValue(everestChargerAcMaxChargingCurrentStateTypeId, m_evseStatus.acChargeParameters.maxCurrent);
    }
}

void EverestEvse::processHardwareCapabilities()
{
    if (m_thing->thingClassId() == everestChargerAcThingClassId) {
        if (!m_hardwareCapabilities.phaseSwitchDuringCharging) {
            // Only option left is set the desired phase count to 3, force that value
            m_thing->setStatePossibleValues(everestChargerAcDesiredPhaseCountStateTypeId, { 3 });
            m_thing->setStateValue(everestChargerAcDesiredPhaseCountStateTypeId, 3);
            m_thing->setStateValue(everestChargerAcPhaseCountStateTypeId, 3);
        } else {
            m_thing->setStatePossibleValues(everestChargerAcDesiredPhaseCountStateTypeId, { 1, 3 });
        }

        m_thing->setStateMaxValue(everestChargerAcMaxChargingCurrentStateTypeId, m_hardwareCapabilities.maxCurrentImport);
        m_thing->setStateMinValue(everestChargerAcMaxChargingCurrentStateTypeId, m_hardwareCapabilities.minCurrentImport == 0 ? 6 : m_hardwareCapabilities.minCurrentImport);
    }
}

void EverestEvse::processMeterData()
{
    if (m_thing->thingClassId() == everestChargerAcThingClassId) {

        m_thing->setStateValue(everestChargerAcCurrentPowerStateTypeId, m_meterData.powerTotal);

        m_thing->setStateValue(everestChargerAcCurrentPowerPhaseAStateTypeId, m_meterData.powerL1);
        m_thing->setStateValue(everestChargerAcCurrentPowerPhaseBStateTypeId, m_meterData.powerL2);
        m_thing->setStateValue(everestChargerAcCurrentPowerPhaseCStateTypeId, m_meterData.powerL3);

        m_thing->setStateValue(everestChargerAcCurrentPhaseAStateTypeId, m_meterData.currentL1);
        m_thing->setStateValue(everestChargerAcCurrentPhaseBStateTypeId, m_meterData.currentL2);
        m_thing->setStateValue(everestChargerAcCurrentPhaseCStateTypeId, m_meterData.currentL3);

        m_thing->setStateValue(everestChargerAcVoltagePhaseAStateTypeId, m_meterData.voltageL1);
        m_thing->setStateValue(everestChargerAcVoltagePhaseBStateTypeId, m_meterData.voltageL2);
        m_thing->setStateValue(everestChargerAcVoltagePhaseCStateTypeId, m_meterData.voltageL3);

        m_thing->setStateValue(everestChargerAcTotalEnergyConsumedStateTypeId, m_meterData.energyImportedTotal / 1000.0);
    }
}
