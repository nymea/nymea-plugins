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

static QHash<ThingClassId, ParamTypeId> idParamTypeMap = {
    {shelly1ThingClassId, shelly1ThingIdParamTypeId},
    {shelly1pmThingClassId, shelly1pmThingIdParamTypeId},
    {shelly1lThingClassId, shelly1lThingIdParamTypeId},
    {shellyPlugThingClassId, shellyPlugThingIdParamTypeId},
    {shellyRgbw2ThingClassId, shellyRgbw2ThingIdParamTypeId},
    {shellyDimmerThingClassId, shellyDimmerThingIdParamTypeId},
    {shelly2ThingClassId, shelly2ThingIdParamTypeId},
    {shelly25ThingClassId, shelly25ThingIdParamTypeId},
    {shellyButton1ThingClassId, shellyButton1ThingIdParamTypeId},
    {shellyEm3ThingClassId, shellyEm3ThingIdParamTypeId},
    {shellyHTThingClassId, shellyHTThingIdParamTypeId},
    {shellyI3ThingClassId, shellyI3ThingIdParamTypeId},
    {shellyMotionThingClassId, shellyMotionThingIdParamTypeId},
};

static QHash<ThingClassId, ParamTypeId> usernameParamTypeMap = {
    {shelly1ThingClassId, shelly1ThingUsernameParamTypeId},
    {shelly1pmThingClassId, shelly1pmThingUsernameParamTypeId},
    {shelly1lThingClassId, shelly1lThingUsernameParamTypeId},
    {shellyPlugThingClassId, shellyPlugThingUsernameParamTypeId},
    {shellyRgbw2ThingClassId, shellyRgbw2ThingUsernameParamTypeId},
    {shellyDimmerThingClassId, shellyDimmerThingUsernameParamTypeId},
    {shelly2ThingClassId, shelly2ThingUsernameParamTypeId},
    {shelly25ThingClassId, shelly25ThingUsernameParamTypeId},
    {shellyButton1ThingClassId, shellyButton1ThingUsernameParamTypeId},
    {shellyEm3ThingClassId, shellyEm3ThingUsernameParamTypeId},
    {shellyHTThingClassId, shellyHTThingUsernameParamTypeId},
    {shellyI3ThingClassId, shellyI3ThingUsernameParamTypeId},
    {shellyMotionThingClassId, shellyMotionThingUsernameParamTypeId}
};

static QHash<ThingClassId, ParamTypeId> passwordParamTypeMap = {
    {shelly1ThingClassId, shelly1ThingPasswordParamTypeId},
    {shelly1pmThingClassId, shelly1pmThingPasswordParamTypeId},
    {shelly1lThingClassId, shelly1lThingPasswordParamTypeId},
    {shellyPlugThingClassId, shellyPlugThingPasswordParamTypeId},
    {shellyRgbw2ThingClassId, shellyRgbw2ThingPasswordParamTypeId},
    {shellyDimmerThingClassId, shellyDimmerThingPasswordParamTypeId},
    {shelly2ThingClassId, shelly2ThingPasswordParamTypeId},
    {shelly25ThingClassId, shelly25ThingPasswordParamTypeId},
    {shellyButton1ThingClassId, shellyButton1ThingPasswordParamTypeId},
    {shellyEm3ThingClassId, shellyEm3ThingPasswordParamTypeId},
    {shellyHTThingClassId, shellyHTThingPasswordParamTypeId},
    {shellyI3ThingClassId, shellyI3ThingPasswordParamTypeId},
    {shellyMotionThingClassId, shellyMotionThingPasswordParamTypeId}
};

static QHash<ThingClassId, ParamTypeId> connectedDeviceParamTypeMap = {
    {shelly2ThingClassId, shelly2ThingConnectedDevice1ParamTypeId},
    {shelly25ThingClassId, shelly25ThingConnectedDevice1ParamTypeId}
};

static QHash<ThingClassId, ParamTypeId> connectedDevice2ParamTypeMap = {
    {shelly2ThingClassId, shelly2ThingConnectedDevice2ParamTypeId},
    {shelly25ThingClassId, shelly25ThingConnectedDevice2ParamTypeId},
};

static QHash<ThingClassId, ParamTypeId> channelParamTypeMap = {
    {shellyGenericThingClassId, shellyGenericThingChannelParamTypeId},
    {shellyLightThingClassId, shellyLightThingChannelParamTypeId},
    {shellySocketThingClassId, shellySocketThingChannelParamTypeId},
    {shellyGenericPMThingClassId, shellyGenericPMThingChannelParamTypeId},
    {shellyLightPMThingClassId, shellyLightPMThingChannelParamTypeId},
    {shellySocketPMThingClassId, shellySocketPMThingChannelParamTypeId},
    {shellyRollerThingClassId, shellyRollerThingChannelParamTypeId},
};

static QHash<ThingClassId, StateTypeId> connectedStateTypesMap = {
    {shelly1ThingClassId, shelly1ConnectedStateTypeId},
    {shelly1pmThingClassId, shelly1pmConnectedStateTypeId},
    {shelly1lThingClassId, shelly1lConnectedStateTypeId},
    {shelly2ThingClassId, shelly2ConnectedStateTypeId},
    {shelly25ThingClassId, shelly25ConnectedStateTypeId},
    {shellyPlugThingClassId, shellyPlugConnectedStateTypeId},
    {shellyRgbw2ThingClassId, shellyRgbw2ConnectedStateTypeId},
    {shellyDimmerThingClassId, shellyDimmerConnectedStateTypeId},
    {shellyButton1ThingClassId, shellyButton1ConnectedStateTypeId},
    {shellyEm3ThingClassId, shellyEm3ConnectedStateTypeId},
    {shellyHTThingClassId, shellyHTConnectedStateTypeId},
    {shellySwitchThingClassId, shellySwitchConnectedStateTypeId},
    {shellyGenericThingClassId, shellyGenericConnectedStateTypeId},
    {shellyLightThingClassId, shellyLightConnectedStateTypeId},
    {shellySocketThingClassId, shellySocketConnectedStateTypeId},
    {shellyGenericPMThingClassId, shellyGenericPMConnectedStateTypeId},
    {shellyLightPMThingClassId, shellyLightPMConnectedStateTypeId},
    {shellySocketPMThingClassId, shellySocketPMConnectedStateTypeId},
    {shellyRollerThingClassId, shellyRollerConnectedStateTypeId},
    {shellyI3ThingClassId, shellyI3ConnectedStateTypeId},
    {shellyMotionThingClassId, shellyMotionConnectedStateTypeId}
};

static QHash<ThingClassId, StateTypeId> signalStrengthStateTypesMap = {
    {shelly1ThingClassId, shelly1SignalStrengthStateTypeId},
    {shelly1pmThingClassId, shelly1pmSignalStrengthStateTypeId},
    {shelly1lThingClassId, shelly1lSignalStrengthStateTypeId},
    {shelly2ThingClassId, shelly2SignalStrengthStateTypeId},
    {shelly25ThingClassId, shelly25SignalStrengthStateTypeId},
    {shellyPlugThingClassId, shellyPlugSignalStrengthStateTypeId},
    {shellyRgbw2ThingClassId, shellyRgbw2SignalStrengthStateTypeId},
    {shellyDimmerThingClassId, shellyDimmerSignalStrengthStateTypeId},
    {shellyButton1ThingClassId, shellyButton1SignalStrengthStateTypeId},
    {shellyEm3ThingClassId, shellyEm3SignalStrengthStateTypeId},
    {shellyHTThingClassId, shellyHTSignalStrengthStateTypeId},
    {shellySwitchThingClassId, shellySwitchSignalStrengthStateTypeId},
    {shellyGenericThingClassId, shellyGenericSignalStrengthStateTypeId},
    {shellyLightThingClassId, shellyLightSignalStrengthStateTypeId},
    {shellySocketThingClassId, shellySocketSignalStrengthStateTypeId},
    {shellyGenericPMThingClassId, shellyGenericPMSignalStrengthStateTypeId},
    {shellyLightPMThingClassId, shellyLightPMSignalStrengthStateTypeId},
    {shellySocketPMThingClassId, shellySocketPMSignalStrengthStateTypeId},
    {shellyRollerThingClassId, shellyRollerSignalStrengthStateTypeId},
    {shellyI3ThingClassId, shellyI3SignalStrengthStateTypeId},
    {shellyMotionThingClassId, shellyMotionSignalStrengthStateTypeId}
};

