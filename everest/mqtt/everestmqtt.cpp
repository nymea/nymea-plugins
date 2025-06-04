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

#include "everestmqtt.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QJsonParseError>

#include <loggingcategories.h>

NYMEA_LOGGING_CATEGORY(dcEverestTraffic, "EverestTraffic")

EverestMqtt::EverestMqtt(MqttClient *client, Thing *thing, QObject *parent)
    : QObject{parent},
    m_client{client},
    m_thing{thing}
{
    m_connector = m_thing->paramValue(everestMqttThingConnectorParamTypeId).toString();
    m_topicPrefix = "everest_api/" + m_connector;

    m_subscribedTopics.append(buildTopic("hardware_capabilities"));
    m_subscribedTopics.append(buildTopic("limits"));
    m_subscribedTopics.append(buildTopic("powermeter"));
    m_subscribedTopics.append(buildTopic("session_info"));
    m_subscribedTopics.append(buildTopic("telemetry"));

    connect(m_client, &MqttClient::connected, this, &EverestMqtt::onConnected);
    connect(m_client, &MqttClient::disconnected, this, &EverestMqtt::onDisconnected);
    connect(m_client, &MqttClient::publishReceived, this, &EverestMqtt::onPublishReceived);
    connect(m_client, &MqttClient::subscribed, this, &EverestMqtt::onSubscribed);

    m_aliveTimer.setInterval(2000);
    m_aliveTimer.setSingleShot(true);
    connect(&m_aliveTimer, &QTimer::timeout, this, [this](){
        qCDebug(dcEverest()) << "No MQTT traffic since" << m_aliveTimer.interval()
        << "ms. Mark device as not connected" << m_thing;
        m_thing->setStateValue(everestMqttCurrentPowerStateTypeId, 0);
        m_thing->setStateValue(everestMqttConnectedStateTypeId, false);
    });

    if (m_client->isConnected()) {
        qCDebug(dcEverest()) << "The connection is already available. Initializing the instance...";
        initialize();
    }
}

EverestMqtt::~EverestMqtt()
{
    deinitialize();
}

Thing *EverestMqtt::thing() const
{
    return m_thing;
}

QString EverestMqtt::connector() const
{
    return m_connector;
}

void EverestMqtt::initialize()
{
    qCDebug(dcEverest()) << "Initializing" << m_thing->name();
    if (!m_client->isConnected()) {
        qCWarning(dcEverest()) << "Cannot initialize because the MQTT client is not connected for" << m_thing;
        m_initialized = false;
        return;
    }

    foreach (const QString &topic, m_subscribedTopics)
        m_client->subscribe(topic);

    m_initialized = true;
    qCDebug(dcEverest()) << "Initialized" << m_thing->name() << "successfully";
}

void EverestMqtt::deinitialize()
{
    qCDebug(dcEverest()) << "Deinitializing" << m_thing->name();
    if (m_initialized && m_client->isConnected()) {
        foreach (const QString &topic, m_subscribedTopics) {
            m_client->unsubscribe(topic);
        }
    }

    m_initialized = false;
}

void EverestMqtt::enableCharging(bool enable)
{
    QString topic;
    if (enable) {
        topic = m_topicPrefix + "/cmd/resume_charging";
    } else {
        topic = m_topicPrefix + "/cmd/pause_charging";
    }

    m_client->publish(topic, QByteArray::fromHex("01"));
}

void EverestMqtt::setMaxChargingCurrent(double current)
{
    QString topic = m_topicPrefix + "/cmd/set_limit_amps";
    QByteArray payload = QByteArray::number(current);

    m_client->publish(topic, payload);
}

void EverestMqtt::setMaxChargingCurrentAndPhaseCount(uint phasesCount, double current)
{
    QString topic = m_topicPrefix + "/cmd/set_limit_amps_phases";
    QVariantMap data;
    data.insert("amps", current);
    data.insert("phases", phasesCount);

    m_client->publish(topic, QJsonDocument::fromVariant(data).toJson());
}

void EverestMqtt::onConnected()
{
    m_aliveTimer.start();
    initialize();
}

