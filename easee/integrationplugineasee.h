/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2023, nymea GmbH
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

#ifndef INTEGRATIONPLUGINEASEE_H
#define INTEGRATIONPLUGINEASEE_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>

#include "extern-plugininfo.h"

#include <QTimer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QWebSocket>

class SignalRConnection;

class IntegrationPluginEasee: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugineasee.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    enum ObservationPoint {
        ObservationPointSelfTestResult = 1,
        ObservationPointSelfTestDetails = 2,
        ObservationPointWiFiEvent = 10,
        ObservationPointChargerOfflineReason = 11,
        ObservationPointEaseeLinkCommandResponse = 13,
        ObservationPointEaseeLinkDataReceived = 14,
        ObservationPointLocalPreAuthorizeEnabled = 15,
        ObservationPointLocalAuthorizeOfflineEnabled = 16,
        ObservationPointAllowOfflineTxForUnknownId = 17,
        ObservationPointErraticEvMaxToggles = 18,
        ObservationPointBackplateType = 19,
        ObservationPointSiteStructure = 20,
        ObservationPointDetectedPowerGridType = 21,
        ObservationPointCircuitMaxCurrentP1 = 22,
        ObservationPointCircuitMaxCurrentP2 = 23,
        ObservationPointCircuitMaxCurrentP3 = 24,
        ObservationPointLocation = 25,
        ObservationPointSiteIdString = 26,
        ObservationPointSiteIdNumeric = 27,
        ObservationPointLockCablePermanently = 30,
        ObservationPointIsEnabled = 31,
        ObservationPointCircuitSequenceNumber = 33,
        ObservationPointSinglePhaseNumber = 34,
        ObservationPointEnable3PhasesDeprecated = 35,
        ObservationPointWiFiSSID = 36,
        ObservationPointEnableIdCurrent = 37,
        ObservationPointPhaseMode = 38,
        ObservationPointForced3PhaseOnITWithGndFault = 39,
        ObservationPointLedStripBrightness = 40,
        ObservationPointLocalAuthorizationRequired = 41,
        ObservationPointAuthorizationRequired = 42,
        ObservationPointRemoteStartRequired = 43,
        ObservationPointSmartButtonEnabled = 44,
        ObservationPointOfflineChargingMode = 45,
        ObservationPointLedMode = 46,
        ObservationPointMaxChargerCurrent = 47,
        ObservationPointDynamicChargerCurrent = 48,
        ObservationPointMaxCurrentOfflineFallbackP1 = 50,
        ObservationPointMaxCurrentOfflineFallbackP2 = 51,
        ObservationPointMaxCurrentOfflineFallbackP3 = 52,
        ObservationPointListenToControlPulse = 56,
        ObservationPointControlPulseRTT = 57,
        ObservationPointChargingSchedule = 62,
        ObservationPointPairedEqualizer = 65,
        ObservationPointWiFiApEnabled = 68,
        ObservationPointPairedUserIdToken = 69,
        ObservationPointCircuitTotalAllocatedPhaseConductorCurrentL1 = 70,
        ObservationPointCircuitTotalAllocatedPhaseConductorCurrentL2 = 71,
        ObservationPointCircuitTotalAllocatedPhaseConductorCurrentL3 = 72,
        ObservationPointCircuitAllocatedPhaseConductorCurrentL1 = 73,
        ObservationPointCircuitAllocatedPhaseConductorCurrentL2 = 74,
        ObservationPointCircuitAllocatedPhaseConductorCurrentL3 = 75,
        ObservationPointNumberOfCarsConnected = 76,
        ObservationPointNumberOfCarsCharging = 77,
        ObservationPointNumberOfCarsInQueue = 78,
        ObservationPointNumberOfCarsFullyCharged = 79,
        ObservationPointSoftwareRelease = 80,
        ObservationPointICCID = 81,
        ObservationPointModemFwId = 82,
        ObservationPointOTAErrorCode = 83,
        ObservationPointMobileNetworkOperator = 84,
        ObservationPointRebootReason = 89,
        ObservationPointPowerPCBVersion = 90,
        ObservationPointCOMPCBVersion = 91,
        ObservationPointReasonForNoCurrent = 96,
        ObservationPointLoadBalancingNumberOfConnectedCharger = 97,
        ObservationPointUDPNumOfConnectedNodes = 98,
        ObservationPointLocalConnection = 99,
        ObservationPointPilotMode = 100,
        ObservationPointCarConnectedDeprecated = 101,
        ObservationPointSmartCharging = 102,
        ObservationPointCableLocked = 103,
        ObservationPointCableRating = 104,
        ObservationPointPilotHigh = 105,
        ObservationPointPilotLow = 106,
        ObservationPointBackPlateId = 107,
        ObservationPointUserIdTokenReversed = 108,
        ObservationPointChargerOpMode = 109,
        ObservationPointOutputPhase = 110,
        ObservationPointDynamicCircuitCurrentP1 = 111,
        ObservationPointDynamicCircuitCurrentP2 = 112,
        ObservationPointDynamicCircuitCurrentP3 = 113,
        ObservationPointOutputCurrent = 114,
        ObservationPointDeratedCurrent = 115,
        ObservationPointDeratingActive = 116,
        ObservationPointDebugString = 117,
        ObservationPointErrorString = 118,
        ObservationPointErrorCode = 119,
        ObservationPointTotalPower = 120,
        ObservationPointSessionEnergy = 121,
        ObservationPointEnergyPerHour = 122,
        ObservationPointLegacyEVStatus = 123,
        ObservationPointLifetimeEnergy = 124,
        ObservationPointLifetimeRelaySwitches = 125,
        ObservationPointLifetimeHours = 126,
        ObservationPointDynamicCurrentOfflineFallbackDeprecated = 127,
        ObservationPointUserIdToken = 128,
        ObservationPointChargingSession = 129,
        ObservationPointCellRSSI = 130,
        ObservationPointCellRAT = 131,
        ObservationPointWiFiRSSI = 132,
        ObservationPointCellAddress = 133,
        ObservationPointWiFiAddress = 134,
        ObservationPointWiFiType = 135,
        ObservationPointLocalRSSI = 136,
        ObservationPointMasterBackplateId = 137,
        ObservationPointLocalTXPower = 138,
        ObservationPointLocalState = 139,
        ObservationPointFoundWiFi = 140,
        ObservationPointChargerRAT = 141,
        ObservationPointCellularInterfaceErrorCount = 142,
        ObservationPointCellularInterfaceResetCount = 143,
        ObservationPointWiFiInterfaceErrorCount = 144,
        ObservationPointWiFiInterfaceResetCount = 145,
        ObservationPointLocalNodeType = 146,
        ObservationPointLocalRadioChannel = 147,
        ObservationPointLocalShortAddress = 148,
        ObservationPointLocalParentAddrOrNumOfNodes = 149,
        ObservationPointTempMax = 150,
        ObservationPointTempAmbientPowerBoard = 151,
        ObservationPointTempInputT2 = 152,
        ObservationPointTempInputT3 = 153,
        ObservationPointTempInputT4 = 154,
        ObservationPointTempInputT5 = 155,
        ObservationPointTempOutputN = 160,
        ObservationPointTempOutputL1 = 161,
        ObservationPointTempOutputL2 = 162,
        ObservationPointTempOutputL3 = 163,
        ObservationPointTempAmbient = 170,
        ObservationPointLightAmbient = 171,
        ObservationPointIntRelHumidity = 172,
        ObservationPointBackplateLocked = 173,
        ObservationPointCurrentMotor = 174,
        ObservationPointBackplateHallSensor = 175,
        ObservationPointInCurrentT2 = 182,
        ObservationPointInCurrentT3 = 183,
        ObservationPointInCurrentT4 = 184,
        ObservationPointInCurrentT5 = 185,
        ObservationPointInVoltT1T2 = 190,
        ObservationPointInVoltT1T3 = 191,
        ObservationPointInVoltT1T4 = 192,
        ObservationPointInVoltT1T5 = 193,
        ObservationPointInVoltT2T3 = 194,
        ObservationPointInVoltT2T4 = 195,
        ObservationPointInVoltT2T5 = 196,
        ObservationPointInVoltT3T4 = 197,
        ObservationPointInVoltT3T5 = 198,
        ObservationPointInVoltT4T5 = 199,
        ObservationPointOutVoltPin12 = 202,
        ObservationPointOutVoltPin13 = 203,
        ObservationPointOutVoltPin14 = 204,
        ObservationPointOutVoltPin15 = 205,
        ObservationPointVoltLevel33 = 210,
        ObservationPointVoltLevel5 = 211,
        ObservationPointVoltLevel12 = 212,
        ObservationPointLTERSRP = 220,
        ObservationPointLTESINR = 221,
        ObservationPointLTERSRQ = 222,
        ObservationPointEQAvailableCurrentP1 = 230,
        ObservationPointEQAvailableCurrentP2 = 231,
        ObservationPointEQAvailableCurrentP3 = 232
    };
    Q_ENUM(ObservationPoint)

    explicit IntegrationPluginEasee();
    ~IntegrationPluginEasee();

//    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

private:
    QNetworkRequest createRequest(Thing *thing, const QString &endpoint);
    QNetworkReply *refreshToken(Thing *thing);
    void refreshProducts(Thing *account);
    void refreshCurrentState(Thing *charger);

    QHash<Thing*, SignalRConnection*> m_signalRConnections;
    QHash<QString, uint> m_circuitIds; // chargerId, circuitId
    QHash<QString, uint> m_siteIds; // chargerId, siteId

    PluginTimer *m_timer = nullptr;
};

#endif // INTEGRATIONPLUGINEASEE_H