static QHash<ThingClassId, StateTypeId> powerStateTypeMap = {
    {shelly1ThingClassId, shelly1PowerStateTypeId},
    {shelly1pmThingClassId, shelly1pmPowerStateTypeId},
    {shelly1lThingClassId, shelly1lPowerStateTypeId},
    {shellyPlugThingClassId, shellyPlugPowerStateTypeId},
    {shellyRgbw2ThingClassId, shellyRgbw2PowerStateTypeId},
    {shellyDimmerThingClassId, shellyDimmerPowerStateTypeId},
    {shellyGenericThingClassId, shellyGenericPowerStateTypeId},
    {shellyLightThingClassId, shellyLightPowerStateTypeId},
    {shellySocketThingClassId, shellySocketPowerStateTypeId},
    {shellyGenericPMThingClassId, shellyGenericPMPowerStateTypeId},
    {shellyLightPMThingClassId, shellyLightPMPowerStateTypeId},
    {shellySocketPMThingClassId, shellySocketPMPowerStateTypeId},
    {shellyEm3ThingClassId, shellyEm3PowerStateTypeId},
};

static QHash<ThingClassId, StateTypeId> currentPowerStateTypeMap = {
    {shelly1pmThingClassId, shelly1pmCurrentPowerStateTypeId},
    {shelly1lThingClassId, shelly1lCurrentPowerStateTypeId},
    {shellyPlugThingClassId, shellyPlugCurrentPowerStateTypeId},
    {shellyRgbw2ThingClassId, shellyRgbw2CurrentPowerStateTypeId},
    {shellyDimmerThingClassId, shellyDimmerCurrentPowerStateTypeId},
    {shellyGenericPMThingClassId, shellyGenericPMCurrentPowerStateTypeId},
    {shellyLightPMThingClassId, shellyLightPMCurrentPowerStateTypeId},
    {shellySocketPMThingClassId, shellySocketPMCurrentPowerStateTypeId},
    {shellyRollerThingClassId, shellyRollerCurrentPowerStateTypeId},
    {shellyEm3ThingClassId, shellyEm3CurrentPowerStateTypeId},
};


static QHash<ThingClassId, StateTypeId> totalEnergyConsumedStateTypeMap = {
    {shellyPlugThingClassId, shellyPlugTotalEnergyConsumedStateTypeId},
    {shellyGenericPMThingClassId, shellyGenericPMTotalEnergyConsumedStateTypeId},
    {shellyLightPMThingClassId, shellyLightPMTotalEnergyConsumedStateTypeId},
    {shellySocketPMThingClassId, shellySocketPMTotalEnergyConsumedStateTypeId},
    {shellyRollerThingClassId, shellyRollerTotalEnergyConsumedStateTypeId},
    {shellyEm3ThingClassId, shellyEm3TotalEnergyConsumedStateTypeId},
};

static QHash<ThingClassId, StateTypeId> colorStateTypeMap = {
    {shellyRgbw2ThingClassId, shellyRgbw2ColorStateTypeId},
};

static QHash<ThingClassId, StateTypeId> colorTemperatureStateTypeMap = {
    {shellyRgbw2ThingClassId, shellyRgbw2ColorTemperatureStateTypeId},
};

static QHash<ThingClassId, StateTypeId> brightnessStateTypeMap = {
    {shellyRgbw2ThingClassId, shellyRgbw2BrightnessStateTypeId},
    {shellyDimmerThingClassId, shellyDimmerBrightnessStateTypeId},
};

static QHash<ThingClassId, StateTypeId> updateStatusStateTypesMap = {
    {shelly1ThingClassId, shelly1UpdateStatusStateTypeId},
    {shelly1pmThingClassId, shelly1pmUpdateStatusStateTypeId},
    {shelly1lThingClassId, shelly1lUpdateStatusStateTypeId},
    {shelly2ThingClassId, shelly2UpdateStatusStateTypeId},
    {shelly25ThingClassId, shelly25UpdateStatusStateTypeId},
    {shellyPlugThingClassId, shellyPlugUpdateStatusStateTypeId},
    {shellyRgbw2ThingClassId, shellyRgbw2UpdateStatusStateTypeId},
    {shellyDimmerThingClassId, shellyDimmerUpdateStatusStateTypeId},
    {shellyButton1ThingClassId, shellyButton1UpdateStatusStateTypeId},
    {shellyEm3ThingClassId, shellyEm3UpdateStatusStateTypeId},
    {shellyHTThingClassId, shellyHTUpdateStatusStateTypeId},
    {shellyI3ThingClassId, shellyI3UpdateStatusStateTypeId},
    {shellyMotionThingClassId, shellyMotionUpdateStatusStateTypeId}
};

static QHash<ThingClassId, StateTypeId> currentVersionStateTypesMap = {
    {shelly1ThingClassId, shelly1CurrentVersionStateTypeId},
    {shelly1pmThingClassId, shelly1pmCurrentVersionStateTypeId},
    {shelly1lThingClassId, shelly1lCurrentVersionStateTypeId},
    {shelly2ThingClassId, shelly2CurrentVersionStateTypeId},
    {shelly25ThingClassId, shelly25CurrentVersionStateTypeId},
    {shellyPlugThingClassId, shellyPlugCurrentVersionStateTypeId},
    {shellyRgbw2ThingClassId, shellyRgbw2CurrentVersionStateTypeId},
    {shellyDimmerThingClassId, shellyDimmerCurrentVersionStateTypeId},
    {shellyButton1ThingClassId, shellyButton1CurrentVersionStateTypeId},
    {shellyEm3ThingClassId, shellyEm3CurrentVersionStateTypeId},
    {shellyHTThingClassId, shellyHTCurrentVersionStateTypeId},
    {shellyI3ThingClassId, shellyI3CurrentVersionStateTypeId},
    {shellyMotionThingClassId, shellyMotionCurrentVersionStateTypeId}
};

static QHash<ThingClassId, StateTypeId> availableVersionStateTypesMap = {
    {shelly1ThingClassId, shelly1AvailableVersionStateTypeId},
    {shelly1pmThingClassId, shelly1pmAvailableVersionStateTypeId},
    {shelly1lThingClassId, shelly1lAvailableVersionStateTypeId},
    {shelly2ThingClassId, shelly2AvailableVersionStateTypeId},
    {shelly25ThingClassId, shelly25AvailableVersionStateTypeId},
    {shellyPlugThingClassId, shellyPlugAvailableVersionStateTypeId},
    {shellyRgbw2ThingClassId, shellyRgbw2AvailableVersionStateTypeId},
    {shellyDimmerThingClassId, shellyDimmerAvailableVersionStateTypeId},
    {shellyButton1ThingClassId, shellyButton1AvailableVersionStateTypeId},
    {shellyEm3ThingClassId, shellyEm3AvailableVersionStateTypeId},
    {shellyHTThingClassId, shellyHTAvailableVersionStateTypeId},
    {shellyI3ThingClassId, shellyI3AvailableVersionStateTypeId},
    {shellyMotionThingClassId, shellyMotionAvailableVersionStateTypeId}
};