void EverestMqtt::onDisconnected()
{
    m_thing->setStateValue(everestMqttConnectedStateTypeId, false);
    m_thing->setStateValue(everestMqttCurrentPowerStateTypeId, 0);
    m_initialized = false;
}

void EverestMqtt::onSubscribed(const QString &topic, Mqtt::SubscribeReturnCode subscribeReturnCode)
{
    if (subscribeReturnCode == Mqtt::SubscribeReturnCodeFailure) {
        qCWarning(dcEverest()) << "Failed to subscribe to" << topic << m_thing;
    } else {
        qCDebug(dcEverestTraffic()) << "Subscribed to" << topic << m_thing;
    }
}

void EverestMqtt::onPublishReceived(const QString &topic, const QByteArray &payload, bool retained)
{
    Q_UNUSED(retained)

    // Make sure this publish is for this everest instance
    if (!topic.startsWith(m_topicPrefix))
        return;

    // We are for sure connected now
    m_aliveTimer.start();
    m_thing->setStateValue(everestMqttConnectedStateTypeId, true);

    qCDebug(dcEverestTraffic()) << "Received publish on" << topic << qUtf8Printable(payload);

    // During boot the API module publishes empty payloads
    if (payload.isEmpty())
        return;

    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &jsonError);
    if (jsonError.error) {
        qCWarning(dcEverestTraffic()) << "Unable read JSON data published on topic"
                                      << topic << jsonError.errorString() << payload;
        return;
    }

    if (topic.endsWith("hardware_capabilities")) {
        /*
            {
                "max_phase_count_import": 3,
                "min_phase_count_import": 1,
                "max_phase_count_export": 3,
                "min_phase_count_export": 1,
                "supports_changing_phases_during_charging": true,
                "max_current_A_import": 32,
                "min_current_A_import": 6,
                "max_current_A_export": 16,
                "min_current_A_export": 0,
                "connector_type": "IEC62196Type2Cable"
            }
        */

        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        uint maxCurrent = dataMap.value("max_current_A_import").toUInt();
        uint minCurrent = dataMap.value("min_current_A_import").toUInt();

        m_thing->setStateMaxValue(everestMqttMaxChargingCurrentStateTypeId, maxCurrent);
        m_thing->setStateMinValue(everestMqttMaxChargingCurrentStateTypeId, minCurrent == 0 ? 6 : minCurrent);

        bool phaseSwitchingAvailable = dataMap.value("supports_changing_phases_during_charging", false).toBool();
        if (!phaseSwitchingAvailable) {
            // Only option left is set the desired phase count to 3, force that value
            m_thing->setStatePossibleValues(everestMqttDesiredPhaseCountStateTypeId, { 3 });
            m_thing->setStateValue(everestMqttDesiredPhaseCountStateTypeId, 3);
            m_thing->setStateValue(everestMqttPhaseCountStateTypeId, 3);
        } else {
            m_thing->setStatePossibleValues(everestMqttDesiredPhaseCountStateTypeId, { 1, 3 });
            m_thing->setStateValue(everestMqttPhaseCountStateTypeId, m_thing->stateValue(everestMqttDesiredPhaseCountStateTypeId));
        }

    } else if (topic.endsWith("limits")) {
        /*
            {
                "uuid": "connector_1",
                "nr_of_phases_available": 3,
                "max_current": 32
            }
        */
        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        //m_thing->setStateValue(everestPhaseCountStateTypeId, dataMap.value("nr_of_phases_available").toUInt());
        double maxCurrent = dataMap.value("max_current").toDouble();
        if (maxCurrent >= 6) {
            // FIXME: make it a double again once supported from the interface
            m_thing->setStateValue(everestMqttMaxChargingCurrentStateTypeId, qRound(maxCurrent));
        }

    } else if (topic.endsWith("powermeter")) {
        /*
            {
                "timestamp": "2024-03-19T10:40:03.427Z",
                "meter_id": "YETI_POWERMETER",
                "phase_seq_error": false,
                "energy_Wh_import": {
                    "total": 0, "L1": 0, "L2": 0, "L3": 0
                },
                "power_W": {
                    "total": 0, "L1": 0, "L2": 0, "L3": 0
                },
                "voltage_V": {
                    "L1": 230.55, "L2": 230.55, "L3": 230.55
                },
                "current_A": {
                    "L1": 0, "L2": 0, "L3": 0, "N": 0
                },
                "frequency_Hz": {
                    "L1": 49.97, "L2": 49.97, "L3": 49.97
                }
            }
        */
        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        QVariantMap energyImportedMap = dataMap.value("energy_Wh_import").toMap();
        m_thing->setStateValue(everestMqttTotalEnergyConsumedStateTypeId,
                               energyImportedMap.value("total").toDouble() / 1000.0);
        QVariantMap powerMap = dataMap.value("power_W").toMap();
        m_thing->setStateValue(everestMqttCurrentPowerStateTypeId, powerMap.value("total").toUInt());

    } else if (topic.endsWith("session_info")) {
        /*
            {
                "active_errors": [],
                "active_permanent_faults": [],
                "charged_energy_wh": 0,
                "charging_duration_s": 0,
                "datetime": "2024-03-19T10:40:02.986Z",
                "discharged_energy_wh": 0,
                "latest_total_w": 0,
                "state": "Unplugged"
            }
        */

        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        m_thing->setStateValue(everestMqttSessionEnergyStateTypeId,
                               dataMap.value("charged_energy_wh").toDouble() / 1000.0);

        // Interpret state
        QString stateString = dataMap.value("state").toString();
        m_thing->setStateValue(everestMqttStateStateTypeId, stateString);

        State state = convertStringToState(stateString);
        if (state == StateUnknown)
            return; // No need to proceed here

        m_thing->setStateValue(everestMqttChargingStateTypeId, state == StateCharging);
        m_thing->setStateValue(everestMqttPluggedInStateTypeId, state != StateUnplugged);

        // TODO: check if we can set the power state in other EVSE states
        if (state == StateCharging && !m_thing->stateValue(everestMqttPowerStateTypeId).toBool()) {
            m_thing->setStateValue(everestMqttPowerStateTypeId, true);
        }

    } else if (topic.endsWith("telemetry")) {
        /*
            {
                "phase_seq_error": false,
                "temperature": 25,
                "fan_rpm": 1500,
                "supply_voltage_12V": 12.01,
                "supply_voltage_minus_12V": -11.8
            }
        */

        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        m_thing->setStateValue(everestMqttTemperatureStateTypeId, dataMap.value("temperature").toDouble());
        m_thing->setStateValue(everestMqttFanSpeedStateTypeId, dataMap.value("fan_rpm").toDouble());
    }
}

