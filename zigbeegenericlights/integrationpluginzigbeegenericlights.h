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

#ifndef INTEGRATIONPLUGINZIGBEEGENERICLIGHTS_H
#define INTEGRATIONPLUGINZIGBEEGENERICLIGHTS_H

#include "integrations/integrationplugin.h"
#include "hardware/zigbee/zigbeehandler.h"

class IntegrationPluginZigbeeGenericLights: public IntegrationPlugin, public ZigbeeHandler
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginzigbeegenericlights.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginZigbeeGenericLights();

    QString name() const override;
    bool handleNode(ZigbeeNode *node, const QUuid &networkUuid) override;
    void handleRemoveNode(ZigbeeNode *node, const QUuid &networkUuid) override;

    void init() override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing *thing) override;

private:
    // Common thing params
    QHash<ThingClassId, ParamTypeId> m_ieeeAddressParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_networkUuidParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_endpointIdParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_modelIdParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_manufacturerIdParamTypeIds;

    // Common states
    QHash<ThingClassId, StateTypeId> m_connectedStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_signalStrengthStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_versionStateTypeIds;

    QHash<Thing *, ZigbeeNode *> m_thingNodes;
    QHash<Thing *, ThingActionInfo *> m_pendingBrightnessActions;

    // Get the endpoint for the given thing
    ZigbeeNodeEndpoint *findEndpoint(Thing *thing);

    void createLightThing(const ThingClassId &thingClassId, const QUuid &networkUuid, ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint);

    // Action help methods since they work all the same
    void executeAlertAction(ThingActionInfo *info, ZigbeeNodeEndpoint *endpoint);
    void executePowerAction(ThingActionInfo *info, ZigbeeNodeEndpoint *endpoint, const StateTypeId &powerStateTypeId, bool power);
    void executeBrightnessAction(ThingActionInfo *info, ZigbeeNodeEndpoint *endpoint, const StateTypeId &powerStateTypeId, const StateTypeId &brightnessStateTypeId, int brightness, quint8 level);
    void executeColorTemperatureAction(ThingActionInfo *info, ZigbeeNodeEndpoint *endpoint, const StateTypeId &colorTemperatureStateTypeId, int colorTemperatureScaled);
    void executeColorAction(ThingActionInfo *info, ZigbeeNodeEndpoint *endpoint, const StateTypeId &colorStateTypeId, const QColor &color);

    // Read state values from the node
    void readLightPowerState(Thing *thing);
    void readLightLevelState(Thing *thing);
    void readLightColorTemperatureState(Thing *thing);
    void readLightColorXYState(Thing *thing);

    // Configure reporting
    void configureLightPowerReporting(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint);
    void configureLightBrightnessReporting(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint);


    // Color temperature information handling
    typedef struct ColorTemperatureRange {
        quint16 minValue = 250;
        quint16 maxValue = 450;
    } ColorTemperatureRange;

    // Color temperature scaling range defined in
    // all color temperature states/actions (the slider min/max)
    int m_minScaleValue = 0;
    int m_maxScaleValue = 200;

    QHash<Thing *, ColorTemperatureRange> m_colorTemperatureRanges;
    QHash<Thing *, ZigbeeClusterColorControl::ColorCapabilities> m_colorCapabilities;

    void readColorCapabilities(Thing *thing);
    void processColorCapabilities(Thing *thing);

    void readColorTemperatureRange(Thing *thing);
    bool readCachedColorTemperatureRange(Thing *thing, ZigbeeClusterColorControl *colorCluster);
    quint16 mapScaledValueToColorTemperature(Thing *thing, int scaledColorTemperature);
    int mapColorTemperatureToScaledValue(Thing *thing, quint16 colorTemperature);

};

#endif // INTEGRATIONPLUGINZIGBEEGENERICLIGHTS_H
