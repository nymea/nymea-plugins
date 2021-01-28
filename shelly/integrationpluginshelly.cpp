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

#include "integrationpluginshelly.h"
#include "plugininfo.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QHostAddress>
#include <QJsonDocument>
#include <QColor>

#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"
#include "network/mqtt/mqttprovider.h"
#include "network/mqtt/mqttchannel.h"

#include "plugintimer.h"

#include "network/zeroconf/zeroconfservicebrowser.h"
#include "platform/platformzeroconfcontroller.h"

// Maps update status strings: Shelly <-> nymea
static QHash<QString, QString> updateStatusMap = {
    {"idle", "idle"},
    {"pending", "available"},
    {"updating", "updating"},
    {"unknown", "idle"}
};

IntegrationPluginShelly::IntegrationPluginShelly()
{
    // Device param types
    m_idParamTypeMap[shelly1ThingClassId] = shelly1ThingIdParamTypeId;
    m_idParamTypeMap[shelly1pmThingClassId] = shelly1pmThingIdParamTypeId;
    m_idParamTypeMap[shelly1lThingClassId] = shelly1lThingIdParamTypeId;
    m_idParamTypeMap[shellyPlugThingClassId] = shellyPlugThingIdParamTypeId;
    m_idParamTypeMap[shellyRgbw2ThingClassId] = shellyRgbw2ThingIdParamTypeId;
    m_idParamTypeMap[shellyDimmerThingClassId] = shellyDimmerThingIdParamTypeId;
    m_idParamTypeMap[shelly2ThingClassId] = shelly2ThingIdParamTypeId;
    m_idParamTypeMap[shelly25ThingClassId] = shelly25ThingIdParamTypeId;
    m_idParamTypeMap[shellyButton1ThingClassId] = shellyButton1ThingIdParamTypeId;
    m_idParamTypeMap[shellyEm3ThingClassId] = shellyEm3ThingIdParamTypeId;
    m_idParamTypeMap[shellyHTThingClassId] = shellyHTThingIdParamTypeId;

    m_usernameParamTypeMap[shelly1ThingClassId] = shelly1ThingUsernameParamTypeId;
    m_usernameParamTypeMap[shelly1pmThingClassId] = shelly1pmThingUsernameParamTypeId;
    m_usernameParamTypeMap[shelly1lThingClassId] = shelly1lThingUsernameParamTypeId;
    m_usernameParamTypeMap[shellyPlugThingClassId] = shellyPlugThingUsernameParamTypeId;
    m_usernameParamTypeMap[shellyRgbw2ThingClassId] = shellyRgbw2ThingUsernameParamTypeId;
    m_usernameParamTypeMap[shellyDimmerThingClassId] = shellyDimmerThingUsernameParamTypeId;
    m_usernameParamTypeMap[shelly2ThingClassId] = shelly2ThingUsernameParamTypeId;
    m_usernameParamTypeMap[shelly25ThingClassId] = shelly25ThingUsernameParamTypeId;
    m_usernameParamTypeMap[shellyButton1ThingClassId] = shellyButton1ThingUsernameParamTypeId;
    m_usernameParamTypeMap[shellyEm3ThingClassId] = shellyEm3ThingUsernameParamTypeId;
    m_usernameParamTypeMap[shellyHTThingClassId] = shellyHTThingUsernameParamTypeId;

    m_passwordParamTypeMap[shelly1ThingClassId] = shelly1ThingPasswordParamTypeId;
    m_passwordParamTypeMap[shelly1pmThingClassId] = shelly1pmThingPasswordParamTypeId;
    m_passwordParamTypeMap[shelly1lThingClassId] = shelly1lThingPasswordParamTypeId;
    m_passwordParamTypeMap[shellyPlugThingClassId] = shellyPlugThingPasswordParamTypeId;
    m_passwordParamTypeMap[shellyRgbw2ThingClassId] = shellyRgbw2ThingPasswordParamTypeId;
    m_passwordParamTypeMap[shellyDimmerThingClassId] = shellyDimmerThingPasswordParamTypeId;
    m_passwordParamTypeMap[shelly2ThingClassId] = shelly2ThingPasswordParamTypeId;
    m_passwordParamTypeMap[shelly25ThingClassId] = shelly25ThingPasswordParamTypeId;
    m_passwordParamTypeMap[shellyButton1ThingClassId] = shellyButton1ThingPasswordParamTypeId;
    m_passwordParamTypeMap[shellyEm3ThingClassId] = shellyEm3ThingPasswordParamTypeId;
    m_passwordParamTypeMap[shellyHTThingClassId] = shellyHTThingPasswordParamTypeId;

    m_connectedDeviceParamTypeMap[shelly2ThingClassId] = shelly2ThingConnectedDevice1ParamTypeId;
    m_connectedDeviceParamTypeMap[shelly25ThingClassId] = shelly25ThingConnectedDevice1ParamTypeId;

    m_connectedDevice2ParamTypeMap[shelly2ThingClassId] = shelly2ThingConnectedDevice2ParamTypeId;
    m_connectedDevice2ParamTypeMap[shelly25ThingClassId] = shelly25ThingConnectedDevice2ParamTypeId;

    m_channelParamTypeMap[shellyGenericThingClassId] = shellyGenericThingChannelParamTypeId;
    m_channelParamTypeMap[shellyLightThingClassId] = shellyLightThingChannelParamTypeId;
    m_channelParamTypeMap[shellySocketThingClassId] = shellySocketThingChannelParamTypeId;
    m_channelParamTypeMap[shellyGenericPMThingClassId] = shellyGenericPMThingChannelParamTypeId;
    m_channelParamTypeMap[shellyLightPMThingClassId] = shellyLightPMThingChannelParamTypeId;
    m_channelParamTypeMap[shellySocketPMThingClassId] = shellySocketPMThingChannelParamTypeId;
    m_channelParamTypeMap[shellyRollerThingClassId] = shellyRollerThingChannelParamTypeId;

    // States
    m_connectedStateTypesMap[shelly1ThingClassId] = shelly1ConnectedStateTypeId;
    m_connectedStateTypesMap[shelly1pmThingClassId] = shelly1pmConnectedStateTypeId;
    m_connectedStateTypesMap[shelly1lThingClassId] = shelly1lConnectedStateTypeId;
    m_connectedStateTypesMap[shelly2ThingClassId] = shelly2ConnectedStateTypeId;
    m_connectedStateTypesMap[shelly25ThingClassId] = shelly25ConnectedStateTypeId;
    m_connectedStateTypesMap[shellyPlugThingClassId] = shellyPlugConnectedStateTypeId;
    m_connectedStateTypesMap[shellyRgbw2ThingClassId] = shellyRgbw2ConnectedStateTypeId;
    m_connectedStateTypesMap[shellyDimmerThingClassId] = shellyDimmerConnectedStateTypeId;
    m_connectedStateTypesMap[shellyButton1ThingClassId] = shellyButton1ConnectedStateTypeId;
    m_connectedStateTypesMap[shellyEm3ThingClassId] = shellyEm3ConnectedStateTypeId;
    m_connectedStateTypesMap[shellyHTThingClassId] = shellyHTConnectedStateTypeId;
    m_connectedStateTypesMap[shellySwitchThingClassId] = shellySwitchConnectedStateTypeId;
    m_connectedStateTypesMap[shellyGenericThingClassId] = shellyGenericConnectedStateTypeId;
    m_connectedStateTypesMap[shellyLightThingClassId] = shellyLightConnectedStateTypeId;
    m_connectedStateTypesMap[shellySocketThingClassId] = shellySocketConnectedStateTypeId;
    m_connectedStateTypesMap[shellyGenericPMThingClassId] = shellyGenericPMConnectedStateTypeId;
    m_connectedStateTypesMap[shellyLightPMThingClassId] = shellyLightPMConnectedStateTypeId;
    m_connectedStateTypesMap[shellySocketPMThingClassId] = shellySocketPMConnectedStateTypeId;
    m_connectedStateTypesMap[shellyRollerThingClassId] = shellyRollerConnectedStateTypeId;

    m_signalStrengthStateTypesMap[shelly1ThingClassId] = shelly1SignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shelly1pmThingClassId] = shelly1pmSignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shelly1lThingClassId] = shelly1lSignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shelly2ThingClassId] = shelly2SignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shelly25ThingClassId] = shelly25SignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellyPlugThingClassId] = shellyPlugSignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellyRgbw2ThingClassId] = shellyRgbw2SignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellyDimmerThingClassId] = shellyDimmerSignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellyButton1ThingClassId] = shellyButton1SignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellyEm3ThingClassId] = shellyEm3SignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellyHTThingClassId] = shellyHTSignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellySwitchThingClassId] = shellySwitchSignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellyGenericThingClassId] = shellyGenericSignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellyLightThingClassId] = shellyLightSignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellySocketThingClassId] = shellySocketSignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellyGenericPMThingClassId] = shellyGenericPMSignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellyLightPMThingClassId] = shellyLightPMSignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellySocketPMThingClassId] = shellySocketPMSignalStrengthStateTypeId;
    m_signalStrengthStateTypesMap[shellyRollerThingClassId] = shellyRollerSignalStrengthStateTypeId;

    m_powerStateTypeMap[shelly1ThingClassId] = shelly1PowerStateTypeId;
    m_powerStateTypeMap[shelly1pmThingClassId] = shelly1pmPowerStateTypeId;
    m_powerStateTypeMap[shelly1lThingClassId] = shelly1lPowerStateTypeId;
    m_powerStateTypeMap[shellyPlugThingClassId] = shellyPlugPowerStateTypeId;
    m_powerStateTypeMap[shellyRgbw2ThingClassId] = shellyRgbw2PowerStateTypeId;
    m_powerStateTypeMap[shellyDimmerThingClassId] = shellyDimmerPowerStateTypeId;
    m_powerStateTypeMap[shellyGenericThingClassId] = shellyGenericPowerStateTypeId;
    m_powerStateTypeMap[shellyLightThingClassId] = shellyLightPowerStateTypeId;
    m_powerStateTypeMap[shellySocketThingClassId] = shellySocketPowerStateTypeId;
    m_powerStateTypeMap[shellyGenericPMThingClassId] = shellyGenericPMPowerStateTypeId;
    m_powerStateTypeMap[shellyLightPMThingClassId] = shellyLightPMPowerStateTypeId;
    m_powerStateTypeMap[shellySocketPMThingClassId] = shellySocketPMPowerStateTypeId;
    m_powerStateTypeMap[shellyEm3ThingClassId] = shellyEm3PowerStateTypeId;

    m_currentPowerStateTypeMap[shelly1pmThingClassId] = shelly1pmCurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shelly1lThingClassId] = shelly1lCurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellyPlugThingClassId] = shellyPlugCurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellyRgbw2ThingClassId] = shellyRgbw2CurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellyDimmerThingClassId] = shellyDimmerCurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellyGenericPMThingClassId] = shellyGenericPMCurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellyLightPMThingClassId] = shellyLightPMCurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellySocketPMThingClassId] = shellySocketPMCurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellyRollerThingClassId] = shellyRollerCurrentPowerStateTypeId;
    m_currentPowerStateTypeMap[shellyEm3ThingClassId] = shellyEm3CurrentPowerStateTypeId;

    m_totalEnergyConsumedStateTypeMap[shellyPlugThingClassId] = shellyPlugTotalEnergyConsumedStateTypeId;
    m_totalEnergyConsumedStateTypeMap[shellyGenericPMThingClassId] = shellyGenericPMTotalEnergyConsumedStateTypeId;
    m_totalEnergyConsumedStateTypeMap[shellyLightPMThingClassId] = shellyLightPMTotalEnergyConsumedStateTypeId;
    m_totalEnergyConsumedStateTypeMap[shellySocketPMThingClassId] = shellySocketPMTotalEnergyConsumedStateTypeId;
    m_totalEnergyConsumedStateTypeMap[shellyRollerThingClassId] = shellyRollerTotalEnergyConsumedStateTypeId;
    m_totalEnergyConsumedStateTypeMap[shellyEm3ThingClassId] = shellyEm3TotalEnergyConsumedStateTypeId;

    m_colorStateTypeMap[shellyRgbw2ThingClassId] = shellyRgbw2ColorStateTypeId;
    m_colorTemperatureStateTypeMap[shellyRgbw2ThingClassId] = shellyRgbw2ColorTemperatureStateTypeId;

    m_brightnessStateTypeMap[shellyRgbw2ThingClassId] = shellyRgbw2BrightnessStateTypeId;
    m_brightnessStateTypeMap[shellyDimmerThingClassId] = shellyDimmerBrightnessStateTypeId;

    m_updateStatusStateTypesMap[shelly1ThingClassId] = shelly1UpdateStatusStateTypeId;
    m_updateStatusStateTypesMap[shelly1pmThingClassId] = shelly1pmUpdateStatusStateTypeId;
    m_updateStatusStateTypesMap[shelly1lThingClassId] = shelly1lUpdateStatusStateTypeId;
    m_updateStatusStateTypesMap[shelly2ThingClassId] = shelly2UpdateStatusStateTypeId;
    m_updateStatusStateTypesMap[shelly25ThingClassId] = shelly25UpdateStatusStateTypeId;
    m_updateStatusStateTypesMap[shellyPlugThingClassId] = shellyPlugUpdateStatusStateTypeId;
    m_updateStatusStateTypesMap[shellyRgbw2ThingClassId] = shellyRgbw2UpdateStatusStateTypeId;
    m_updateStatusStateTypesMap[shellyDimmerThingClassId] = shellyDimmerUpdateStatusStateTypeId;
    m_updateStatusStateTypesMap[shellyButton1ThingClassId] = shellyButton1UpdateStatusStateTypeId;
    m_updateStatusStateTypesMap[shellyEm3ThingClassId] = shellyEm3UpdateStatusStateTypeId;
    m_updateStatusStateTypesMap[shellyHTThingClassId] = shellyHTUpdateStatusStateTypeId;

    m_currentVersionStateTypesMap[shelly1ThingClassId] = shelly1CurrentVersionStateTypeId;
    m_currentVersionStateTypesMap[shelly1pmThingClassId] = shelly1pmCurrentVersionStateTypeId;
    m_currentVersionStateTypesMap[shelly1lThingClassId] = shelly1lCurrentVersionStateTypeId;
    m_currentVersionStateTypesMap[shelly2ThingClassId] = shelly2CurrentVersionStateTypeId;
    m_currentVersionStateTypesMap[shelly25ThingClassId] = shelly25CurrentVersionStateTypeId;
    m_currentVersionStateTypesMap[shellyPlugThingClassId] = shellyPlugCurrentVersionStateTypeId;
    m_currentVersionStateTypesMap[shellyRgbw2ThingClassId] = shellyRgbw2CurrentVersionStateTypeId;
    m_currentVersionStateTypesMap[shellyDimmerThingClassId] = shellyDimmerCurrentVersionStateTypeId;
    m_currentVersionStateTypesMap[shellyButton1ThingClassId] = shellyButton1CurrentVersionStateTypeId;
    m_currentVersionStateTypesMap[shellyEm3ThingClassId] = shellyEm3CurrentVersionStateTypeId;
    m_currentVersionStateTypesMap[shellyHTThingClassId] = shellyHTCurrentVersionStateTypeId;

    m_availableVersionStateTypesMap[shelly1ThingClassId] = shelly1AvailableVersionStateTypeId;
    m_availableVersionStateTypesMap[shelly1pmThingClassId] = shelly1pmAvailableVersionStateTypeId;
    m_availableVersionStateTypesMap[shelly1lThingClassId] = shelly1lAvailableVersionStateTypeId;
    m_availableVersionStateTypesMap[shelly2ThingClassId] = shelly2AvailableVersionStateTypeId;
    m_availableVersionStateTypesMap[shelly25ThingClassId] = shelly25AvailableVersionStateTypeId;
    m_availableVersionStateTypesMap[shellyPlugThingClassId] = shellyPlugAvailableVersionStateTypeId;
    m_availableVersionStateTypesMap[shellyRgbw2ThingClassId] = shellyRgbw2AvailableVersionStateTypeId;
    m_availableVersionStateTypesMap[shellyDimmerThingClassId] = shellyDimmerAvailableVersionStateTypeId;
    m_availableVersionStateTypesMap[shellyButton1ThingClassId] = shellyButton1AvailableVersionStateTypeId;
    m_availableVersionStateTypesMap[shellyEm3ThingClassId] = shellyEm3AvailableVersionStateTypeId;
    m_availableVersionStateTypesMap[shellyHTThingClassId] = shellyHTAvailableVersionStateTypeId;

    m_batteryLevelStateTypeMap[shellyButton1ThingClassId] = shellyButton1BatteryLevelStateTypeId;
    m_batteryLevelStateTypeMap[shellyHTThingClassId] = shellyHTBatteryLevelStateTypeId;

    m_batteryCriticalStateTypeMap[shellyButton1ThingClassId] = shellyButton1BatteryCriticalStateTypeId;
    m_batteryCriticalStateTypeMap[shellyHTThingClassId] = shellyHTBatteryCriticalStateTypeId;

    // Actions and their params
    m_rebootActionTypeMap[shelly1RebootActionTypeId] = shelly1ThingClassId;
    m_rebootActionTypeMap[shelly1pmRebootActionTypeId] = shelly1pmThingClassId;
    m_rebootActionTypeMap[shelly1lRebootActionTypeId] = shelly1lThingClassId;
    m_rebootActionTypeMap[shellyPlugRebootActionTypeId] = shellyPlugThingClassId;
    m_rebootActionTypeMap[shellyRgbw2RebootActionTypeId] = shellyRgbw2ThingClassId;
    m_rebootActionTypeMap[shellyDimmerRebootActionTypeId] = shellyDimmerThingClassId;
    m_rebootActionTypeMap[shelly2RebootActionTypeId] = shelly2ThingClassId;
    m_rebootActionTypeMap[shelly25RebootActionTypeId] = shelly25ThingClassId;

    m_powerActionTypesMap[shelly1PowerActionTypeId] = shelly1ThingClassId;
    m_powerActionTypesMap[shelly1pmPowerActionTypeId] = shelly1pmThingClassId;
    m_powerActionTypesMap[shelly1lPowerActionTypeId] = shelly1lThingClassId;
    m_powerActionTypesMap[shellyPlugPowerActionTypeId] = shellyPlugThingClassId;
    m_powerActionTypesMap[shellyGenericPowerActionTypeId] = shellyGenericThingClassId;
    m_powerActionTypesMap[shellyLightPowerActionTypeId] = shellyLightThingClassId;
    m_powerActionTypesMap[shellySocketPowerActionTypeId] = shellySocketThingClassId;
    m_powerActionTypesMap[shellyGenericPMPowerActionTypeId] = shellyGenericPMThingClassId;
    m_powerActionTypesMap[shellyLightPMPowerActionTypeId] = shellyLightPMThingClassId;
    m_powerActionTypesMap[shellySocketPMPowerActionTypeId] = shellySocketPMThingClassId;
    m_powerActionTypesMap[shellyEm3PowerActionTypeId] = shellyEm3ThingClassId;

    m_powerActionParamTypesMap[shelly1PowerActionTypeId] = shelly1PowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shelly1pmPowerActionTypeId] = shelly1pmPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shelly1lPowerActionTypeId] = shelly1lPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellyPlugPowerActionTypeId] = shellyPlugPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellyGenericPowerActionTypeId] = shellyGenericPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellyLightPowerActionTypeId] = shellyLightPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellySocketPowerActionTypeId] = shellySocketPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellyGenericPMPowerActionTypeId] = shellyGenericPMPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellyLightPMPowerActionTypeId] = shellyLightPMPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellySocketPMPowerActionTypeId] = shellySocketPMPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellyEm3PowerActionTypeId] = shellyEm3PowerActionPowerParamTypeId;

    m_colorPowerActionTypesMap[shellyRgbw2PowerActionTypeId] = shellyRgbw2ThingClassId;
    m_colorPowerActionParamTypesMap[shellyRgbw2PowerActionPowerParamTypeId] = shellyRgbw2PowerActionTypeId;

    m_colorActionTypesMap[shellyRgbw2ColorActionTypeId] = shellyRgbw2ThingClassId;
    m_colorActionParamTypesMap[shellyRgbw2ColorActionTypeId] = shellyRgbw2ColorActionTypeId;

    m_colorBrightnessActionTypesMap[shellyRgbw2BrightnessActionTypeId] = shellyRgbw2ThingClassId;
    m_colorBrightnessActionParamTypesMap[shellyRgbw2BrightnessActionBrightnessParamTypeId] = shellyRgbw2BrightnessActionTypeId;

    m_colorTemperatureActionTypesMap[shellyRgbw2ColorTemperatureActionTypeId] = shellyRgbw2ThingClassId;
    m_colorTemperatureActionParamTypesMap[shellyRgbw2ColorTemperatureActionTypeId] = shellyRgbw2ColorTemperatureActionColorTemperatureParamTypeId;

    m_dimmablePowerActionTypesMap[shellyDimmerPowerActionTypeId] = shellyDimmerThingClassId;
    m_dimmablePowerActionParamTypesMap[shellyDimmerPowerActionTypeId] = shellyDimmerPowerActionPowerParamTypeId;

    m_dimmableBrightnessActionTypesMap[shellyDimmerBrightnessActionTypeId] = shellyDimmerThingClassId;
    m_dimmableBrightnessActionParamTypesMap[shellyDimmerBrightnessActionTypeId] = shellyDimmerBrightnessActionBrightnessParamTypeId;

    m_rollerOpenActionTypeMap[shellyRollerOpenActionTypeId] = shellyRollerThingClassId;
    m_rollerCloseActionTypeMap[shellyRollerCloseActionTypeId] = shellyRollerThingClassId;
    m_rollerStopActionTypeMap[shellyRollerStopActionTypeId] = shellyRollerThingClassId;

    m_updateActionTypesMap[shelly1PerformUpdateActionTypeId] = shelly1ThingClassId;
    m_updateActionTypesMap[shelly1pmPerformUpdateActionTypeId] = shelly1pmThingClassId;
    m_updateActionTypesMap[shelly1lPerformUpdateActionTypeId] = shelly1lThingClassId;
    m_updateActionTypesMap[shelly2PerformUpdateActionTypeId] = shelly2ThingClassId;
    m_updateActionTypesMap[shelly25PerformUpdateActionTypeId] = shelly25ThingClassId;
    m_updateActionTypesMap[shellyPlugPerformUpdateActionTypeId] = shellyPlugThingClassId;
    m_updateActionTypesMap[shellyRgbw2PerformUpdateActionTypeId] = shellyRgbw2ThingClassId;
    m_updateActionTypesMap[shellyDimmerPerformUpdateActionTypeId] = shellyDimmerThingClassId;
    m_updateActionTypesMap[shellyButton1PerformUpdateActionTypeId] = shellyButton1ThingClassId;
    m_updateActionTypesMap[shellyEm3PerformUpdateActionTypeId] = shellyEm3ThingClassId;
    m_updateActionTypesMap[shellyHTPerformUpdateActionTypeId] = shellyHTThingClassId;
}

