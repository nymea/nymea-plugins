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

#ifndef EVERESTMQTT_H
#define EVERESTMQTT_H

#include <QTimer>
#include <QObject>

#include <mqttclient.h>

#include <integrations/thing.h>

class EverestMqtt : public QObject
{
    Q_OBJECT
public:

    enum State {
        StateUnplugged,
        StateDisabled,
        StatePreparing,
        StateReserved,
        StateAuthRequired,
        StateWaitingForEnergy,
        StateCharging,
        StateChargingPausedEV,
        StateChargingPausedEVSE,
        StateFinished,
        StateUnknown
    };
    Q_ENUM(State)

    explicit EverestMqtt(MqttClient *client, Thing *thing, QObject *parent = nullptr);
    ~EverestMqtt();

    Thing *thing() const;

    QString connector() const;

    void initialize();
    void deinitialize();

    void enableCharging(bool enable);

    // Use only without phase switching
    void setMaxChargingCurrent(double current);

    // Use also for setting the charging current if phase switching is available
    void setMaxChargingCurrentAndPhaseCount(uint phasesCount, double current);

private slots:
    void onConnected();
    void onDisconnected();
    void onSubscribed(const QString &topic, Mqtt::SubscribeReturnCode subscribeReturnCode);
    void onPublishReceived(const QString &topic, const QByteArray &payload, bool retained);

private:
    MqttClient *m_client = nullptr;
    Thing *m_thing = nullptr;
    QTimer m_aliveTimer;

    QString m_connector;
    QString m_topicPrefix;
    QStringList m_subscribedTopics;

    bool m_initialized = false;

    // Prepends everest_api/<connector>/ to the given topic
    QString buildTopic(const QString &topic);
    State convertStringToState(const QString &stateString);
};

#endif // EVERESTMQTT_H
