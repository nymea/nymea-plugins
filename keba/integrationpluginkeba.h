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

#ifndef INTEGRATIONPLUGINKEBA_H
#define INTEGRATIONPLUGINKEBA_H

#include <plugintimer.h>
#include <integrations/integrationplugin.h>
#include <network/networkdevicediscovery.h>

#include "kecontact.h"
#include "kebadiscovery.h"
#include "kecontactdatalayer.h"

#include <QHash>
#include <QUdpSocket>
#include <QDateTime>
#include <QUdpSocket>

#include "extern-plugininfo.h"

class IntegrationPluginKeba : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginkeba.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginKeba();

    void init() override;

    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;

    void postSetupThing(Thing* thing) override;
    void thingRemoved(Thing* thing) override;

    void executeAction(ThingActionInfo *info) override;

private:
    PluginTimer *m_updateTimer = nullptr;
    KeContactDataLayer *m_kebaDataLayer = nullptr;

    QHash<ThingId, KeContact *> m_kebaDevices;
    QHash<Thing *, NetworkDeviceMonitor *> m_monitors;
    QHash<ThingId, int> m_lastSessionId;
    QHash<QUuid, ThingActionInfo *> m_asyncActions;
    KebaDiscovery *m_runningDiscovery = nullptr;

    QHash<ThingClassId, ParamTypeId> m_macAddressParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_modelParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_serialNumberParamTypeIds;

    void setupKeba(ThingSetupInfo *info, const QHostAddress &address);

    void setDeviceState(Thing *device, KeContact::State state);
    void setDevicePlugState(Thing *device, KeContact::PlugState plugState);

    void refresh(Thing *thing, KeContact *keba);

private slots:
    void onCommandExecuted(QUuid requestId, bool success);
    void onReportTwoReceived(const KeContact::ReportTwo &reportTwo);
    void onReportThreeReceived(const KeContact::ReportThree &reportThree);
    void onReport1XXReceived(int reportNumber, const KeContact::Report1XX &report);
    void onBroadcastReceived(KeContact::BroadcastType type, const QVariant &content);
};

#endif // INTEGRATIONPLUGINKEBA_H
