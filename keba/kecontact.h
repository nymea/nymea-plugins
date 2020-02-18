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

#ifndef KECONTACT_H
#define KECONTACT_H

#include <QTimer>
#include <QObject>
#include <QHostAddress>
#include <QByteArray>
#include <QUdpSocket>
#include <QUuid>

class KeContact : public QObject
{
    Q_OBJECT
public:
    explicit KeContact(QHostAddress address, QObject *parent = nullptr);
    ~KeContact();
    bool init();

    enum State {
        Starting = 0,
        NotReady,
        Ready,
        Charging,
        Error,
        AuthorizationRejected
    };

    enum PlugState {
        Unplugged                                           = 0,
        PluggedOnChargingStation                            = 1,
        PluggedOnChargingStationAndPlugLocked               = 3,
        PluggedOnChargingStationAndPluggedOnEV              = 5,
        PluggedOnChargingStationAndPlugLockedAndPluggedOnEV = 7
    };

    struct ReportOne {
        QString product;
        QString serialNumber;
        QString firmware;
    };

    struct ReportTwo {
        State state;                    //Current state of the charging station
        int error1;                     //Detail code for state 4; exceptions see FAQ on www.kecontact.com
        int error2;                     //Detail code for state 4 exception #6 see FAQ on www.kecontact.com
        PlugState plugState;            //Current condition of the loading connection
        bool enableSys;                 //Enable state for charging (contains Enable input, RFID, UDP,..).
        bool enableUser;                //Enable condition via UDP.
        int MaxCurrent;                 //Current preset value via Control pilot in milliampere.
        int MaxCurrentPercentage;       //Current preset value via Control pilot in 0,1% of the PWM value
        int CurrentHardwareLimitation;  //Highest possible charging current of the charging connection. Contains device maximum, DIP-switch setting, cable coding and temperature reduction.
        int CurrentUser;                //Current preset value of the user via UDP; Default = 63000mA.
        int CurrFS;                     //Current preset value for the Failsafe function.
        int TmoFS;                      //Communication timeout before triggering the Failsafe function.
        bool output;                    //State of the output X2.
        bool input;                     //State of the potential free Enable input X1. When using the input, please pay attention to the information in the installation manual.
        QString serialNumber;           //
        int seconds;                    //Current system clock since restart of the charging station.
    };

    struct ReportThree {
        int VoltagePhase1; //voltage in V
        int VoltagePhase2; //voltage in V
        int VoltagePhase3; //voltage in V
        int CurrentPhase1; //current in mA
        int CurrentPhase2; //current in mA
        int CurrentPhase3;  //current in mA
        int Power;          //Current power in mW (Real Power).
        int PowerFactor;    //Power factor in 0,1% (cosphi)
        int EnergySession;  //Power consumption of the current loading session in 0,1Wh; Reset with new loading session (state = 2).
        int EnergyTotal;    //Total power consumption (persistent) without current loading session 0,1Wh; Is summed up after each completed charging session (state = 0).
        QString SerialNumber;
    };

    QHostAddress address();
    int serialNumber();

    void setAddress(QHostAddress address);
    bool getDeviceConnectedStatus();
    bool getDeviceBlockedStatus();

    QUuid enableOutput(bool state);
    QUuid setMaxAmpere(int milliAmpere);
    void getDeviceInformation();
    void getReport1();
    void getReport2();
    void getReport3();
    QUuid unlockCharger();
    QUuid displayMessage(const QByteArray &message);


private:
    QUdpSocket *m_udpSocket = nullptr;
    QHostAddress m_address;
    QByteArrayList m_commandList;
    bool m_deviceBlocked = false;
    bool m_connected = false;
    QTimer *m_requestTimeoutTimer = nullptr;
    int m_serialNumber;
    QList<QUuid> m_pendingRequests;

    void sendCommand(const QByteArray &data);
    void handleNextCommandInQueue();

signals:
    void connectionChanged(bool status);
    void commandExecuted(QUuid requestId, bool success);
    void deviceInformationReceived(const QString &firmware);
    void reportOneReceived(const ReportOne &reportOne);
    void reportTwoReceived(const ReportTwo &reportTwo);
    void reportThreeReceived(const ReportThree &reportThree);

private slots:
    void readPendingDatagrams();
};


#endif // KECONTACT_H