static QHash<ThingClassId, StateTypeId> batteryLevelStateTypesMap = {
    {shellyButton1ThingClassId, shellyButton1BatteryLevelStateTypeId},
    {shellyHTThingClassId, shellyHTBatteryLevelStateTypeId},
    {shellyMotionThingClassId, shellyMotionBatteryLevelStateTypeId}
};

static QHash<ThingClassId, StateTypeId> batteryCriticalStateTypesMap = {
    {shellyButton1ThingClassId, shellyButton1BatteryCriticalStateTypeId},
    {shellyHTThingClassId, shellyHTBatteryCriticalStateTypeId},
    {shellyMotionThingClassId, shellyMotionBatteryCriticalStateTypeId}
};

static QHash<ThingClassId, StateTypeId> presenceStateTypesMap = {
    {shellyMotionThingClassId, shellyMotionIsPresentStateTypeId}
};

static QHash<ThingClassId, StateTypeId> lightIntensityStateTypesMap = {
    {shellyMotionThingClassId, shellyMotionLightIntensityStateTypeId}
};

static QHash<ThingClassId, StateTypeId> vibrationStateTypesMap = {
    {shellyMotionThingClassId, shellyMotionVibrationStateTypeId}
};

// Actions and their params
static QHash<ActionTypeId, ThingClassId> rebootActionTypeMap = {
    {shelly1RebootActionTypeId, shelly1ThingClassId},
    {shelly1pmRebootActionTypeId, shelly1pmThingClassId},
    {shelly1lRebootActionTypeId, shelly1lThingClassId},
    {shellyPlugRebootActionTypeId, shellyPlugThingClassId},
    {shellyRgbw2RebootActionTypeId, shellyRgbw2ThingClassId},
    {shellyDimmerRebootActionTypeId, shellyDimmerThingClassId},
    {shelly2RebootActionTypeId, shelly2ThingClassId},
    {shelly25RebootActionTypeId, shelly25ThingClassId},
    {shellyI3RebootActionTypeId, shellyI3ThingClassId},
};

static QHash<ActionTypeId, ThingClassId> powerActionTypesMap = {
    {shelly1PowerActionTypeId, shelly1ThingClassId},
    {shelly1pmPowerActionTypeId, shelly1pmThingClassId},
    {shelly1lPowerActionTypeId, shelly1lThingClassId},
    {shellyPlugPowerActionTypeId, shellyPlugThingClassId},
    {shellyGenericPowerActionTypeId, shellyGenericThingClassId},
    {shellyLightPowerActionTypeId, shellyLightThingClassId},
    {shellySocketPowerActionTypeId, shellySocketThingClassId},
    {shellyGenericPMPowerActionTypeId, shellyGenericPMThingClassId},
    {shellyLightPMPowerActionTypeId, shellyLightPMThingClassId},
    {shellySocketPMPowerActionTypeId, shellySocketPMThingClassId},
    {shellyEm3PowerActionTypeId, shellyEm3ThingClassId},
    {shelly25Channel1ActionTypeId, shelly25ThingClassId},
    {shelly25Channel2ActionTypeId, shelly25ThingClassId}
};

static QHash<ActionTypeId, ThingClassId> powerActionParamTypesMap = {
    {shelly1PowerActionTypeId, shelly1PowerActionPowerParamTypeId},
    {shelly1pmPowerActionTypeId, shelly1pmPowerActionPowerParamTypeId},
    {shelly1lPowerActionTypeId, shelly1lPowerActionPowerParamTypeId},
    {shellyPlugPowerActionTypeId, shellyPlugPowerActionPowerParamTypeId},
    {shellyGenericPowerActionTypeId, shellyGenericPowerActionPowerParamTypeId},
    {shellyLightPowerActionTypeId, shellyLightPowerActionPowerParamTypeId},
    {shellySocketPowerActionTypeId, shellySocketPowerActionPowerParamTypeId},
    {shellyGenericPMPowerActionTypeId, shellyGenericPMPowerActionPowerParamTypeId},
    {shellyLightPMPowerActionTypeId, shellyLightPMPowerActionPowerParamTypeId},
    {shellySocketPMPowerActionTypeId, shellySocketPMPowerActionPowerParamTypeId},
    {shellyEm3PowerActionTypeId, shellyEm3PowerActionPowerParamTypeId},
    {shelly25Channel1ActionTypeId, shelly25Channel1ActionChannel1ParamTypeId},
    {shelly25Channel2ActionTypeId, shelly25Channel2ActionChannel2ParamTypeId}
};

static QHash<ActionTypeId, ThingClassId> colorPowerActionTypesMap = {
    {shellyRgbw2PowerActionTypeId, shellyRgbw2ThingClassId},
};

static QHash<ActionTypeId, ThingClassId> colorPowerActionParamTypesMap = {
    {shellyRgbw2PowerActionPowerParamTypeId, shellyRgbw2PowerActionTypeId},
};

static QHash<ActionTypeId, ThingClassId> colorActionTypesMap = {
    {shellyRgbw2ColorActionTypeId, shellyRgbw2ThingClassId},
};

static QHash<ParamTypeId, ActionTypeId> colorActionParamTypesMap = {
    {shellyRgbw2ColorActionTypeId, shellyRgbw2ColorActionTypeId},
};

static QHash<ActionTypeId, ThingClassId> colorBrightnessActionTypesMap = {
    {shellyRgbw2BrightnessActionTypeId, shellyRgbw2ThingClassId},
};

static QHash<ParamTypeId, ActionTypeId> colorBrightnessActionParamTypesMap = {
    {shellyRgbw2BrightnessActionBrightnessParamTypeId, shellyRgbw2BrightnessActionTypeId},
};

static QHash<ActionTypeId, ThingClassId> colorTemperatureActionTypesMap = {
    {shellyRgbw2ColorTemperatureActionTypeId, shellyRgbw2ThingClassId},
};

static QHash<ActionTypeId, ThingClassId> colorTemperatureActionParamTypesMap = {
    {shellyRgbw2ColorTemperatureActionTypeId, shellyRgbw2ColorTemperatureActionColorTemperatureParamTypeId},
};

static QHash<ActionTypeId, ThingClassId> dimmablePowerActionTypesMap = {
    {shellyDimmerPowerActionTypeId, shellyDimmerThingClassId},
};

static QHash<ParamTypeId, ActionTypeId> dimmablePowerActionParamTypesMap = {
    {shellyDimmerPowerActionTypeId, shellyDimmerPowerActionPowerParamTypeId},
};

static QHash<ActionTypeId, ThingClassId> dimmableBrightnessActionTypesMap = {
    {shellyDimmerBrightnessActionTypeId, shellyDimmerThingClassId},
};

static QHash<ParamTypeId, ActionTypeId> dimmableBrightnessActionParamTypesMap = {
    {shellyDimmerBrightnessActionTypeId, shellyDimmerBrightnessActionBrightnessParamTypeId},
};

static QHash<ActionTypeId, ThingClassId> rollerOpenActionTypeMap = {
    {shellyRollerOpenActionTypeId, shellyRollerThingClassId},
};

static QHash<ActionTypeId, ThingClassId> rollerCloseActionTypeMap = {
    {shellyRollerCloseActionTypeId, shellyRollerThingClassId},
};