QString EverestMqtt::buildTopic(const QString &topic)
{
    Q_ASSERT_X(!m_connector.isEmpty(), "EverestMqtt::buildTopic", "The connector must be known before building a topic");

    QString baseTopic = QString(m_topicPrefix + "/var");
    if (!topic.startsWith("/"))
        baseTopic.append("/");

    return baseTopic + topic;
}

EverestMqtt::State EverestMqtt::convertStringToState(const QString &stateString)
{
    State state = StateUnknown;

    if (stateString == "Unplugged") {
        state = StateUnplugged;
    } else if (stateString == "Disabled") {
        state = StateDisabled;
    } else if (stateString == "Preparing") {
        state = StatePreparing;
    } else if (stateString == "Reserved") {
        state = StateReserved;
    } else if (stateString == "AuthRequired") {
        state = StateAuthRequired;
    } else if (stateString == "WaitingForEnergy") {
        state = StateWaitingForEnergy;
    } else if (stateString == "Charging") {
        state = StateCharging;
    } else if (stateString == "ChargingPausedEV") {
        state = StateChargingPausedEV;
    } else if (stateString == "ChargingPausedEVSE") {
        state = StateChargingPausedEVSE;
    } else if (stateString == "Finished") {
        state = StateFinished;
    }

    return state;
}
