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

#ifndef KECONTACT_H
#define KECONTACT_H

#include <QTimer>
#include <QObject>
#include <QHostAddress>
#include <QByteArray>
#include <QUdpSocket>
#include <QUuid>
#include <QQueue>

#include "kecontactdatalayer.h"

class KeContactRequest
{
public:
    KeContactRequest() = default;
    KeContactRequest(const QUuid &requestId, const QByteArray &command) : m_requestId(requestId), m_command(command) { }

    QUuid requestId() const { return m_requestId; }
    QByteArray command() const { return m_command; }

    uint delayUntilNextCommand() const { return m_delayUntilNextCommand; }
    void setDelayUntilNextCommand(uint delayUntilNextCommand) { m_delayUntilNextCommand = delayUntilNextCommand; }

    bool isValid() { return !m_requestId.isNull() && !m_command.isEmpty(); }

private:
    QUuid m_requestId;
    QByteArray m_command;
    uint m_delayUntilNextCommand = 200;
};



class KeContact : public QObject
{
    Q_OBJECT
public:
    enum DipSwitchOne {
        // Power settings
        // DIP 6 7 8 (Bit 2, 1, 0)
        //     0 0 0 : 10A
        //     1 0 0 : 13A
        //     0 1 0 : 16A
        //     1 1 0 : 20A
        //     0 0 1 : 25A
        //     1 0 1 : 32A
        DipSwitchOnePin8 = 0x01,
        DipSwitchOnePin7 = 0x02,
        DipSwitchOnePin6 = 0x04,
        DipSwitchOnePin5 = 0x08,
        DipSwitchOnePin4 = 0x10,
        DipSwitchOneSmartHomeInterface = 0x20, // 3
        DipSwitchOneExternalInputX2 = 0x40, // 2
        DipSwitchOneExternalInputX1 = 0x80 // 1
    };
    Q_ENUM(DipSwitchOne)
    Q_DECLARE_FLAGS(DipSwitchOneFlag, DipSwitchOne)

    enum State {
        StateStarting = 0,
        StateNotReady,
        StateReady,
        StateCharging,
        StateError,
        StateAuthorizationRejected
    };
    Q_ENUM(State)

    enum PlugState {
        PlugStateUnplugged                                           = 0,
        PlugStatePluggedOnChargingStation                            = 1,
        PlugStatePluggedOnChargingStationAndPlugLocked               = 3,
        PlugStatePluggedOnChargingStationAndPluggedOnEV              = 5,
        PlugStatePluggedOnChargingStationAndPlugLockedAndPluggedOnEV = 7
    };
    Q_ENUM(PlugState)

    enum BroadcastType {
        BroadcastTypeState = 0,
        BroadcastTypePlug,
        BroadcastTypeInput,
        BroadcastTypeEnableSys,
        BroadcastTypeMaxCurr,
        BroadcastTypeEPres
    };
    Q_ENUM(BroadcastType)

    struct ReportOne {
        QString product;        // Model  name  (variant
        QString serialNumber;   // Serial number
        QString firmware;       // Firmware version
        bool comModule;         // Communication module is installed (only P30)
        int seconds;            // Current system clock since restart of the charging station.(only P30)
        quint8 dipSw1;            // Dip Switch 1 flag
        quint8 dipSw2;            // Dip Switch 2 flag
    };

    struct ReportTwo {
        State state;                    //Current state of the charging station
        int error1;                     //Detail code for state 4; exceptions see FAQ on www.kecontact.com
        int error2;                     //Detail code for state 4 exception #6 see FAQ on www.kecontact.com
        PlugState plugState;            //Current condition of the loading connection
        bool enableSys;                 //Enable state for charging (contains Enable input, RFID, UDP,..).
        bool enableUser;                //Enable condition via UDP.
        double maxCurrent;              //Current preset value via Control pilot in ampere.
        double maxCurrentPercentage;    //Current preset value via Control pilot in 0,1% of the PWM value
        double currentHardwareLimitation;  //Highest possible charging current of the charging connection. Contains device maximum, DIP-switch setting, cable coding and temperature reduction.
        double currentUser;                //Current preset value of the user via UDP; Default = 63000mA.
        double currentFailsafe;            //Current preset value for the Failsafe function.
        int timeoutFailsafe;            //Communication timeout before triggering the Failsafe function.
        int currTimer;                  //Shows  the  current  preset  value  of  currtime.
        int timeoutCt;                  //Shows the remaining time until the current value is accepted.
        double setEnergy;                  //Shows  the  set  energy  limit
        bool output;                    //State of the output X2.
        bool input;                     //State of the potential free Enable input X1. When using the input, please pay attention to the information in the installation manual.
        QString serialNumber;           //Serial number
        int seconds;                    //Current system clock since restart of the charging station.
    };