static QHash<ActionTypeId, ThingClassId> rollerStopActionTypeMap = {
    {shellyRollerStopActionTypeId, shellyRollerThingClassId},
};

static QHash<ActionTypeId, ThingClassId> updateActionTypesMap = {
    {shelly1PerformUpdateActionTypeId, shelly1ThingClassId},
    {shelly1pmPerformUpdateActionTypeId, shelly1pmThingClassId},
    {shelly1lPerformUpdateActionTypeId, shelly1lThingClassId},
    {shelly2PerformUpdateActionTypeId, shelly2ThingClassId},
    {shelly25PerformUpdateActionTypeId, shelly25ThingClassId},
    {shellyPlugPerformUpdateActionTypeId, shellyPlugThingClassId},
    {shellyRgbw2PerformUpdateActionTypeId, shellyRgbw2ThingClassId},
    {shellyDimmerPerformUpdateActionTypeId, shellyDimmerThingClassId},
    {shellyButton1PerformUpdateActionTypeId, shellyButton1ThingClassId},
    {shellyEm3PerformUpdateActionTypeId, shellyEm3ThingClassId},
    {shellyHTPerformUpdateActionTypeId, shellyHTThingClassId},
    {shellyI3PerformUpdateActionTypeId, shellyI3ThingClassId},
    {shellyMotionPerformUpdateActionTypeId, shellyMotionThingClassId}
};

// Settings
static QHash<ThingClassId, ParamTypeId> longpushMinDurationSettingIds = {
    {shellyI3ThingClassId, shellyI3SettingsLongpushMinDurationParamTypeId}
};
static QHash<ThingClassId, ParamTypeId> longpushMaxDurationSettingIds = {
    {shellyButton1ThingClassId, shellyButton1SettingsLongpushMaxDurationParamTypeId},
    {shellyI3ThingClassId, shellyI3SettingsLongpushMaxDurationParamTypeId}
};
static QHash<ThingClassId, ParamTypeId> multipushTimeBetweenPushesSettingIds = {
    {shellyButton1ThingClassId, shellyButton1SettingsMultipushTimeBetweenPushesParamTypeId},
    {shellyI3ThingClassId, shellyI3SettingsMultipushTimeBetweenPushesParamTypeId}
};

