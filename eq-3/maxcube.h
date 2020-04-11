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

#ifndef MAXCUBE_H
#define MAXCUBE_H

#include <QObject>
#include <QTcpSocket>
#include <QDateTime>
#include <QHostAddress>

#include "maxdevice.h"
#include "room.h"
#include "wallthermostat.h"
#include "radiatorthermostat.h"
#include "integrations/integrationplugin.h"

class MaxCube : public QTcpSocket
{
    Q_OBJECT
public:
    MaxCube(QObject *parent = 0, QString serialNumber = QString(), QHostAddress hostAdress = QHostAddress(), quint16 port = 0);

    enum WeekDay{
        Saturday = 0,
        Sunday = 1,
        Monday = 2,
        Tuesday = 3,
        Wednesday = 4,
        Thursday = 5,
        Friday = 6
    };

    // cube data access functions
    QString serialNumber() const;
    void setSerialNumber(const QString &serialNumber);

    QByteArray rfAddress() const;
    void setRfAddress(const QByteArray &rfAddress);

    int firmware() const;
    void setFirmware(const int &firmware);

    QHostAddress hostAddress() const;
    void setHostAddress(const QHostAddress &hostAddress);

    quint16 port() const;
    void setPort(const quint16 &port);

    QDateTime cubeDateTime() const;
    void setCubeDateTime(const QDateTime &cubeDateTime);

    bool portalEnabeld() const;

    QList<WallThermostat*> wallThermostatList();
    QList<RadiatorThermostat*> radiatorThermostatList();

    QList<Room*> roomList();

    void connectToCube();
    void disconnectFromCube();
    bool sendData(QByteArray data);

    bool isConnected();
    bool isInitialized();

public slots:
    void enablePairingMode();
    void disablePairingMode();
    void refresh();
    void customRequest(QByteArray data);

    // for actions
    int setDeviceSetpointTemp(QByteArray rfAddress, int roomId, double temperature);
    int setDeviceAutoMode(QByteArray rfAddress, int roomId);
    int setDeviceManuelMode(QByteArray rfAddress, int roomId);
    int setDeviceEcoMode(QByteArray rfAddress, int roomId);
    int displayCurrentTemperature(QByteArray rfAddress, int roomId, bool display);

signals:
    void cubeDataAvailable(const QByteArray &data);
    void cubeACK();
    void cubeConnectionStatusChanged(bool connected);

    // when things are parsed
    void cubeConfigReady();
    void wallThermostatFound();
    void radiatorThermostatFound();

    void wallThermostatDataUpdated();
    void radiatorThermostatDataUpdated();

    void commandActionFinished(bool succeeded, int commandId);

private slots:
    void connectionStateChanged(const QAbstractSocket::SocketState &socketState);
    void error(QAbstractSocket::SocketError error);
    void readData();
    void processCubeData(const QByteArray &data);

    void processCommandQueue();

private:
    // cube data
    QString m_serialNumber;
    QByteArray m_rfAddress;
    int m_firmware;
    QHostAddress m_hostAddress;
    quint16 m_port;
    QDateTime m_cubeDateTime;
    bool m_portalEnabeld;

    QList<Room*> m_roomList;
    QList<WallThermostat*> m_wallThermostatList;
    QList<RadiatorThermostat*> m_radiatorThermostatList;

    bool m_cubeInitialized;

    void decodeHelloMessage(QByteArray data);
    void decodeMetadataMessage(QByteArray data);
    void decodeConfigMessage(QByteArray data);
    void decodeDevicelistMessage(QByteArray data);
    void decodeCommandMessage(QByteArray data);
    void parseWeeklyProgram(QByteArray data);
    void decodeNewDeviceFoundMessage(QByteArray data);

    QDateTime calculateDateTime(QByteArray dateRaw, QByteArray timeRaw);
    QString deviceTypeString(int deviceType);
    QString weekDayString(int weekDay);

    QByteArray fillBin(QByteArray data, int dataLength);
    QList<QByteArray> splitMessage(QByteArray data);
    int deviceTypeFromRFAddress(QByteArray rfAddress);

    struct Command {
        qint16 commandId;
        QByteArray data;
    };

    Command m_pendingCommand;

    quint8 generateCommandId();
    QList<Command> m_commandQueue;

};

#endif // MAXCUBE_H
