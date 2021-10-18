/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
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

#ifndef INTEGRATIONPLUGINGARADGET_H
#define INTEGRATIONPLUGINGARADGET_H

#include "integrations/integrationplugin.h"

class MqttChannel;

class IntegrationPluginGaradget: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginGaradget.json")
    Q_INTERFACES(IntegrationPlugin)


public:
    explicit IntegrationPluginGaradget();
    ~IntegrationPluginGaradget();

    void init() override;
    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

private slots:
    void onClientConnected(MqttChannel *channel);
    void onClientDisconnected(MqttChannel *channel);
    void onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload);

private:
    QHash<Thing*, MqttChannel*> m_mqttChannels;

    // Helpers for parent devices (the ones starting with sonoff)
    QHash<ThingClassId, ParamTypeId> m_ipAddressParamTypeMap;
    QHash<ThingClassId, QList<ParamTypeId> > m_attachedDeviceParamTypeIdMap;
    
    // helpers from bee
    QHash<ThingClassId, ParamTypeId> m_stateParamTypeMap;
    QHash<ThingClassId, ParamTypeId> m_movingParamTypeMap;
    QHash<ThingClassId, ParamTypeId> m_percentageParamTypeMap;
    QHash<ThingClassId, ParamTypeId> m_closingOutputParamTypeMap;
    QHash<ThingClassId, ParamTypeId> m_openingOutputParamTypeMap;
    
    QHash<ThingClassId, StateTypeId> m_stateStateTypeMap;
    QHash<ThingClassId, StateTypeId> m_movingStateTypeMap;
    QHash<ThingClassId, StateTypeId> m_percentageStateTypeMap;
    QHash<ThingClassId, StateTypeId> m_closingOutputStateTypeMap;
    QHash<ThingClassId, StateTypeId> m_openingOutputStateTypeMap;

    // Helpers for child devices (virtual ones, starting with Garadget)
    QHash<ThingClassId, ParamTypeId> m_channelParamTypeMap;
    QHash<ThingClassId, ParamTypeId> m_openingChannelParamTypeMap;
    QHash<ThingClassId, ParamTypeId> m_closingChannelParamTypeMap;
    QHash<ThingClassId, StateTypeId> m_powerStateTypeMap;

    QHash<ThingClassId, ActionTypeId> m_closableOpenActionTypeMap;
    QHash<ThingClassId, ActionTypeId> m_closableCloseActionTypeMap;
    QHash<ThingClassId, ActionTypeId> m_closableStopActionTypeMap;

    // Helpers for both devices
    QHash<ThingClassId, StateTypeId> m_connectedStateTypeMap;
    QHash<ThingClassId, StateTypeId> m_signalStrengthStateTypeMap;

    QHash<ThingClassId, StateTypeId> m_brightnessStateTypeMap;
};

#endif // INTEGRATIONPLUGINGARADGET_H