IntegrationPluginShelly::IntegrationPluginShelly()
{
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
        } else if (info->thingClassId() == shellyI3ThingClassId) {
            namePattern = QRegExp("shellyix3-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyMotionThingClassId) {
            namePattern = QRegExp("shellymotionsensor-[0-9A-Z]+$");
        }
        if (!entry.name().contains(namePattern)) {
            continue;
        }

        qCDebug(dcShelly()) << "Found shelly thing!" << entry;

        ThingDescriptor descriptor(info->thingClassId(), entry.name(), entry.hostAddress().toString());
        ParamList params;
        params << Param(idParamTypeMap.value(info->thingClassId()), entry.name());
        params << Param(usernameParamTypeMap.value(info->thingClassId()), "");
        params << Param(passwordParamTypeMap.value(info->thingClassId()), "");
        if (connectedDeviceParamTypeMap.contains(info->thingClassId())) {
            params << Param(connectedDeviceParamTypeMap.value(info->thingClassId()), "None");
        }
        if (connectedDevice2ParamTypeMap.contains(info->thingClassId())) {
            params << Param(connectedDevice2ParamTypeMap.value(info->thingClassId()), "None");
        }
        descriptor.setParams(params);

        Things existingThings = myThings().filterByParam(idParamTypeMap.value(info->thingClassId()), entry.name());
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

    if (idParamTypeMap.contains(thing->thingClassId())) {
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

    if (rebootActionTypeMap.contains(action.actionTypeId())) {
        QUrl url;
        url.setScheme("http");
        url.setHost(getIP(info->thing()));
        url.setPath("/reboot");
        url.setUserName(thing->paramValue(usernameParamTypeMap.value(thing->thingClassId())).toString());
        url.setPassword(thing->paramValue(passwordParamTypeMap.value(thing->thingClassId())).toString());
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply](){
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (updateActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = thing->paramValue(idParamTypeMap.value(thing->thingClassId())).toString();
        channel->publish(QString("shellies/%1/command").arg(shellyId), "update_fw");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (powerActionTypesMap.contains(action.actionTypeId())) {
        // If the main shelly has a power action (e.g. Shelly Plug, there is no parentId)
        Thing *parentDevice = thing->parentId().isNull() ? thing : myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(idParamTypeMap.value(parentDevice->thingClassId())).toString();
        int relay = 1;
        QHash<ActionTypeId, int> actionChannelMap = {
            {shelly25Channel1ActionTypeId, 1},
            {shelly25Channel2ActionTypeId, 2}
        };
        if (channelParamTypeMap.contains(thing->thingClassId())) {
            relay = thing->paramValue(channelParamTypeMap.value(thing->thingClassId())).toInt();
        } else if (actionChannelMap.contains(action.actionTypeId())) {
            relay = actionChannelMap.value(action.actionTypeId());
        }
        ParamTypeId powerParamTypeId = powerActionParamTypesMap.value(action.actionTypeId());
        bool on = action.param(powerParamTypeId).value().toBool();
        channel->publish(QString("shellies/%1/relay/%2/command").arg(shellyId).arg(relay - 1), on ? "on" : "off");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (colorPowerActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = info->thing()->paramValue(idParamTypeMap.value(info->thing()->thingClassId())).toString();
        ParamTypeId colorPowerParamTypeId = colorPowerActionParamTypesMap.value(action.actionTypeId());
        bool on = action.param(colorPowerParamTypeId).value().toBool();
        channel->publish("shellies/" + shellyId + "/color/0/command", on ? "on" : "off");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (colorActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = info->thing()->paramValue(idParamTypeMap.value(info->thing()->thingClassId())).toString();
        ParamTypeId colorParamTypeId = colorActionParamTypesMap.value(action.actionTypeId());
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

    if (colorTemperatureStateTypeMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = info->thing()->paramValue(idParamTypeMap.value(info->thing()->thingClassId())).toString();
        ParamTypeId colorTemperatureParamTypeId = colorTemperatureActionParamTypesMap.value(action.actionTypeId());
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

    if (colorBrightnessActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = info->thing()->paramValue(idParamTypeMap.value(info->thing()->thingClassId())).toString();
        ParamTypeId brightnessParamTypeId = colorBrightnessActionParamTypesMap.value(action.actionTypeId());
        int brightness = action.param(brightnessParamTypeId).value().toInt();
        QVariantMap data;
        data.insert("turn", "on"); // Should we really?
        data.insert("gain", brightness);
        QJsonDocument jsonDoc = QJsonDocument::fromVariant(data);
        channel->publish("shellies/" + shellyId + "/color/0/set", jsonDoc.toJson());
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (dimmablePowerActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = info->thing()->paramValue(idParamTypeMap.value(info->thing()->thingClassId())).toString();
        ParamTypeId powerParamTypeId = dimmablePowerActionParamTypesMap.value(action.actionTypeId());
        bool on = action.param(powerParamTypeId).value().toBool();
        channel->publish("shellies/" + shellyId + "/light/0/command", on ? "on" : "off");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (dimmableBrightnessActionTypesMap.contains(action.actionTypeId())) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = info->thing()->paramValue(idParamTypeMap.value(info->thing()->thingClassId())).toString();
        ParamTypeId brightnessParamTypeId = dimmableBrightnessActionParamTypesMap.value(action.actionTypeId());
        int brightness = action.param(brightnessParamTypeId).value().toInt();
        QVariantMap data;
        data.insert("turn", "on"); // Should we really?
        data.insert("brightness", brightness);
        QJsonDocument jsonDoc = QJsonDocument::fromVariant(data);
        channel->publish("shellies/" + shellyId + "/light/0/set", jsonDoc.toJson());
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (rollerOpenActionTypeMap.contains(action.actionTypeId())) {
        Thing *parentDevice = myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(idParamTypeMap.value(parentDevice->thingClassId())).toString();
        channel->publish("shellies/" + shellyId + "/roller/0/command", "open");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (rollerCloseActionTypeMap.contains(action.actionTypeId())) {
        Thing *parentDevice = myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(idParamTypeMap.value(parentDevice->thingClassId())).toString();
        channel->publish("shellies/" + shellyId + "/roller/0/command", "close");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (rollerStopActionTypeMap.contains(action.actionTypeId())) {
        Thing *parentDevice = myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(idParamTypeMap.value(parentDevice->thingClassId())).toString();
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
        QString shellyId = parentDevice->paramValue(idParamTypeMap.value(parentDevice->thingClassId())).toString();
        channel->publish("shellies/" + shellyId + "/roller/0/command", "rc");
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (action.actionTypeId() == shellyRollerPercentageActionTypeId) {
        Thing *parentDevice = myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(idParamTypeMap.value(parentDevice->thingClassId())).toString();
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
    thing->setStateValue(connectedStateTypesMap.value(thing->thingClassId()), true);

    foreach (Thing *child, myThings().filterByParentId(thing->id())) {
        child->setStateValue(connectedStateTypesMap[child->thingClassId()], true);
    }
}

void IntegrationPluginShelly::onClientDisconnected(MqttChannel *channel)
{
    Thing *thing = m_mqttChannels.key(channel);
    if (!thing) {
        qCWarning(dcShelly()) << "Received a client disconnect for a thing we don't know!";
        return;
    }
    thing->setStateValue(connectedStateTypesMap.value(thing->thingClassId()), false);

    foreach (Thing *child, myThings().filterByParentId(thing->id())) {
        child->setStateValue(connectedStateTypesMap[child->thingClassId()], false);
    }
}

void IntegrationPluginShelly::onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload)
{
    Thing *thing = m_mqttChannels.key(channel);
    if (!thing) {
        qCWarning(dcShelly()) << "Received a publish message for a thing we don't know!";
        return;
    }

    qCDebug(dcShelly()) << "Publish received from" << thing->name() << topic << payload;

    QString shellyId = thing->paramValue(idParamTypeMap.value(thing->thingClassId())).toString();
    if (topic == "shellies/" + shellyId + "/info") {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcShelly()) << "Failed to parse shelly info payload:" << error.errorString();
            qCWarning(dcShelly()) << qUtf8Printable(payload);
            return;
        }
        QVariantMap data = jsonDoc.toVariant().toMap();

        // Wifi signal strength
        int signalStrength = -1;
        if (data.value("wifi_sta").toMap().contains("rssi")) {
            int rssi = data.value("wifi_sta").toMap().value("rssi").toInt();
            signalStrength = qMin(100, qMax(0, (rssi + 100) * 2));
        }
        thing->setStateValue(signalStrengthStateTypesMap.value(thing->thingClassId()), signalStrength);
        foreach (Thing *child, myThings().filterByParentId(thing->id())) {
            child->setStateValue(signalStrengthStateTypesMap.value(child->thingClassId()), signalStrength);
        }

        // Firmware update
        QString updateStatus = updateStatusMap.value(data.value("update").toMap().value("status").toString());
        thing->setStateValue(updateStatusStateTypesMap.value(thing->thingClassId()), updateStatus);
        thing->setStateValue(currentVersionStateTypesMap.value(thing->thingClassId()), data.value("update").toMap().value("old_version").toString());
        thing->setStateValue(availableVersionStateTypesMap.value(thing->thingClassId()), data.value("update").toMap().value("new_version").toString());

        if (data.contains("longpush_duration_ms")) {
            if (longpushMinDurationSettingIds.contains(thing->thingClassId())) {
                thing->setSettingValue(longpushMinDurationSettingIds.value(thing->thingClassId()), data.value("longpush_duration_ms").toMap().value("min").toUInt());
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id())) {
                if (longpushMinDurationSettingIds.contains(child->thingClassId())) {
                    thing->setSettingValue(longpushMinDurationSettingIds.value(thing->thingClassId()), data.value("longpush_duration_ms").toMap().value("min").toUInt());
                }
            }
            if (longpushMaxDurationSettingIds.contains(thing->thingClassId())) {
                thing->setSettingValue(longpushMaxDurationSettingIds.value(thing->thingClassId()), data.value("longpush_duration_ms").toMap().value("max").toUInt());
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id())) {
                if (longpushMaxDurationSettingIds.contains(child->thingClassId())) {
                    thing->setSettingValue(longpushMaxDurationSettingIds.value(thing->thingClassId()), data.value("longpush_duration_ms").toMap().value("max").toUInt());
                }
            }
        }
        if (data.contains("multipush_time_between_pushes_ms")) {
            if (multipushTimeBetweenPushesSettingIds.contains(thing->thingClassId())) {
                thing->setSettingValue(multipushTimeBetweenPushesSettingIds.value(thing->thingClassId()), data.value("multipush_time_between_pushes_ms").toMap().value("max").toUInt());
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id())) {
                if (multipushTimeBetweenPushesSettingIds.contains(child->thingClassId())) {
                    thing->setSettingValue(multipushTimeBetweenPushesSettingIds.value(thing->thingClassId()), data.value("multipush_time_between_pushes_ms").toMap().value("max").toUInt());
                }
            }
        }


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
        int channel = topic.split("/").last().toInt();
        // "1" or "0"
        // Emit event button pressed
        bool on = payload == "1";
        if (thing->thingClassId() == shellyI3ThingClassId) {
            if (channel == 0) {
                thing->setStateValue(shellyI3Input1StateTypeId, on);
            } else if (channel == 1) {
                thing->setStateValue(shellyI3Input2StateTypeId, on);
            } else {
                thing->setStateValue(shellyI3Input3StateTypeId, on);
            }
            return;
        }
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
        QStringList parts = topic.split("/");
        int channel = parts.at(3).toInt();
        bool on = payload == "on";

        // If the shelly main thing has a power state (e.g. Shelly Plug)
        if (powerStateTypeMap.contains(thing->thingClassId())) {
            thing->setStateValue(powerStateTypeMap.value(thing->thingClassId()), on);
        }
        // If the shelly main thing has multiple channels (e.g. Shelly 2.5)
        if (thing->thingClassId() == shelly25ThingClassId) {
            QHash<int, StateTypeId> powerChannelStateTypesMap = {
                {0, shelly25Channel1StateTypeId},
                {1, shelly25Channel2StateTypeId}
            };
            thing->setStateValue(powerChannelStateTypesMap.value(channel), on);
        }

        // And switch all childs of this shelly too
        foreach (Thing *child, myThings().filterByParentId(thing->id())) {
            if (powerStateTypeMap.contains(child->thingClassId())) {
                ParamTypeId channelParamTypeId = channelParamTypeMap.value(child->thingClassId());
                if (child->paramValue(channelParamTypeId).toInt() == channel + 1) {
                    child->setStateValue(powerStateTypeMap.value(child->thingClassId()), on);
                }
            }
        }

    }

    topicMatcher = QRegExp("shellies/" + shellyId + "/(relay|roller)/[0-1]/power");
    if (topicMatcher.exactMatch(topic)) {
        QStringList parts = topic.split("/");
        int channel = parts.at(3).toInt();
        double power = payload.toDouble();
        // If this gateway thing supports power measuring (e.g. Shelly Plug S) set it directly here
        if (currentPowerStateTypeMap.contains(thing->thingClassId())) {
            thing->setStateValue(currentPowerStateTypeMap.value(thing->thingClassId()), power);
        }
        // For multi-channel devices, power measurements are per-channel, so, find the child thing
        foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByInterface("extendedsmartmeterconsumer")) {
            ParamTypeId channelParamTypeId = channelParamTypeMap.value(child->thingClassId());
            if (child->paramValue(channelParamTypeId).toInt() == channel + 1) {
                child->setStateValue(currentPowerStateTypeMap.value(child->thingClassId()), power);
            }
        }
    }

    topicMatcher = QRegExp("shellies/" + shellyId + "/(relay|roller)/[0-1]/energy");
    if (topicMatcher.exactMatch(topic)) {
        QStringList parts = topic.split("/");
        int channel = parts.at(3).toInt();
        // W/min => kW/h
        double energy = payload.toDouble() / 1000 / 60;
        // If this gateway thing supports energy measuring (e.g. Shelly Plug S) set it directly here
        if (totalEnergyConsumedStateTypeMap.contains(thing->thingClassId())) {
            thing->setStateValue(totalEnergyConsumedStateTypeMap.value(thing->thingClassId()), energy);
        }
        // For multi-channel devices, power measurements are per-channel, so, find the child thing
        foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByInterface("extendedsmartmeterconsumer")) {
            ParamTypeId channelParamTypeId = channelParamTypeMap.value(child->thingClassId());
            if (child->paramValue(channelParamTypeId).toInt() == channel + 1) {
                child->setStateValue(totalEnergyConsumedStateTypeMap.value(child->thingClassId()), energy);
            }
        }
    }

    if (topic == "shellies/" + shellyId + "/color/0") {
        bool on = payload == "on";
        if (powerStateTypeMap.contains(thing->thingClassId())) {
            thing->setStateValue(powerStateTypeMap.value(thing->thingClassId()), on);
        }
    }

    if (topic == "shellies/" + shellyId + "/color/0/status") {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcShelly()) << "Error parsing JSON from Shelly:" << error.error << error.errorString() << payload;
            return;
        }
        QVariantMap statusMap = jsonDoc.toVariant().toMap();
        if (colorStateTypeMap.contains(thing->thingClassId())) {
            QColor color = QColor(statusMap.value("red").toInt(), statusMap.value("green").toInt(), statusMap.value("blue").toInt());
            thing->setStateValue(colorStateTypeMap.value(thing->thingClassId()), color);
        }
        if (brightnessStateTypeMap.contains(thing->thingClassId())) {
            int brightness = statusMap.value("gain").toInt();
            thing->setStateValue(brightnessStateTypeMap.value(thing->thingClassId()), brightness);
        }
        if (currentPowerStateTypeMap.contains(thing->thingClassId())) {
            double power = statusMap.value("power").toDouble();
            thing->setStateValue(currentPowerStateTypeMap.value(thing->thingClassId()), power);
        }
    }

    if (topic == "shellies/" + shellyId + "/light/0") {
        bool on = payload == "on";
        if (powerStateTypeMap.contains(thing->thingClassId())) {
            thing->setStateValue(powerStateTypeMap.value(thing->thingClassId()), on);
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
        if (brightnessStateTypeMap.contains(thing->thingClassId())) {
            int brightness = statusMap.value("brightness").toInt();
            thing->setStateValue(brightnessStateTypeMap.value(thing->thingClassId()), brightness);
        }
    }

    if (topic == "shellies/" + shellyId + "/light/0/power") {
//        qCDebug(dcShelly()) << "Payload:" << payload;
        if (currentPowerStateTypeMap.contains(thing->thingClassId())) {
            double power = payload.toDouble();
            thing->setStateValue(currentPowerStateTypeMap.value(thing->thingClassId()), power);
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
        if (batteryLevelStateTypesMap.contains(thing->thingClassId())) {
            int batteryLevel = payload.toInt();
            thing->setStateValue(batteryLevelStateTypesMap.value(thing->thingClassId()), batteryLevel);
            thing->setStateValue(batteryCriticalStateTypesMap.value(thing->thingClassId()), batteryLevel < 10);
        }
    }

    if (topic.startsWith("shellies/" + shellyId + "/input_event/")) {
        qCDebug(dcShelly()) << "Payload:" << payload;
        if (thing->thingClassId() == shellyButton1ThingClassId) {  // it can be only at channel 0
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
        if (thing->thingClassId() == shellyI3ThingClassId) {
            int channel = topic.split("/").last().toInt();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcShelly()) << "Failed to parse JSON from shelly:" << error.errorString() << qUtf8Printable(payload);
                return;
            }

            QString buttonName = QString::number(channel + 1);
            QString event = jsonDoc.toVariant().toMap().value("event").toString();
            if (event == "S") {
                thing->emitEvent(shellyI3PressedEventTypeId, ParamList() << Param(shellyI3PressedEventButtonNameParamTypeId, buttonName) << Param(shellyI3PressedEventCountParamTypeId, 1));
            } else if (event == "L") {
                thing->emitEvent(shellyI3LongPressedEventTypeId, ParamList() << Param(shellyI3LongPressedEventButtonNameParamTypeId, buttonName));
            } else if (event == "SS") {
                thing->emitEvent(shellyI3PressedEventTypeId, ParamList() << Param(shellyI3PressedEventButtonNameParamTypeId, buttonName) << Param(shellyI3PressedEventCountParamTypeId, 2));
            } else if (event == "SSS") {
                thing->emitEvent(shellyI3PressedEventTypeId, ParamList() << Param(shellyI3PressedEventButtonNameParamTypeId, buttonName) << Param(shellyI3PressedEventCountParamTypeId, 3));
            } else if (event == "SL") {
                thing->emitEvent(shellyI3PressedEventTypeId, ParamList() << Param(shellyI3PressedEventButtonNameParamTypeId, buttonName) << Param(shellyI3PressedEventCountParamTypeId, 1));
                thing->emitEvent(shellyI3LongPressedEventTypeId, ParamList() << Param(shellyI3LongPressedEventButtonNameParamTypeId, buttonName));
            } else if (event == "LS") {
                thing->emitEvent(shellyI3LongPressedEventTypeId, ParamList() << Param(shellyI3LongPressedEventButtonNameParamTypeId, buttonName));
                thing->emitEvent(shellyI3PressedEventTypeId, ParamList() << Param(shellyI3PressedEventButtonNameParamTypeId, buttonName) << Param(shellyI3PressedEventCountParamTypeId, 1));
            } else {
                qCDebug(dcShelly()) << "Invalid button code from shelly I3:" << event;
            }
        }
    }

    topicMatcher = QRegExp("shellies/" + shellyId + "/emeter/[0-2]/.*");
    if (topicMatcher.exactMatch(topic) && thing->thingClassId() == shellyEm3ThingClassId) {
        int channel = topic.split('/').at(3).toInt();
        QString stateName = topic.split('/').at(4);
        QVariant value = payload;
        QHash<int, QHash<QString, StateTypeId>> channelMap;
        QHash<QString, StateTypeId> stateTypeIdMap;
        stateTypeIdMap["power"] = shellyEm3CurrentPowerPhaseAStateTypeId;
        stateTypeIdMap["pf"] = shellyEm3PowerFactorPhaseAStateTypeId;
        stateTypeIdMap["current"] = shellyEm3CurrentPhaseAStateTypeId;
        stateTypeIdMap["voltage"] = shellyEm3VoltagePhaseAStateTypeId;
        stateTypeIdMap["total"] = shellyEm3EnergyConsumedPhaseAStateTypeId;
        stateTypeIdMap["total_returned"] = shellyEm3EnergyProducedPhaseAStateTypeId;
        channelMap[0] = stateTypeIdMap;
        stateTypeIdMap["power"] = shellyEm3CurrentPowerPhaseBStateTypeId;
        stateTypeIdMap["pf"] = shellyEm3PowerFactorPhaseBStateTypeId;
        stateTypeIdMap["current"] = shellyEm3CurrentPhaseBStateTypeId;
        stateTypeIdMap["voltage"] = shellyEm3VoltagePhaseBStateTypeId;
        stateTypeIdMap["total"] = shellyEm3EnergyConsumedPhaseBStateTypeId;
        stateTypeIdMap["total_returned"] = shellyEm3EnergyProducedPhaseBStateTypeId;
        channelMap[1] = stateTypeIdMap;
        stateTypeIdMap["power"] = shellyEm3CurrentPowerPhaseCStateTypeId;
        stateTypeIdMap["pf"] = shellyEm3PowerFactorPhaseCStateTypeId;
        stateTypeIdMap["current"] = shellyEm3CurrentPhaseCStateTypeId;
        stateTypeIdMap["voltage"] = shellyEm3VoltagePhaseCStateTypeId;
        stateTypeIdMap["total"] = shellyEm3EnergyConsumedPhaseCStateTypeId;
        stateTypeIdMap["total_returned"] = shellyEm3EnergyProducedPhaseCStateTypeId;
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
            double grandTotal = thing->stateValue(shellyEm3EnergyConsumedPhaseAStateTypeId).toDouble();
            grandTotal += thing->stateValue(shellyEm3EnergyConsumedPhaseBStateTypeId).toDouble();
            grandTotal += thing->stateValue(shellyEm3EnergyConsumedPhaseCStateTypeId).toDouble();
            thing->setStateValue(shellyEm3TotalEnergyConsumedStateTypeId, grandTotal);
            double grandTotalReturned = thing->stateValue(shellyEm3EnergyProducedPhaseAStateTypeId).toDouble();
            grandTotalReturned += thing->stateValue(shellyEm3EnergyProducedPhaseBStateTypeId).toDouble();
            grandTotalReturned += thing->stateValue(shellyEm3EnergyProducedPhaseCStateTypeId).toDouble();
            thing->setStateValue(shellyEm3TotalEnergyProducedStateTypeId, grandTotalReturned);
            double totalPower = thing->stateValue(shellyEm3CurrentPowerPhaseAStateTypeId).toDouble();
            totalPower += thing->stateValue(shellyEm3CurrentPowerPhaseBStateTypeId).toDouble();
            totalPower += thing->stateValue(shellyEm3CurrentPowerPhaseCStateTypeId).toDouble();
            thing->setStateValue(shellyEm3CurrentPowerStateTypeId, totalPower);
        }
    }

    if (topic == "shellies/" + shellyId + "/status") {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcShelly()) << "Error parsing JSON from Shelly:" << error.error << error.errorString() << payload;
            return;
        }
        //        qCDebug(dcShelly()) << "Payload:" << qUtf8Printable(jsonDoc.toJson());
        QVariantMap statusMap = jsonDoc.toVariant().toMap();

        if (presenceStateTypesMap.contains(thing->thingClassId())) {
            thing->setStateValue(presenceStateTypesMap.value(thing->thingClassId()), statusMap.value("motion").toBool());
        }
        if (lightIntensityStateTypesMap.contains(thing->thingClassId())) {
            thing->setStateValue(lightIntensityStateTypesMap.value(thing->thingClassId()), statusMap.value("lux").toDouble());
        }
        if (vibrationStateTypesMap.contains(thing->thingClassId())) {
            thing->setStateValue(vibrationStateTypesMap.value(thing->thingClassId()), statusMap.value("vibration").toBool());
        }
        if (batteryLevelStateTypesMap.contains(thing->thingClassId()) && statusMap.contains("bat")) {
            thing->setStateValue(batteryLevelStateTypesMap.value(thing->thingClassId()), statusMap.value("bat").toMap().value("value").toInt());
            thing->setStateValue(batteryCriticalStateTypesMap.value(thing->thingClassId()), statusMap.value("bat").toMap().value("value").toInt() < 10);
        }
    }
}

void IntegrationPluginShelly::updateStatus()
{
    foreach (Thing *thing, m_mqttChannels.keys()) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        QString shellyId = thing->paramValue(idParamTypeMap.value(thing->thingClassId())).toString();
        qCDebug(dcShelly()) << "Requesting announcement" << QString("shellies/%1/info").arg(shellyId);
        channel->publish(QString("shellies/%1/command").arg(shellyId), "announce");
    }
}

