/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
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

#include <network/networkaccessmanager.h>
#include <network/mqtt/mqttchannel.h>
#include <network/mqtt/mqttprovider.h>
#include <network/networkdevicemonitor.h>
#include <integrations/integrationplugin.h>
#include <plugintimer.h>

#include "extern-plugininfo.h"

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
    QHash<Thing *, MqttChannel *> m_channels;
    QHash<Thing *, NetworkDeviceMonitor *> m_monitors;
    QHash<Thing *, QNetworkReply *> m_pendingReplies;

    void update(Thing *thing, const QVariantMap &statusMap);

    void setupGoeHome(ThingSetupInfo *info);

    QNetworkRequest buildStatusRequest(Thing *thing);
    QNetworkRequest buildConfigurationRequest(Thing *thing, const QUrlQuery &configuration);

    void executeEnableCharging(ThingActionInfo *info, bool enabled);
    void executeSetChargingCurrent(ThingActionInfo *info, uint ampere);
    void executeSetMaxAllowedCurrent(ThingActionInfo *info, uint ampere);

    void sendActionRequest(Thing *thing, ThingActionInfo *info, const QString &configuration, ApiVersion apiVersion = ApiVersion2);
    QNetworkReply *sendActionReply(Thing *thing, const QString &configuration);

    void setupMqttChannel(ThingSetupInfo *info, const QVariantMap &statusMap);
    void reconfigureMqttChannel(Thing *thing, const QVariantMap &statusMap);

    // Helpers
    QHostAddress getHostAddress(Thing *thing);
    ApiVersion getApiVersion(Thing *thing);


private slots:
    void refreshHttp();
    void refreshHttpThing(Thing *thing);

    void onClientConnected(MqttChannel* channel);
    void onClientDisconnected(MqttChannel* channel);

};

#endif // INTEGRATIONPLUGINGOECHARGER_H

