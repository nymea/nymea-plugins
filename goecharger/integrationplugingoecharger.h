/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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

#ifndef INTEGRATIONPLUGINGOECHARGER_H
#define INTEGRATIONPLUGINGOECHARGER_H

#include <QUuid>

#include <plugintimer.h>
#include <network/mqtt/mqttchannel.h>
#include <network/mqtt/mqttprovider.h>
#include <network/networkaccessmanager.h>
#include <network/networkdevicemonitor.h>
#include <integrations/integrationplugin.h>

class IntegrationPluginGoECharger: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugingoecharger.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    enum ApiVersion {
        ApiVersion1 = 1,
        ApiVersion2 = 2
    };
    Q_ENUM(ApiVersion)

    enum CarState {
        CarStateUnknown = 0,
        CarStateReadyNoCar = 1,
        CarStateCharging = 2,
        CarStateWaitForCar = 3,
        CarStateChargedCarConnected = 4
    };
    Q_ENUM(CarState)

    enum ForceState {
        ForceStateNeutral = 0,
        ForceStateOff = 1,
        ForceStateOn = 2
    };
    Q_ENUM(ForceState)

    enum Access {
        AccessOpen = 0,
        AccessRfid = 1,
        AccessAuto = 2
    };
    Q_ENUM(Access)

    enum ErrorCode {
        ErrorCodeResidualCurrentCircuitBreaker = 1,
        ErrorCodePhase = 3,
        ErrorCodeNoGround = 8,
        ErrorCodeInternalError = 10
    };
    Q_ENUM(ErrorCode)

    enum CableLockMode {
        CableLockModeLockWhileCareConnected = 0,
        CableLockModeUnlockAfterCharging = 1,
        CableLockModeAlwaysLock = 2
    };
    Q_ENUM(CableLockMode)

    explicit IntegrationPluginGoECharger();

    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;

    void executeAction(ThingActionInfo *info) override;

private:
    PluginTimer *m_refreshTimer = nullptr;

    QHash<Thing *, MqttChannel *> m_mqttChannelsV1;
    QHash<Thing *, MqttChannel *> m_mqttChannelsV2;

    QHash<Thing *, QNetworkReply *> m_pendingReplies;
    QHash<Thing *, NetworkDeviceMonitor *> m_monitors;

    // General methods
    void setupGoeHome(ThingSetupInfo *info);
    QNetworkRequest buildStatusRequest(Thing *thing);
    QHostAddress getHostAddress(Thing *thing);
    ApiVersion getApiVersion(Thing *thing);

    // API V1
    void updateV1(Thing *thing, const QVariantMap &statusMap);
    QNetworkRequest buildConfigurationRequestV1(const QHostAddress &address, const QString &configuration);
    void sendActionRequestV1(Thing *thing, ThingActionInfo *info, const QString &configuration, const QVariant &value);
    void setupMqttChannelV1(ThingSetupInfo *info, const QHostAddress &address, const QVariantMap &statusMap);
    void reconfigureMqttChannelV1(Thing *thing, const QVariantMap &statusMap);

    // API V2
    void updateV2(Thing *thing, const QVariantMap &statusMap);
    QNetworkRequest buildConfigurationRequestV2(const QHostAddress &address, const QUrlQuery &configuration);
    void setupMqttChannelV2(ThingSetupInfo *info, const QHostAddress &address, const QVariantMap &statusMap);
    void reconfigureMqttChannelV2(Thing *thing);

private slots:
    void refreshHttp();

    // API V1
    void onMqttClientV1Connected(MqttChannel* channel);
    void onMqttClientV1Disconnected(MqttChannel* channel);
    void onMqttPublishV1Received(MqttChannel* channel, const QString &topic, const QByteArray &payload);

    // API V2
    void onMqttClientV2Connected(MqttChannel* channel);
    void onMqttClientV2Disconnected(MqttChannel* channel);

};

#endif // INTEGRATIONPLUGINGOECHARGER_H