void IntegrationPluginShelly::setupShellyGateway(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QString shellyId = info->thing()->paramValue(idParamTypeMap.value(info->thing()->thingClassId())).toString();
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
        QString connectedDevice1 = info->thing()->paramValue(connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString();
        QString connectedDevice2 = info->thing()->paramValue(connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString();
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

    if (!channel) {
        qCWarning(dcShelly()) << "Failed to create MQTT channel.";
        return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error creating MQTT channel. Please check MQTT server settings."));
    }

    m_mqttChannels.insert(info->thing(), channel);
    connect(channel, &MqttChannel::clientConnected, this, &IntegrationPluginShelly::onClientConnected);
    connect(channel, &MqttChannel::clientDisconnected, this, &IntegrationPluginShelly::onClientDisconnected);
    connect(channel, &MqttChannel::publishReceived, this, &IntegrationPluginShelly::onPublishReceived);

    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());
    url.setPort(80);
    url.setPath("/settings");
    url.setUserName(info->thing()->paramValue(usernameParamTypeMap.value(info->thing()->thingClassId())).toString());
    url.setPassword(info->thing()->paramValue(passwordParamTypeMap.value(info->thing()->thingClassId())).toString());

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
            info->thing()->setSettingValue(shellyButton1SettingsMultipushTimeBetweenPushesParamTypeId, settingsMap.value("multipush_time_between_pushes_ms").toMap().value("max").toUInt());
        } else  if (info->thing()->thingClassId() == shellyI3ThingClassId) {
            info->thing()->setSettingValue(shellyI3SettingsLongpushMinDurationParamTypeId, settingsMap.value("longpush_duration_ms").toMap().value("min").toUInt());
            info->thing()->setSettingValue(shellyI3SettingsLongpushMaxDurationParamTypeId, settingsMap.value("longpush_duration_ms").toMap().value("max").toUInt());
            info->thing()->setSettingValue(shellyI3SettingsMultipushTimeBetweenPushesParamTypeId, settingsMap.value("multipush_time_between_pushes_ms").toMap().value("max").toUInt());
        }

        ThingDescriptors autoChilds;

        // Autogenerate some childs if this thing has no childs yet
        if (myThings().filterByParentId(info->thing()->id()).isEmpty()) {
            // Always create the switch thing if we don't have one yet for shellies with input (1, 1pm etc)
            if (info->thing()->thingClassId() == shelly1ThingClassId
                    || info->thing()->thingClassId() == shelly1pmThingClassId) {
                ThingDescriptor switchChild(shellySwitchThingClassId, info->thing()->name() + " switch", QString(), info->thing()->id());
                switchChild.setParams(ParamList() << Param(shellySwitchThingChannelParamTypeId, 1));
                autoChilds.append(switchChild);
            }

            // Create 2 switches for some that have 2
            if (info->thing()->thingClassId() == shelly2ThingClassId
                    || info->thing()->thingClassId() == shelly25ThingClassId
                    || info->thing()->thingClassId() == shellyDimmerThingClassId
                    || info->thing()->thingClassId() == shelly1lThingClassId) {
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
                if (info->thing()->paramValue(connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Generic") {
                    ThingDescriptor genericChild(shellyGenericThingClassId, info->thing()->name() + " connected device", QString(), info->thing()->id());
                    genericChild.setParams(ParamList() << Param(shellyGenericThingChannelParamTypeId, 1));
                    autoChilds.append(genericChild);
                }
                if (info->thing()->paramValue(connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Light") {
                    ThingDescriptor lightChild(shellyLightThingClassId, info->thing()->name() + " connected light", QString(), info->thing()->id());
                    lightChild.setParams(ParamList() << Param(shellyLightThingChannelParamTypeId, 1));
                    autoChilds.append(lightChild);
                }
                if (info->thing()->paramValue(connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Socket") {
                    ThingDescriptor socketChild(shellySocketThingClassId, info->thing()->name() + " connected socket", QString(), info->thing()->id());
                    socketChild.setParams(ParamList() << Param(shellySocketThingChannelParamTypeId, 1));
                    autoChilds.append(socketChild);
                }
                // PM devices for shelly 1 pm and 2.5
            } else if (info->thing()->thingClassId() == shelly1pmThingClassId
                       || info->thing()->thingClassId() == shelly25ThingClassId) {
                if (info->thing()->paramValue(connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Generic") {
                    ThingDescriptor genericChild(shellyGenericPMThingClassId, info->thing()->name() + " connected device", QString(), info->thing()->id());
                    genericChild.setParams(ParamList() << Param(shellyGenericPMThingChannelParamTypeId, 1));
                    autoChilds.append(genericChild);
                }
                if (info->thing()->paramValue(connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Light") {
                    ThingDescriptor lightChild(shellyLightPMThingClassId, info->thing()->name() + " connected light", QString(), info->thing()->id());
                    lightChild.setParams(ParamList() << Param(shellyLightPMThingChannelParamTypeId, 1));
                    autoChilds.append(lightChild);
                }
                if (info->thing()->paramValue(connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Socket") {
                    ThingDescriptor socketChild(shellySocketPMThingClassId, info->thing()->name() + " connected socket", QString(), info->thing()->id());
                    socketChild.setParams(ParamList() << Param(shellySocketPMThingChannelParamTypeId, 1));
                    autoChilds.append(socketChild);
                }

                // Second channel for shelly 2 (no power metering)
                if (info->thing()->thingClassId() == shelly2ThingClassId) {
                    if (info->thing()->paramValue(connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Generic") {
                        ThingDescriptor genericChild(shellyGenericThingClassId, info->thing()->name() + " connected thing 2", QString(), info->thing()->id());
                        genericChild.setParams(ParamList() << Param(shellyGenericThingChannelParamTypeId, 2));
                        autoChilds.append(genericChild);
                    }
                    if (info->thing()->paramValue(connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Light") {
                        ThingDescriptor lightChild(shellyLightThingClassId, info->thing()->name() + " connected light 2", QString(), info->thing()->id());
                        lightChild.setParams(ParamList() << Param(shellyLightThingChannelParamTypeId, 2));
                        autoChilds.append(lightChild);
                    }
                    if (info->thing()->paramValue(connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Socket") {
                        ThingDescriptor socketChild(shellySocketThingClassId, info->thing()->name() + " connected socket 2", QString(), info->thing()->id());
                        socketChild.setParams(ParamList() << Param(shellySocketThingChannelParamTypeId, 2));
                        autoChilds.append(socketChild);
                    }
                }

                // Second channel for shelly 2.5 (with power metering)
                if (info->thing()->thingClassId() == shelly25ThingClassId) {
                    if (info->thing()->paramValue(connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Generic") {
                        ThingDescriptor genericChild(shellyGenericPMThingClassId, info->thing()->name() + " connected thing 2", QString(), info->thing()->id());
                        genericChild.setParams(ParamList() << Param(shellyGenericPMThingChannelParamTypeId, 2));
                        autoChilds.append(genericChild);
                    }
                    if (info->thing()->paramValue(connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Light") {
                        ThingDescriptor lightChild(shellyLightPMThingClassId, info->thing()->name() + " connected light 2", QString(), info->thing()->id());
                        lightChild.setParams(ParamList() << Param(shellyLightPMThingChannelParamTypeId, 2));
                        autoChilds.append(lightChild);
                    }
                    if (info->thing()->paramValue(connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Socket") {
                        ThingDescriptor socketChild(shellySocketPMThingClassId, info->thing()->name() + " connected socket 2", QString(), info->thing()->id());
                        socketChild.setParams(ParamList() << Param(shellySocketPMThingChannelParamTypeId, 2));
                        autoChilds.append(socketChild);
                    }
                }

                // And finally the special roller shutter mode
                if (info->thing()->thingClassId() == shelly2ThingClassId
                        || info->thing()->thingClassId() == shelly25ThingClassId) {
                    if (info->thing()->paramValue(connectedDeviceParamTypeMap.value(info->thing()->thingClassId())).toString() == "Roller Shutter Up"
                            && info->thing()->paramValue(connectedDevice2ParamTypeMap.value(info->thing()->thingClassId())).toString() == "Roller Shutter Down") {
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
        QString username = info->thing()->paramValue(usernameParamTypeMap.value(info->thing()->thingClassId())).toString();
        QString password = info->thing()->paramValue(passwordParamTypeMap.value(info->thing()->thingClassId())).toString();
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
            info->thing()->thingClassId() == shellyButton1ThingClassId ||
            info->thing()->thingClassId() == shellyI3ThingClassId) {
        connect(info->thing(), &Thing::settingChanged, this, [this, thing, shellyId](const ParamTypeId &settingTypeId, const QVariant &value) {

            pluginStorage()->beginGroup(thing->id().toString());
            QString address = pluginStorage()->value("cachedAddress").toString();
            pluginStorage()->endGroup();

            QUrl url;
            url.setScheme("http");
            url.setHost(address);
            url.setPort(80);
            url.setUserName(thing->paramValue(usernameParamTypeMap.value(thing->thingClassId())).toString());
            url.setPassword(thing->paramValue(passwordParamTypeMap.value(thing->thingClassId())).toString());

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
            } else if (settingTypeId == shellyI3SettingsLongpushMinDurationParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("longpush_duration_ms_min", value.toString());
            } else if (settingTypeId == shellyButton1SettingsLongpushMaxDurationParamTypeId
                       || settingTypeId == shellyI3SettingsLongpushMaxDurationParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("longpush_duration_ms_max", value.toString());
            } else if (settingTypeId == shellyButton1SettingsMultipushTimeBetweenPushesParamTypeId
                       || settingTypeId == shellyI3SettingsMultipushTimeBetweenPushesParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("multipush_time_between_pushes_ms_max", value.toString());
            }

            url.setQuery(query);

            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            qCDebug(dcShelly()) << "Setting configuration:" << url.toString();
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
        url.setPath(QString("/settings/relay/%1").arg(thing->paramValue(channelParamTypeMap.value(thing->thingClassId())).toInt() + 1));
        url.setUserName(parentDevice->paramValue(usernameParamTypeMap.value(parentDevice->thingClassId())).toString());
        url.setPassword(parentDevice->paramValue(passwordParamTypeMap.value(parentDevice->thingClassId())).toString());

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