    struct ReportThree {
        int voltagePhase1;  //voltage in V
        int voltagePhase2;  //voltage in V
        int voltagePhase3;  //voltage in V
        double currentPhase1;  //current in A
        double currentPhase2;  //current in A
        double currentPhase3;  //current in A
        double power;          //Current power in W (Real Power).
        double powerFactor;    //Power factor (cosphi)
        double energySession;  //Power consumption of the current loading session in kWh; Reset with new loading session (state = 2).
        double energyTotal;    //Total power consumption (persistent) without current loading session kWh; Is summed up after each completed charging session (state = 0).
        QString serialNumber;
        int seconds;        //Current system clock since restart of the charging station.
    };

    struct Report1XX {
        int sessionId;          // running session counter; not resettable"
        double currHW;          // maximum charging current of the cable and the charging station setting
        double startEnergy;     // total energy value at the beginning of the session"
        double presentEnergy;   // delivered energy until now (equal to E pres in report 3)"
        int startTime;          // system time when the session was started (seconds from reboot;
        int endTime;            // system time when the session has ended"
        int stopReason;         // reason for stopping the session (1 = vehicle unplug; 10 = Rfid token)"
        QByteArray rfidTag;     // RFID Token ID if session started with rfid
        QByteArray rfidClass;   // RFID classifier shows the defined color code
        QString serialNumber;   // serial number of the charging station"
        int seconds;            // current time when the report was generated
    };

    explicit KeContact(const QHostAddress &address, KeContactDataLayer *dataLayer, QObject *parent = nullptr);
    ~KeContact();

    QHostAddress address() const;
    void setAddress(const QHostAddress &address);

    bool reachable() const;

    QUuid start(const QByteArray &rfidToken, const QByteArray &rfidClassifier);     // Command “start”
    QUuid stop(const QByteArray &rfidToken);                // Command “stop”

    QUuid enableOutput(bool state);                         // Command “ena”
    QUuid setMaxAmpere(int milliAmpere);                    // Command "currtime"
    QUuid setMaxAmpereGeneral(int milliAmpere);             // Command "curr"
    QUuid unlockCharger();                                  // Command “unlock"
    QUuid displayMessage(const QByteArray &message);        // Command “display”
    QUuid chargeWithEnergyLimit(double energy);             // Command “setenergy”
    QUuid setFailsafe(int timeout, int current, bool save); // Command “failsafe”

    void getDeviceInformation();                        // Command “i”
    void getReport1();                                  // Command “report”
    void getReport2();
    void getReport3();
    void getReport1XX(int reportNumber = 100);          // Command “report 1xx”

    // Command “currtime”
    QUuid setOutputX2(bool state);                       // Command “output”

private:
    KeContactDataLayer *m_dataLayer = nullptr;
    bool m_reachable = false;

    QHostAddress m_address;

    QTimer *m_requestTimeoutTimer = nullptr;
    QTimer *m_pauseTimer = nullptr;
    int m_serialNumber = 0;

    KeContactRequest m_currentRequest;
    QQueue<KeContactRequest> m_requestQueue;

    void getReport(int reportNumber);

    void sendCommand(const QByteArray &command);
    void sendNextCommand();
    void setReachable(bool reachable);

signals:
    void reachableChanged(bool status);
    void commandExecuted(QUuid requestId, bool success);
    void deviceInformationReceived(const QString &firmware);
    void reportOneReceived(const ReportOne &reportOne);
    void reportTwoReceived(const ReportTwo &reportTwo);
    void reportThreeReceived(const ReportThree &reportThree);
    void report1XXReceived(int reportNumber, const Report1XX &report);
    void broadcastReceived(BroadcastType type, const QVariant &content);

private slots:
    void onReceivedDatagram(const QHostAddress &address, const QByteArray &datagram);

};

Q_DECLARE_OPERATORS_FOR_FLAGS(KeContact::DipSwitchOneFlag);

#endif // KECONTACT_H