IntegrationPluginShelly::~IntegrationPluginShelly()
{
}

void IntegrationPluginShelly::init()
{
    m_zeroconfBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_http._tcp");
}

void IntegrationPluginShelly::discoverThings(ThingDiscoveryInfo *info)
{
    foreach (const ZeroConfServiceEntry &entry, m_zeroconfBrowser->serviceEntries()) {
//        qCDebug(dcShelly()) << "Have entry" << entry;
        QRegExp namePattern;
        if (info->thingClassId() == shelly1ThingClassId) {
            namePattern = QRegExp("^shelly1-[0-9A-Z]+$");
        } else if (info->thingClassId() == shelly1pmThingClassId) {
            namePattern = QRegExp("^shelly1pm-[0-9A-Z]+$");
        } else if (info->thingClassId() == shelly1lThingClassId) {
            namePattern = QRegExp("^shelly1l-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyPlugThingClassId) {
            namePattern = QRegExp("^shellyplug(-s)?-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyRgbw2ThingClassId) {
            namePattern = QRegExp("^shellyrgbw2-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyDimmerThingClassId) {
            namePattern = QRegExp("^shellydimmer(2)?-[0-9A-Z]+$");
        } else if (info->thingClassId() == shelly2ThingClassId) {
            namePattern = QRegExp("^shellyswitch-[0-9A-Z]+$");
        } else if (info->thingClassId() == shelly25ThingClassId) {
            namePattern = QRegExp("^shellyswitch25-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyButton1ThingClassId) {
            namePattern = QRegExp("^shellybutton1-[0-9-A-Z]+$");
        } else if (info->thingClassId() == shellyEm3ThingClassId) {
            namePattern = QRegExp("^shellyem3-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyHTThingClassId) {
            namePattern = QRegExp("shellyht-[0-9A-Z]+$");
        }
        if (!entry.name().contains(namePattern)) {
            continue;
        }

        qCDebug(dcShelly()) << "Found shelly thing!" << entry;

        ThingDescriptor descriptor(info->thingClassId(), entry.name(), entry.hostAddress().toString());
        ParamList params;
        params << Param(m_idParamTypeMap.value(info->thingClassId()), entry.name());
        params << Param(m_usernameParamTypeMap.value(info->thingClassId()), "");
        params << Param(m_passwordParamTypeMap.value(info->thingClassId()), "");
        if (m_connectedDeviceParamTypeMap.contains(info->thingClassId())) {
            params << Param(m_connectedDeviceParamTypeMap.value(info->thingClassId()), "None");
        }
        if (m_connectedDevice2ParamTypeMap.contains(info->thingClassId())) {
            params << Param(m_connectedDevice2ParamTypeMap.value(info->thingClassId()), "None");
        }
        descriptor.setParams(params);

        Things existingThings = myThings().filterByParam(m_idParamTypeMap.value(info->thingClassId()), entry.name());
        if (existingThings.count() == 1) {
            qCDebug(dcShelly()) << "This shelly already exists in the system!";
            descriptor.setThingId(existingThings.first()->id());
        }

        info->addThingDescriptor(descriptor);
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginShelly::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (m_idParamTypeMap.contains(thing->thingClassId())) {
        setupShellyGateway(info);
        return;
    }

    setupShellyChild(info);
}

void IntegrationPluginShelly::thingRemoved(Thing *thing)
{
    if (m_mqttChannels.contains(thing)) {
        hardwareManager()->mqttProvider()->releaseChannel(m_mqttChannels.take(thing));
    }

    if (myThings().isEmpty() && m_timer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_timer);
        m_timer = nullptr;
    }
    qCDebug(dcShelly()) << "Device removed" << thing->name();
}

void IntegrationPluginShelly::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (m_rebootActionTypeMap.contains(action.actionTypeId())) {
        QUrl url;
        url.setScheme("http");
        url.setHost(getIP(info->thing()));
        url.setPath("/reboot");
        url.setUserName(thing->paramValue(m_usernameParamTypeMap.value(thing->thingClassId())).toString());
        url.setPassword(thing->paramValue(m_passwordParamTypeMap.value(thing->thingClassId())).toString());
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply](){
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (m_updateActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = thing->paramValue(m_idParamTypeMap.value(thing->thingClassId())).toString();
        channel->publish(QString("shellies/%1/command").arg(shellyId), "update_fw");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (m_powerActionTypesMap.contains(action.actionTypeId())) {
        // If the main shelly has a power action (e.g. Shelly Plug, there is no parentId)
        Thing *parentDevice = thing->parentId().isNull() ? thing : myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(m_idParamTypeMap.value(parentDevice->thingClassId())).toString();
        int relay = 1;
        if (m_channelParamTypeMap.contains(thing->thingClassId())) {
            relay = thing->paramValue(m_channelParamTypeMap.value(thing->thingClassId())).toInt();
        }
        ParamTypeId powerParamTypeId = m_powerActionParamTypesMap.value(action.actionTypeId());
        bool on = action.param(powerParamTypeId).value().toBool();
        channel->publish(QString("shellies/%1/relay/%2/command").arg(shellyId).arg(relay - 1), on ? "on" : "off");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (m_colorPowerActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = info->thing()->paramValue(m_idParamTypeMap.value(info->thing()->thingClassId())).toString();
        ParamTypeId colorPowerParamTypeId = m_colorPowerActionParamTypesMap.value(action.actionTypeId());
        bool on = action.param(colorPowerParamTypeId).value().toBool();
        channel->publish("shellies/" + shellyId + "/color/0/command", on ? "on" : "off");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (m_colorActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = info->thing()->paramValue(m_idParamTypeMap.value(info->thing()->thingClassId())).toString();
        ParamTypeId colorParamTypeId = m_colorActionParamTypesMap.value(action.actionTypeId());
        QColor color = action.param(colorParamTypeId).value().value<QColor>();
        QVariantMap data;
        data.insert("turn", "on"); // Should we really?
        data.insert("red", color.red());
        data.insert("green", color.green());
        data.insert("blue", color.blue());
        data.insert("white", 0);
        QJsonDocument jsonDoc = QJsonDocument::fromVariant(data);
        channel->publish("shellies/" + shellyId + "/color/0/set", jsonDoc.toJson());
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (m_colorTemperatureStateTypeMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = info->thing()->paramValue(m_idParamTypeMap.value(info->thing()->thingClassId())).toString();
        ParamTypeId colorTemperatureParamTypeId = m_colorTemperatureActionParamTypesMap.value(action.actionTypeId());
        int ct = action.param(colorTemperatureParamTypeId).value().toInt();
        QVariantMap data;
        data.insert("turn", "on"); // Should we really?
        data.insert("red", qMin(255, ct * 255 / 50));
        data.insert("green", 0);
        data.insert("blue", qMax(0, ct - 50) * 255 / 50);
        data.insert("white", 255);
        QJsonDocument jsonDoc = QJsonDocument::fromVariant(data);
        channel->publish("shellies/" + shellyId + "/color/0/set", jsonDoc.toJson());
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (m_colorBrightnessActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = info->thing()->paramValue(m_idParamTypeMap.value(info->thing()->thingClassId())).toString();
        ParamTypeId brightnessParamTypeId = m_colorBrightnessActionParamTypesMap.value(action.actionTypeId());
        int brightness = action.param(brightnessParamTypeId).value().toInt();
        QVariantMap data;
        data.insert("turn", "on"); // Should we really?
        data.insert("gain", brightness);
        QJsonDocument jsonDoc = QJsonDocument::fromVariant(data);
        channel->publish("shellies/" + shellyId + "/color/0/set", jsonDoc.toJson());
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (m_dimmablePowerActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = info->thing()->paramValue(m_idParamTypeMap.value(info->thing()->thingClassId())).toString();
        ParamTypeId powerParamTypeId = m_dimmablePowerActionParamTypesMap.value(action.actionTypeId());
        bool on = action.param(powerParamTypeId).value().toBool();
        channel->publish("shellies/" + shellyId + "/light/0/command", on ? "on" : "off");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (m_dimmableBrightnessActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = info->thing()->paramValue(m_idParamTypeMap.value(info->thing()->thingClassId())).toString();
        ParamTypeId brightnessParamTypeId = m_dimmableBrightnessActionParamTypesMap.value(action.actionTypeId());
        int brightness = action.param(brightnessParamTypeId).value().toInt();
        QVariantMap data;
        data.insert("turn", "on"); // Should we really?
        data.insert("brightness", brightness);
        QJsonDocument jsonDoc = QJsonDocument::fromVariant(data);
        channel->publish("shellies/" + shellyId + "/light/0/set", jsonDoc.toJson());
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (m_rollerOpenActionTypeMap.contains(action.actionTypeId())) {
        Thing *parentDevice = myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(m_idParamTypeMap.value(parentDevice->thingClassId())).toString();
        channel->publish("shellies/" + shellyId + "/roller/0/command", "open");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (m_rollerCloseActionTypeMap.contains(action.actionTypeId())) {
        Thing *parentDevice = myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(m_idParamTypeMap.value(parentDevice->thingClassId())).toString();
        channel->publish("shellies/" + shellyId + "/roller/0/command", "close");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (m_rollerStopActionTypeMap.contains(action.actionTypeId())) {
        Thing *parentDevice = myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(m_idParamTypeMap.value(parentDevice->thingClassId())).toString();
        channel->publish("shellies/" + shellyId + "/roller/0/command", "stop");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (action.actionTypeId() == shellyEm3ResetActionTypeId) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = thing->paramValue(shellyEm3ThingIdParamTypeId).toString();
        channel->publish("shellies/" + shellyId + "/command", "reset_data");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (action.actionTypeId() == shellyRollerCalibrateActionTypeId) {
        Thing *parentDevice = myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(m_idParamTypeMap.value(parentDevice->thingClassId())).toString();
        channel->publish("shellies/" + shellyId + "/roller/0/command", "rc");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (action.actionTypeId() == shellyRollerPercentageActionTypeId) {
        Thing *parentDevice = myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(m_idParamTypeMap.value(parentDevice->thingClassId())).toString();
        channel->publish("shellies/" + shellyId + "/roller/0/command/pos", QByteArray::number(action.param(shellyRollerPercentageActionPercentageParamTypeId).value().toInt()));
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    qCWarning(dcShelly()) << "Unhandled execute action" << info->action().actionTypeId() << "call for device" << thing;
}

void IntegrationPluginShelly::onClientConnected(MqttChannel *channel)
{
    Thing *thing = m_mqttChannels.key(channel);
    if (!thing) {
        qCWarning(dcShelly()) << "Received a client connect for a thing we don't know!";
        return;
    }
    thing->setStateValue(m_connectedStateTypesMap.value(thing->thingClassId()), true);

    foreach (Thing *child, myThings().filterByParentId(thing->id())) {
        child->setStateValue(m_connectedStateTypesMap[child->thingClassId()], true);
    }
}

void IntegrationPluginShelly::onClientDisconnected(MqttChannel *channel)
{
    Thing *thing = m_mqttChannels.key(channel);
    if (!thing) {
        qCWarning(dcShelly()) << "Received a client disconnect for a thing we don't know!";
        return;
    }
    thing->setStateValue(m_connectedStateTypesMap.value(thing->thingClassId()), false);

    foreach (Thing *child, myThings().filterByParentId(thing->id())) {
        child->setStateValue(m_connectedStateTypesMap[child->thingClassId()], false);
    }
}

void IntegrationPluginShelly::onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload)
{
    Thing *thing = m_mqttChannels.key(channel);
    if (!thing) {
        qCWarning(dcShelly()) << "Received a publish message for a thing we don't know!";
        return;
    }

    qCDebug(dcShelly()) << "Publish received from" << thing->name() << topic;

    QString shellyId = thing->paramValue(m_idParamTypeMap.value(thing->thingClassId())).toString();
    if (topic == "shellies/" + shellyId + "/info") {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcShelly()) << "Failed to parse shelly info payload:" << error.errorString();
            qCWarning(dcShelly()) << qUtf8Printable(payload);
            return;
        }
//        qCDebug(dcShelly()) << "Payload:" << qUtf8Printable(jsonDoc.toJson());
        QVariantMap data = jsonDoc.toVariant().toMap();

        // Wifi signal strength
        int signalStrength = -1;
        if (data.value("wifi_sta").toMap().contains("rssi")) {
            int rssi = data.value("wifi_sta").toMap().value("rssi").toInt();
            signalStrength = qMin(100, qMax(0, (rssi + 100) * 2));
        }
        thing->setStateValue(m_signalStrengthStateTypesMap.value(thing->thingClassId()), signalStrength);
        foreach (Thing *child, myThings().filterByParentId(thing->id())) {
            child->setStateValue(m_signalStrengthStateTypesMap.value(child->thingClassId()), signalStrength);
        }

        // Firmware update
        QString updateStatus = updateStatusMap.value(data.value("update").toMap().value("status").toString());
        thing->setStateValue(m_updateStatusStateTypesMap.value(thing->thingClassId()), updateStatus);
        thing->setStateValue(m_currentVersionStateTypesMap.value(thing->thingClassId()), data.value("update").toMap().value("old_version").toString());
        thing->setStateValue(m_availableVersionStateTypesMap.value(thing->thingClassId()), data.value("update").toMap().value("new_version").toString());

        // While we normally use the specific topics instead of the "info" object, the Shell H&T posts it very rarely
        // and in combination with its power safe mode let's use this one to get temp/humidity
        if (thing->thingClassId() == shellyHTThingClassId) {
            if (data.value("tmp").toMap().value("is_valid").toBool()) {
                thing->setStateValue(shellyHTTemperatureStateTypeId, data.value("tmp").toMap().value("tC").toDouble());
            }
            if (data.value("hum").toMap().value("is_valid").toBool()) {
                thing->setStateValue(shellyHTHumidityStateTypeId, data.value("hum").toMap().value("value").toDouble());
            }
        }
    }


    if (topic.startsWith("shellies/" + shellyId + "/input/")) {
//        qCDebug(dcShelly()) << "Payload:" << payload;
        int channel = topic.split("/").last().toInt();
        // "1" or "0"
        // Emit event button pressed
        bool on = payload == "1";
        foreach (Thing *child, myThings().filterByParentId(thing->id())) {
            if (child->thingClassId() == shellySwitchThingClassId && child->paramValue(shellySwitchThingChannelParamTypeId).toInt() == channel + 1) {
                if (child->stateValue(shellySwitchPowerStateTypeId).toBool() != on) {
                    child->setStateValue(shellySwitchPowerStateTypeId, on);
                    emit emitEvent(Event(shellySwitchPressedEventTypeId, child->id()));
                }
            }
        }
    }

    QRegExp topicMatcher = QRegExp("shellies/" + shellyId + "/relay/[0-1]");
    if (topicMatcher.exactMatch(topic)) {
//        qCDebug(dcShelly()) << "Payload:" << payload;
        QStringList parts = topic.split("/");
        int channel = parts.at(3).toInt();
        bool on = payload == "on";

        // If the shelly main thing has a power state (e.g. Shelly Plug)
        if (m_powerStateTypeMap.contains(thing->thingClassId())) {
            thing->setStateValue(m_powerStateTypeMap.value(thing->thingClassId()), on);
        }

        // And switch all childs of this shelly too
        foreach (Thing *child, myThings().filterByParentId(thing->id())) {
            if (m_powerStateTypeMap.contains(child->thingClassId())) {
                ParamTypeId channelParamTypeId = m_channelParamTypeMap.value(child->thingClassId());
                if (child->paramValue(channelParamTypeId).toInt() == channel + 1) {
                    child->setStateValue(m_powerStateTypeMap.value(child->thingClassId()), on);
                }
            }
        }
    }

    topicMatcher = QRegExp("shellies/" + shellyId + "/(relay|roller)/[0-1]/power");
    if (topicMatcher.exactMatch(topic)) {
//        qCDebug(dcShelly()) << "Payload:" << payload;
        QStringList parts = topic.split("/");
        int channel = parts.at(3).toInt();
        double power = payload.toDouble();
        // If this gateway thing supports power measuring (e.g. Shelly Plug S) set it directly here
        if (m_currentPowerStateTypeMap.contains(thing->thingClassId())) {
            thing->setStateValue(m_currentPowerStateTypeMap.value(thing->thingClassId()), power);
        }
        // For multi-channel devices, power measurements are per-channel, so, find the child thing
        foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByInterface("extendedsmartmeterconsumer")) {
            ParamTypeId channelParamTypeId = m_channelParamTypeMap.value(child->thingClassId());
            if (child->paramValue(channelParamTypeId).toInt() == channel + 1) {
                child->setStateValue(m_currentPowerStateTypeMap.value(child->thingClassId()), power);
            }
        }
    }

    topicMatcher = QRegExp("shellies/" + shellyId + "/(relay|roller)/[0-1]/energy");
    if (topicMatcher.exactMatch(topic)) {
//        qCDebug(dcShelly()) << "Payload:" << payload;
        QStringList parts = topic.split("/");
        int channel = parts.at(3).toInt();
        // W/min => kW/h
        double energy = payload.toDouble() / 1000 / 60;
        // If this gateway thing supports energy measuring (e.g. Shelly Plug S) set it directly here
        if (m_totalEnergyConsumedStateTypeMap.contains(thing->thingClassId())) {
            thing->setStateValue(m_totalEnergyConsumedStateTypeMap.value(thing->thingClassId()), energy);
        }
        // For multi-channel devices, power measurements are per-channel, so, find the child thing
        foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByInterface("extendedsmartmeterconsumer")) {
            ParamTypeId channelParamTypeId = m_channelParamTypeMap.value(child->thingClassId());
            if (child->paramValue(channelParamTypeId).toInt() == channel + 1) {
                child->setStateValue(m_totalEnergyConsumedStateTypeMap.value(child->thingClassId()), energy);
            }
        }
    }

    if (topic == "shellies/" + shellyId + "/color/0") {
//        qCDebug(dcShelly()) << "Payload:" << payload;
        bool on = payload == "on";
        if (m_powerStateTypeMap.contains(thing->thingClassId())) {
            thing->setStateValue(m_powerStateTypeMap.value(thing->thingClassId()), on);
        }
    }

    if (topic == "shellies/" + shellyId + "/color/0/status") {
//        qCDebug(dcShelly()) << "Payload:" << payload;
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcShelly()) << "Error parsing JSON from Shelly:" << error.error << error.errorString() << payload;
            return;
        }
        QVariantMap statusMap = jsonDoc.toVariant().toMap();
        if (m_colorStateTypeMap.contains(thing->thingClassId())) {
            QColor color = QColor(statusMap.value("red").toInt(), statusMap.value("green").toInt(), statusMap.value("blue").toInt());
            thing->setStateValue(m_colorStateTypeMap.value(thing->thingClassId()), color);
        }
        if (m_brightnessStateTypeMap.contains(thing->thingClassId())) {
            int brightness = statusMap.value("gain").toInt();
            thing->setStateValue(m_brightnessStateTypeMap.value(thing->thingClassId()), brightness);
        }
        if (m_currentPowerStateTypeMap.contains(thing->thingClassId())) {
            double power = statusMap.value("power").toDouble();
            thing->setStateValue(m_currentPowerStateTypeMap.value(thing->thingClassId()), power);
        }
    }

    if (topic == "shellies/" + shellyId + "/light/0") {
//        qCDebug(dcShelly()) << "Payload:" << payload;
        bool on = payload == "on";
        if (m_powerStateTypeMap.contains(thing->thingClassId())) {
            thing->setStateValue(m_powerStateTypeMap.value(thing->thingClassId()), on);
        }
    }

    if (topic == "shellies/" + shellyId + "/light/0/status") {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcShelly()) << "Error parsing JSON from Shelly:" << error.error << error.errorString() << payload;
            return;
        }
//        qCDebug(dcShelly()) << "Payload:" << qUtf8Printable(jsonDoc.toJson());
        QVariantMap statusMap = jsonDoc.toVariant().toMap();
        if (m_brightnessStateTypeMap.contains(thing->thingClassId())) {
            int brightness = statusMap.value("brightness").toInt();
            thing->setStateValue(m_brightnessStateTypeMap.value(thing->thingClassId()), brightness);
        }
    }

    if (topic == "shellies/" + shellyId + "/light/0/power") {
//        qCDebug(dcShelly()) << "Payload:" << payload;
        if (m_currentPowerStateTypeMap.contains(thing->thingClassId())) {
            double power = payload.toDouble();
            thing->setStateValue(m_currentPowerStateTypeMap.value(thing->thingClassId()), power);
        }
    }

    if (topic == "shellies/" + shellyId + "/roller/0") {
//        qCDebug(dcShelly()) << "Payload:" << payload;
        // Roller shutters are always child devices...
        foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByInterface("extendedshutter")) {
            child->setStateValue(shellyRollerMovingStateTypeId, payload != "stop");
        }
    }
    if (topic == "shellies/" + shellyId + "/roller/0/pos") {
//        qCDebug(dcShelly()) << "Payload:" << payload;
        // Roller shutters are always child devices...
        int pos = payload.toInt();
        foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByInterface("extendedshutter")) {
            child->setStateValue(shellyRollerPercentageStateTypeId, 100 - pos);
        }
    }

    if (topic == "shellies/" + shellyId + "/sensor/battery") {
        if (m_batteryLevelStateTypeMap.contains(thing->thingClassId())) {
            int batteryLevel = payload.toInt();
            thing->setStateValue(m_batteryLevelStateTypeMap.value(thing->thingClassId()), batteryLevel);
            thing->setStateValue(m_batteryCriticalStateTypeMap.value(thing->thingClassId()), batteryLevel < 10);
        }
    }

    if (topic == "shellies/" + shellyId + "/input_event/0") {
        if (thing->thingClassId() == shellyButton1ThingClassId) {
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcShelly()) << "Failed to parse JSON from shelly:" << error.errorString() << qUtf8Printable(payload);
                return;
            }
            QString event = jsonDoc.toVariant().toMap().value("event").toString();
            if (event.isEmpty()) {
                return;
            }
            EventTypeId eventTypeId = event == "L" ? shellyButton1LongPressedEventTypeId : shellyButton1PressedEventTypeId;
            ParamTypeId paramTypeId = eventTypeId == shellyButton1PressedEventTypeId ? shellyButton1PressedEventButtonNameParamTypeId : shellyButton1LongPressedEventButtonNameParamTypeId;
            QString param = QString::number(event.length());
            thing->emitEvent(eventTypeId, ParamList() << Param(paramTypeId, param));
        }
    }

    topicMatcher = QRegExp("shellies/" + shellyId + "/emeter/[0-2]/.*");
    if (topicMatcher.exactMatch(topic) && thing->thingClassId() == shellyEm3ThingClassId) {
        int channel = topic.split('/').at(3).toInt();
        QString stateName = topic.split('/').at(4);
        QVariant value = payload;
        QHash<int, QHash<QString, StateTypeId>> channelMap;
        QHash<QString, StateTypeId> stateTypeIdMap;
        stateTypeIdMap["power"] = shellyEm3PhaseAPowerStateTypeId;
        stateTypeIdMap["pf"] = shellyEm3PhaseAPowerFactorStateTypeId;
        stateTypeIdMap["current"] = shellyEm3PhaseACurrentStateTypeId;
        stateTypeIdMap["voltage"] = shellyEm3PhaseAVoltageStateTypeId;
        stateTypeIdMap["total"] = shellyEm3PhaseATotalEnergyConsumedStateTypeId;
        stateTypeIdMap["total_returned"] = shellyEm3PhaseATotalEnergyReturnedStateTypeId;
        channelMap[0] = stateTypeIdMap;
        stateTypeIdMap["power"] = shellyEm3PhaseBPowerStateTypeId;
        stateTypeIdMap["pf"] = shellyEm3PhaseBPowerFactorStateTypeId;
        stateTypeIdMap["current"] = shellyEm3PhaseBCurrentStateTypeId;
        stateTypeIdMap["voltage"] = shellyEm3PhaseBVoltageStateTypeId;
        stateTypeIdMap["total"] = shellyEm3PhaseBTotalEnergyConsumedStateTypeId;
        stateTypeIdMap["total_returned"] = shellyEm3PhaseBTotalEnergyReturnedStateTypeId;
        channelMap[1] = stateTypeIdMap;
        stateTypeIdMap["power"] = shellyEm3PhaseCPowerStateTypeId;
        stateTypeIdMap["pf"] = shellyEm3PhaseCPowerFactorStateTypeId;
        stateTypeIdMap["current"] = shellyEm3PhaseCCurrentStateTypeId;
        stateTypeIdMap["voltage"] = shellyEm3PhaseCVoltageStateTypeId;
        stateTypeIdMap["total"] = shellyEm3PhaseCTotalEnergyConsumedStateTypeId;
        stateTypeIdMap["total_returned"] = shellyEm3PhaseCTotalEnergyReturnedStateTypeId;
        channelMap[2] = stateTypeIdMap;
        StateTypeId stateTypeId = channelMap.value(channel).value(stateName);
        if (stateTypeId.isNull()) {
            qCWarning(dcShelly()) << "Unhandled emeter value for channel" << channel << stateName;
            return;
        }
        double factor = 1;
        if (stateName == "total" || stateName == "total_returned") {
            factor = 0.001;
        }

        thing->setStateValue(stateTypeId, value.toDouble() * factor);

        // Some optimization specific to the EM3: We receive each phase in a separate mqtt message
        // and calculate the total ourselves. In order to not produce intermediate totals for each incoming message
        // we'll only refresh the total when we get the last value for the last channel.
        if (channel == 2 && stateName == "total_returned") {
            double grandTotal = thing->stateValue(shellyEm3PhaseATotalEnergyConsumedStateTypeId).toDouble();
            grandTotal += thing->stateValue(shellyEm3PhaseBTotalEnergyConsumedStateTypeId).toDouble();
            grandTotal += thing->stateValue(shellyEm3PhaseCTotalEnergyConsumedStateTypeId).toDouble();
            thing->setStateValue(shellyEm3TotalEnergyConsumedStateTypeId, grandTotal);
            double grandTotalReturned = thing->stateValue(shellyEm3PhaseATotalEnergyReturnedStateTypeId).toDouble();
            grandTotalReturned += thing->stateValue(shellyEm3PhaseBTotalEnergyReturnedStateTypeId).toDouble();
            grandTotalReturned += thing->stateValue(shellyEm3PhaseCTotalEnergyReturnedStateTypeId).toDouble();
            thing->setStateValue(shellyEm3TotalEnergyProducedStateTypeId, grandTotalReturned);
            double totalPower = thing->stateValue(shellyEm3PhaseAPowerStateTypeId).toDouble();
            totalPower += thing->stateValue(shellyEm3PhaseBPowerStateTypeId).toDouble();
            totalPower += thing->stateValue(shellyEm3PhaseCPowerStateTypeId).toDouble();
            thing->setStateValue(shellyEm3CurrentPowerStateTypeId, totalPower);
        }
    }
}

void IntegrationPluginShelly::updateStatus()
{
    foreach (Thing *thing, m_mqttChannels.keys()) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = thing->paramValue(m_idParamTypeMap.value(thing->thingClassId())).toString();
        qCDebug(dcShelly()) << "Requesting announcement" << QString("shellies/%1/info").arg(shellyId);
        channel->publish(QString("shellies/%1/command").arg(shellyId), "announce");
    }
}

void IntegrationPluginShelly::setupShellyGateway(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QString shellyId = info->thing()->paramValue(m_idParamTypeMap.value(info->thing()->thingClassId())).toString();
    ZeroConfServiceEntry zeroConfEntry;
    foreach (const ZeroConfServiceEntry &entry, m_zeroconfBrowser->serviceEntries()) {
        if (entry.name() == shellyId) {
            zeroConfEntry = entry;
        }
    }
    QHostAddress address;
    pluginStorage()->beginGroup(info->thing()->id().toString());
    if (zeroConfEntry.isValid()) {
        address = zeroConfEntry.hostAddress();
        pluginStorage()->setValue("cachedAddress", address.toString());
    } else {
        qCWarning(dcShelly()) << "Could not find Shelly thing on zeroconf. Trying cached address.";
        address = QHostAddress(pluginStorage()->value("cachedAddress").toString());
    }
    pluginStorage()->endGroup();

    if (address.isNull()) {
        qCWarning(dcShelly()) << "Unable to determine Shelly's network address. Failed to set up device.";
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to find the thing in the network."));
        return;
    }

    // Validate params
    bool rollerMode = false;
    if (info->thing()->thingClassId() == shelly2ThingClassId || info->thing()->thingClassId() == shelly25ThingClassId) {
        QString connectedDevice1 = info->thing()->paramValue(m_connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString();
        QString connectedDevice2 = info->thing()->paramValue(m_connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString();
        if (connectedDevice1.startsWith("Roller Shutter") && !connectedDevice2.startsWith("Roller Shutter")) {
            qCWarning(dcShelly()) << "Cannot mix roller and relay mode. This won't work..";
            info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("Roller shutter mode can't be mixed with relay mode. Please configure both connected devices to control a shutter or relays."));
            return;
        }
        if (connectedDevice1 == "Roller Shutter Up" && connectedDevice2 != "Roller Shutter Down") {
            qCWarning(dcShelly()) << "Connected thing 1 is shutter up but connected thing 2 is not shutter down. This won't work..";
            info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("For using a roller shutter, one channel must be set to up, the other to down."));
            return;
        }
        if (connectedDevice1 == "Roller Shutter Down" && connectedDevice2 != "Roller Shutter Up") {
            qCWarning(dcShelly()) << "Connected thing 1 is shutter down but connected thing 2 is not shutter up. This won't work..";
            info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("For using a roller shutter, one channel must be set to up, the other to down."));
            return;
        }
        if (connectedDevice1.startsWith("Roller Shutter") && connectedDevice2.startsWith("Roller Shutter")) {
            rollerMode = true;
        }
    }

    MqttChannel *channel = hardwareManager()->mqttProvider()->createChannel(shellyId, QHostAddress(address), {"shellies"});
    m_mqttChannels.insert(info->thing(), channel);

    if (!channel) {
        qCWarning(dcShelly()) << "Failed to create MQTT channel.";
        return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error creating MQTT channel. Please check MQTT server settings."));
    }
    connect(channel, &MqttChannel::clientConnected, this, &IntegrationPluginShelly::onClientConnected);
    connect(channel, &MqttChannel::clientDisconnected, this, &IntegrationPluginShelly::onClientDisconnected);
    connect(channel, &MqttChannel::publishReceived, this, &IntegrationPluginShelly::onPublishReceived);

    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());
    url.setPort(80);
    url.setPath("/settings");
    url.setUserName(info->thing()->paramValue(m_usernameParamTypeMap.value(info->thing()->thingClassId())).toString());
    url.setPassword(info->thing()->paramValue(m_passwordParamTypeMap.value(info->thing()->thingClassId())).toString());

    QUrlQuery query;
    query.addQueryItem("mqtt_server", channel->serverAddress().toString() + ":" + QString::number(channel->serverPort()));
    query.addQueryItem("mqtt_user", channel->username());
    query.addQueryItem("mqtt_pass", channel->password());
    query.addQueryItem("mqtt_enable", "true");

    // Make sure the shelly 2.5 is in the mode we expect it to be (roller or relay)
    if (info->thing()->thingClassId() == shelly25ThingClassId) {
        query.addQueryItem("mode", rollerMode ? "roller" : "relay");
    }

    url.setQuery(query);
    QNetworkRequest request(url);

    qCDebug(dcShelly()) << "Connecting to" << url.toString();
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(info, &ThingSetupInfo::aborted, channel, [this, channel, thing](){
        qCWarning(dcShelly()) << "Setup for" << thing->name() << "aborted.";
        hardwareManager()->mqttProvider()->releaseChannel(channel);
        m_mqttChannels.remove(thing);
    });
    connect(reply, &QNetworkReply::finished, info, [this, info, reply, channel, address](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcShelly()) << "Error fetching thing settings for" << info->thing()->name() << reply->error() << reply->errorString();
            // Given the networkManagers timeout is the same as the info timeout (30s) and they are
            // both started in the same event loop pass, they'll also time out in the same event loop pass
            // and it happens we'll get both, ThingSetupInfo::aborted *and* QNetworkReply::finished (with the
            // aborted flag) which both clean up the MQTT channel. Make sure to check if it's still there
            // before actually cleaning up. We can't remove any of the cleanups as that might cause leaks if
            // either the network reply finishes with an earlier error, or the setup is aborted earlier.
            if (m_mqttChannels.contains(info->thing())) {
                hardwareManager()->mqttProvider()->releaseChannel(channel);
                m_mqttChannels.remove(info->thing());
            }
            if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
                info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Username and password not set correctly."));
            } else {
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error connecting to Shelly device."));
            }
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcShelly()) << "Error parsing settings reply" << error.errorString() << "\n" << data;
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unexpected data received from Shelly device."));
            hardwareManager()->mqttProvider()->releaseChannel(channel);
            m_mqttChannels.remove(info->thing());
            return;
        }
        qCDebug(dcShelly()) << "Settings data" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
        QVariantMap settingsMap = jsonDoc.toVariant().toMap();

        if (info->thing()->thingClassId() == shellyPlugThingClassId) {
            info->thing()->setSettingValue(shellyPlugSettingsDefaultStateParamTypeId, settingsMap.value("relays").toList().first().toMap().value("default_state").toString());
        } else if (info->thing()->thingClassId() == shellyButton1ThingClassId) {
            info->thing()->setSettingValue(shellyButton1SettingsRemainAwakeParamTypeId, settingsMap.value("remain_awake").toInt());
            info->thing()->setSettingValue(shellyButton1SettingsStatusLedEnabledParamTypeId, !settingsMap.value("led_status_disable").toBool());
            info->thing()->setSettingValue(shellyButton1SettingsLongpushMaxDurationParamTypeId, settingsMap.value("longpush_duration_ms").toMap().value("max").toUInt());
            info->thing()->setSettingValue(shellyButton1SettingsMultipressIntervalParamTypeId, settingsMap.value("multipush_time_between_pushes_ms").toMap().value("max").toUInt());
        }

        ThingDescriptors autoChilds;

        // Autogenerate some childs if this thing has no childs yet
        if (myThings().filterByParentId(info->thing()->id()).isEmpty()) {
            // Always create the switch thing if we don't have one yet for shellies with input (1, 1pm etc)
            if (info->thing()->thingClassId() == shelly1ThingClassId
                    || info->thing()->thingClassId() == shelly1pmThingClassId
                    || info->thing()->thingClassId() == shelly1lThingClassId) {
                ThingDescriptor switchChild(shellySwitchThingClassId, info->thing()->name() + " switch", QString(), info->thing()->id());
                switchChild.setParams(ParamList() << Param(shellySwitchThingChannelParamTypeId, 1));
                autoChilds.append(switchChild);
            }

            // Create 2 switches for shelly 2.5
            if (info->thing()->thingClassId() == shelly2ThingClassId
                    || info->thing()->thingClassId() == shelly25ThingClassId
                    || info->thing()->thingClassId() == shellyDimmerThingClassId) {
                ThingDescriptor switchChild(shellySwitchThingClassId, info->thing()->name() + " switch 1", QString(), info->thing()->id());
                switchChild.setParams(ParamList() << Param(shellySwitchThingChannelParamTypeId, 1));
                autoChilds.append(switchChild);
                ThingDescriptor switch2Child(shellySwitchThingClassId, info->thing()->name() + " switch 2", QString(), info->thing()->id());
                switch2Child.setParams(ParamList() << Param(shellySwitchThingChannelParamTypeId, 2));
                autoChilds.append(switch2Child);
            }

            // Add connected devices as configured in params
            // No PM devices for shelly 1 and 2
            if (info->thing()->thingClassId() == shelly1ThingClassId
                    || info->thing()->thingClassId() == shelly2ThingClassId) {
                if (info->thing()->paramValue(m_connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Generic") {
                    ThingDescriptor genericChild(shellyGenericThingClassId, info->thing()->name() + " connected device", QString(), info->thing()->id());
                    genericChild.setParams(ParamList() << Param(shellyGenericThingChannelParamTypeId, 1));
                    autoChilds.append(genericChild);
                }
                if (info->thing()->paramValue(m_connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Light") {
                    ThingDescriptor lightChild(shellyLightThingClassId, info->thing()->name() + " connected light", QString(), info->thing()->id());
                    lightChild.setParams(ParamList() << Param(shellyLightThingChannelParamTypeId, 1));
                    autoChilds.append(lightChild);
                }
                if (info->thing()->paramValue(m_connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Socket") {
                    ThingDescriptor socketChild(shellySocketThingClassId, info->thing()->name() + " connected socket", QString(), info->thing()->id());
                    socketChild.setParams(ParamList() << Param(shellySocketThingChannelParamTypeId, 1));
                    autoChilds.append(socketChild);
                }
            // PM devices for shelly 1 pm and 2.5
            } else if (info->thing()->thingClassId() == shelly1pmThingClassId
                       || info->thing()->thingClassId() == shelly25ThingClassId) {
                if (info->thing()->paramValue(m_connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Generic") {
                    ThingDescriptor genericChild(shellyGenericPMThingClassId, info->thing()->name() + " connected device", QString(), info->thing()->id());
                    genericChild.setParams(ParamList() << Param(shellyGenericPMThingChannelParamTypeId, 1));
                    autoChilds.append(genericChild);
                }
                if (info->thing()->paramValue(m_connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Light") {
                    ThingDescriptor lightChild(shellyLightPMThingClassId, info->thing()->name() + " connected light", QString(), info->thing()->id());
                    lightChild.setParams(ParamList() << Param(shellyLightPMThingChannelParamTypeId, 1));
                    autoChilds.append(lightChild);
                }
                if (info->thing()->paramValue(m_connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Socket") {
                    ThingDescriptor socketChild(shellySocketPMThingClassId, info->thing()->name() + " connected socket", QString(), info->thing()->id());
                    socketChild.setParams(ParamList() << Param(shellySocketPMThingChannelParamTypeId, 1));
                    autoChilds.append(socketChild);
                }

                // Second channel for shelly 2 (no power metering)
                if (info->thing()->thingClassId() == shelly2ThingClassId) {
                    if (info->thing()->paramValue(m_connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Generic") {
                        ThingDescriptor genericChild(shellyGenericThingClassId, info->thing()->name() + " connected thing 2", QString(), info->thing()->id());
                        genericChild.setParams(ParamList() << Param(shellyGenericThingChannelParamTypeId, 2));
                        autoChilds.append(genericChild);
                    }
                    if (info->thing()->paramValue(m_connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Light") {
                        ThingDescriptor lightChild(shellyLightThingClassId, info->thing()->name() + " connected light 2", QString(), info->thing()->id());
                        lightChild.setParams(ParamList() << Param(shellyLightThingChannelParamTypeId, 2));
                        autoChilds.append(lightChild);
                    }
                    if (info->thing()->paramValue(m_connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Socket") {
                        ThingDescriptor socketChild(shellySocketThingClassId, info->thing()->name() + " connected socket 2", QString(), info->thing()->id());
                        socketChild.setParams(ParamList() << Param(shellySocketThingChannelParamTypeId, 2));
                        autoChilds.append(socketChild);
                    }
                }

                // Second channel for shelly 2.5 (with power metering)
                if (info->thing()->thingClassId() == shelly25ThingClassId) {
                    if (info->thing()->paramValue(m_connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Generic") {
                        ThingDescriptor genericChild(shellyGenericPMThingClassId, info->thing()->name() + " connected thing 2", QString(), info->thing()->id());
                        genericChild.setParams(ParamList() << Param(shellyGenericPMThingChannelParamTypeId, 2));
                        autoChilds.append(genericChild);
                    }
                    if (info->thing()->paramValue(m_connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Light") {
                        ThingDescriptor lightChild(shellyLightPMThingClassId, info->thing()->name() + " connected light 2", QString(), info->thing()->id());
                        lightChild.setParams(ParamList() << Param(shellyLightPMThingChannelParamTypeId, 2));
                        autoChilds.append(lightChild);
                    }
                    if (info->thing()->paramValue(m_connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Socket") {
                        ThingDescriptor socketChild(shellySocketPMThingClassId, info->thing()->name() + " connected socket 2", QString(), info->thing()->id());
                        socketChild.setParams(ParamList() << Param(shellySocketPMThingChannelParamTypeId, 2));
                        autoChilds.append(socketChild);
                    }
                }

                // And finally the special roller shutter mode
                if (info->thing()->thingClassId() == shelly2ThingClassId
                        || info->thing()->thingClassId() == shelly25ThingClassId) {
                    if (info->thing()->paramValue(m_connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Roller Shutter Up"
                            && info->thing()->paramValue(m_connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Roller Shutter Down") {
                        ThingDescriptor rollerShutterChild(shellyRollerThingClassId, info->thing()->name() + " connected shutter", QString(), info->thing()->id());
                        rollerShutterChild.setParams(ParamList() << Param(shellyRollerThingChannelParamTypeId, 1));
                        autoChilds.append(rollerShutterChild);
                    }
                }
            }
        }

        info->finish(Thing::ThingErrorNoError);

        emit autoThingsAppeared(autoChilds);

        if (!m_timer) {
            m_timer = hardwareManager()->pluginTimerManager()->registerTimer(10);
            connect(m_timer, &PluginTimer::timeout, this, &IntegrationPluginShelly::updateStatus);
        }

        // Make sure authentication is enalbed if the user wants it
        QString username = info->thing()->paramValue(m_usernameParamTypeMap.value(info->thing()->thingClassId())).toString();
        QString password = info->thing()->paramValue(m_passwordParamTypeMap.value(info->thing()->thingClassId())).toString();
        if (!username.isEmpty()) {
            QUrl url;
            url.setScheme("http");
            url.setHost(address.toString());
            url.setPort(80);
            url.setPath("/settings/login");
            url.setUserName(username);
            url.setPassword(password);

            QUrlQuery query;
            query.addQueryItem("username", username);
            query.addQueryItem("password", password);
            query.addQueryItem("enabled", "true");

            url.setQuery(query);

            QNetworkRequest request(url);
            qCDebug(dcShelly()) << "Enabling auth" << username << password;
            QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        }
    });

    // Handle thing settings of gateway devices
    if (info->thing()->thingClassId() == shellyPlugThingClassId ||
            info->thing()->thingClassId() == shellyButton1ThingClassId) {
        connect(info->thing(), &Thing::settingChanged, this, [this, thing, shellyId](const ParamTypeId &settingTypeId, const QVariant &value) {

            pluginStorage()->beginGroup(thing->id().toString());
            QString address = pluginStorage()->value("cachedAddress").toString();
            pluginStorage()->endGroup();

            QUrl url;
            url.setScheme("http");
            url.setHost(address);
            url.setPort(80);
            url.setUserName(thing->paramValue(m_usernameParamTypeMap.value(thing->thingClassId())).toString());
            url.setPassword(thing->paramValue(m_passwordParamTypeMap.value(thing->thingClassId())).toString());

            QUrlQuery query;
            if (settingTypeId == shellyPlugSettingsDefaultStateParamTypeId) {
                url.setPath("/settings/relay/0");
                query.addQueryItem("default_state", value.toString());
            } else if (settingTypeId == shellyButton1SettingsRemainAwakeParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("remain_awake", value.toString());
            } else if (settingTypeId == shellyButton1SettingsStatusLedEnabledParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("led_status_disable", value.toBool() ? "false" : "true");
            } else if (settingTypeId == shellyButton1SettingsLongpushMaxDurationParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("longpush_duration_ms_max", value.toString());
            }

            url.setQuery(query);

            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        });
    }
}

void IntegrationPluginShelly::setupShellyChild(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcShelly()) << "Setting up shelly child:" << info->thing()->name();

    Thing *parent = myThings().findById(thing->parentId());
    Q_ASSERT_X(parent != nullptr, "Shelly::setupChild", "Child has no parent!");
    if (!parent->setupComplete()) {
        qCDebug(dcShelly()) << "Parent for" << info->thing()->name() << "is not set up yet... Waiting...";
        // If the parent isn't set up yet, wait for it...
        connect(parent, &Thing::setupStatusChanged, info, [=](){
            qCDebug(dcShelly()) << "Setup for" << parent->name() << "Completed. Continuing with setup of child" << info->thing()->name();
            if (parent->setupStatus() == Thing::ThingSetupStatusComplete) {
                setupShellyChild(info);
            } else if (parent->setupStatus() == Thing::ThingSetupStatusFailed) {
                info->finish(Thing::ThingErrorHardwareFailure);
            }
        });
        return;
    }

    qCDebug(dcShelly()) << "Parent for" << info->thing()->name() << "is set up. Finishing child setup.";

    // Connect to settings changes to store them to the thing
    connect(info->thing(), &Thing::settingChanged, this, [this, thing](const ParamTypeId &paramTypeId, const QVariant &value){
        Thing *parentDevice = myThings().findById(thing->parentId());
        pluginStorage()->beginGroup(parentDevice->id().toString());
        QString address = pluginStorage()->value("cachedAddress").toString();
        pluginStorage()->endGroup();

        QUrl url;
        url.setScheme("http");
        url.setHost(address);
        url.setPort(80);
        url.setPath("/settings/relay/0");
        url.setUserName(parentDevice->paramValue(m_usernameParamTypeMap.value(parentDevice->thingClassId())).toString());
        url.setPassword(parentDevice->paramValue(m_passwordParamTypeMap.value(parentDevice->thingClassId())).toString());

        QUrlQuery query;
        if (paramTypeId == shellySwitchSettingsButtonTypeParamTypeId) {
            query.addQueryItem("btn_type", value.toString());
        }
        if (paramTypeId == shellySwitchSettingsInvertButtonParamTypeId) {
            query.addQueryItem("btn_reverse", value.toBool() ? "1" : "0");
        }
        if (paramTypeId == shellyGenericSettingsDefaultStateParamTypeId || paramTypeId == shellyLightSettingsDefaultStateParamTypeId) {
            query.addQueryItem("default_state", value.toString());
        }

        url.setQuery(query);

        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    });

    info->finish(Thing::ThingErrorNoError);
}

QString IntegrationPluginShelly::getIP(Thing *thing) const
{
    Thing *d = thing;
    if (!thing->parentId().isNull()) {
        d = myThings().findById(thing->parentId());
    }
    pluginStorage()->beginGroup(d->id().toString());
    QString ip = pluginStorage()->value("cachedAddress").toString();
    pluginStorage()->endGroup();
    return ip;
}
