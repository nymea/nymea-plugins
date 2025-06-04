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
