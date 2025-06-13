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

    if (m_client->available()) {
        qCDebug(dcEverest()) << "Evse: The connection is already available. Initializing the instance...";
    }
}

int EverestEvse::index() const
{
    return m_index;
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
                // No error, store the data
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
                // No error, store the data
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
                // No error, store the data
                m_evseStatus = EverestJsonRpcClient::parseEvseStatus(result.value("status").toMap());
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

        processEvseStatus();
    }

}

void EverestEvse::processEvseStatus()
{
    if (m_thing->thingClassId() == everestChargerAcThingClassId) {
        m_thing->setStateValue(everestChargerAcStateStateTypeId, m_evseStatus.evseStateString);
        m_thing->setStateValue(everestChargerAcChargingStateTypeId, m_evseStatus.evseState == EverestJsonRpcClient::EvseStateCharging);
        m_thing->setStateValue(everestChargerAcPluggedInStateTypeId, m_evseStatus.evseState != EverestJsonRpcClient::EvseStateUnplugged);
        // TODO: check what we do it available is false, shall we disconnect the thing or introduce a new state
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
            m_thing->setStateValue(everestChargerAcPhaseCountStateTypeId, m_thing->stateValue(everestChargerAcDesiredPhaseCountStateTypeId));
        }

        m_thing->setStateMaxValue(everestChargerAcMaxChargingCurrentStateTypeId, m_hardwareCapabilities.maxCurrentImport);
        m_thing->setStateMinValue(everestChargerAcMaxChargingCurrentStateTypeId, m_hardwareCapabilities.minCurrentImport == 0 ? 6 : m_hardwareCapabilities.minCurrentImport);
    }
}
