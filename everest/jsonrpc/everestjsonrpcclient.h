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

#ifndef EVERESTJSONRPCCLIENT_H
#define EVERESTJSONRPCCLIENT_H

#include <QObject>

#include <integrations/thing.h>
#include <network/macaddress.h>
#include <network/networkdevicemonitor.h>

#include "everestjsonrpcreply.h"
#include "everestjsonrpcinterface.h"

class EverestJsonRpcClient : public QObject
{
    Q_OBJECT
public:
    // API Enums

    enum ResponseError {
        ResponseErrorNoError = 0,
        ResponseErrorErrorInvalidParameter,
        ResponseErrorErrorOutOfRange,
        ResponseErrorErrorValuesNotApplied,
        ResponseErrorErrorInvalidEVSEIndex,
        ResponseErrorErrorInvalidConnectorID,
        ResponseErrorErrorNoDataAvailable,
        ResponseErrorErrorUnknownError
    };
    Q_ENUM(ResponseError)

    enum ConnectorType {
        ConnectorTypecCCS1,
        ConnectorTypecCCS2,
        ConnectorTypecG105,
        ConnectorTypecTesla,
        ConnectorTypecType1,
        ConnectorTypecType2,
        ConnectorTypes309_1P_16A,
        ConnectorTypes309_1P_32A,
        ConnectorTypes309_3P_16A,
        ConnectorTypes309_3P_32A,
        ConnectorTypesBS1361,
        ConnectorTypesCEE_7_7,
        ConnectorTypesType2,
        ConnectorTypesType3,
        ConnectorTypeOther1PhMax16A,
        ConnectorTypeOther1PhOver16A,
        ConnectorTypeOther3Ph,
        ConnectorTypePan,
        ConnectorTypewInductive,
        ConnectorTypewResonant,
        ConnectorTypeUndetermined
    };
    Q_ENUM(ConnectorType)

    enum ChargeProtocol {
        ChargeProtocolUnknown,
        ChargeProtocolIEC61851,
        ChargeProtocolDIN70121,
        ChargeProtocolISO15118,
        ChargeProtocolISO15118_20
    };
    Q_ENUM(ChargeProtocol)

    enum EvseState {
        EvseStateUnplugged,
        EvseStateDisabled,
        EvseStatePreparing,
        EvseStateReserved,
        EvseStateAuthRequired,
        EvseStateWaitingForEnergy,
        EvseStateCharging,
        EvseStateChargingPausedEV,
        EvseStateChargingPausedEVSE,
        EvseStateFinished
    };
    Q_ENUM(EvseState)

    // API Objects

    typedef struct ChargerInfo {
        QString vendor;
        QString model;
        QString serialNumber;
        QString firmwareVersion;
    } ChargerInfo;

    typedef struct ConnectorInfo {
        int connectorId = -1;
        ConnectorType type = ConnectorTypeUndetermined;
        QString description; // optional
    } ConnectorInfo;

    typedef struct EVSEInfo {
        int index = -1;
        QString id;
        QString description; // optional
        bool bidirectionalCharging = false;
        QList<ConnectorInfo> availableConnectors;
    } EVSEInfo;

    typedef struct ACChargeStatus {
        int activePhaseCount = 3;
    } ACChargeStatus;

    typedef struct ACChargeParameters {
        // V 1.0.0 supported values
        double maxCurrent = 0; // A
        double maxPhaseCount = 0;

        // double maxChargePower = 0; // W
        // double minChargePower = 0; // W
        // double nominalFrequency = 0; // Hz

    } ACChargeParameters;

    typedef struct EVSEStatus {
        double chargedEnergyWh = 0;
        double dischargedEnergyWh = 0;
        int chargingDuration = 0; // seconds
        bool chargingAllowed = false;
        bool available = false;
        int activeConnectorIndex = -1;
        bool errorPresent = false;
        ChargeProtocol chargeProtocol = ChargeProtocolUnknown;
        EvseState evseState = EvseStateUnplugged;
        QString evseStateString;
        ACChargeStatus acChargeStatus;
        ACChargeParameters acChargeParameters;
        // TODO:
        // o: "ac_charge_param": "$ACChargeParametersObj",
        // o: "dc_charge_param": "$DCChargeParametersObj",
        // o: "ac_charge_loop": "$ACChargeLoopObj",
        // o: "dc_charge_loop": "$DCChargeLoopObj",
        // o: display_parameters: "$DisplayParametersObj",

    } EVSEStatus;


    typedef struct HardwareCapabilities {
        double maxCurrentExport = 0;
        double maxCurrentImport = 0;
        int maxPhaseCountExport = 0;
        int maxPhaseCountImport = 0;
        double minCurrentExport = 0;
        double minCurrentImport = 0;
        int minPhaseCountExport = 0;
        int minPhaseCountImport = 0;
        bool phaseSwitchDuringCharging = false;
    } HardwareCapabilities;


    /*

    MeterDataObj {

        o: "current_A": {
            "L1": "float",
            "L2": "float",
            "L3": "float",
            "N": "float"
        },
        "energy_Wh_import": {
            "L1": "float",
            "L2": "float",
            "L3": "float",
            "total": "float"
        },
        o: "energy_Wh_export": {
            "L1": "float",
            "L2": "float",
            "L3": "float",
            "total": "float"
        },
        o: "frequency_Hz": {
            "L1": "float",
            "L2": "float",
            "L3": "float"
        },
        "meter_id": "string",
        o: "serial_number": "string",
        o: "phase_seq_error": "bool",
        o: "power_W": {
            "L1": "float",
            "L2": "float",
            "L3": "float",
            "total": "float"
        },
        "timestamp": "string",
        o: "voltage_V": {
            "L1": "float",
            "L2": "float",
            "L3": "float"
    }

     */

    typedef struct MeterData {

    } MeterData;


    explicit EverestJsonRpcClient(QObject *parent = nullptr);

    QUrl serverUrl();

    bool available() const;

    // Once available, following properties are set
    QString apiVersion() const;
    QString everestVersion() const;
    ChargerInfo chargerInfo() const;

    QList<EVSEInfo> evseInfos() const;

    // API calls
    EverestJsonRpcReply *evseGetInfo(int evseIndex);
    EverestJsonRpcReply *evseGetStatus(int evseIndex);
    EverestJsonRpcReply *evseGetHardwareCapabilities(int evseIndex);
    EverestJsonRpcReply *evseGetMeterData(int evseIndex);

    EverestJsonRpcReply *evseSetChargingAllowed(int evseIndex, bool allowed);
    EverestJsonRpcReply *evseSetACChargingCurrent(int evseIndex, double current);
    EverestJsonRpcReply *evseSetACChargingPhaseCount(int evseIndex, int phaseCount);

    // API parser methods

    // Enums
    static ResponseError parseResponseError(const QString &responseErrorString);
    static ConnectorType parseConnectorType(const QString &connectorTypeString);
    static ChargeProtocol parseChargeProtocol(const QString &chargeProtocolString);
    static EvseState parseEvseState(const QString &evseStateString);

    // Objects
    static EVSEInfo parseEvseInfo(const QVariantMap &evseInfoMap);
    static ConnectorInfo parseConnectorInfo(const QVariantMap &connectorInfoMap);
    static EVSEStatus parseEvseStatus(const QVariantMap &evseStatusMap);
    static ACChargeStatus parseACChargeStatus(const QVariantMap &acChargeStatusMap);
    static ACChargeParameters parseACChargeParameters(const QVariantMap &acChargeParametersMap);
    static HardwareCapabilities parseHardwareCapabilities(const QVariantMap &hardwareCapabilitiesMap);

public slots:
    void connectToServer(const QUrl &serverUrl);
    void disconnectFromServer();

signals:
    void connectionErrorOccurred();
    void availableChanged(bool available);

    // Notifications
    void evseStatusChanged(int evseIndex, const EverestJsonRpcClient::EVSEStatus &evseStatus);
    void hardwareCapabilitiesChanged(int evseIndex, const EverestJsonRpcClient::HardwareCapabilities &hardwareCapabilities);
    void meterDataChanged(int evseIndex, const EverestJsonRpcClient::HardwareCapabilities &hardwareCapabilities);
    // TODO void activeErrorsChanged();

private slots:
    void sendRequest(EverestJsonRpcReply *reply);
    void processDataPacket(const QByteArray &data);

private:
    bool m_available = false;
    int m_commandId = 0;

    EverestJsonRpcInterface *m_interface = nullptr;
    QHash<int, EverestJsonRpcReply *> m_replies;

    // Init infos
    QString m_apiVersion;
    QString m_everestVersion;
    ChargerInfo m_chargerInfo;
    bool m_authenticationRequired = false;
    QList<EVSEInfo> m_evseInfos;

    // API calls
    EverestJsonRpcReply *apiHello();
    EverestJsonRpcReply *chargePointGetEVSEInfos();

};

#endif // EVERESTJSONRPCCLIENT_H
