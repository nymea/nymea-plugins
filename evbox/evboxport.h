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

#ifndef EVBOXPORT_H
#define EVBOXPORT_H

#include <QObject>
#include <QSerialPort>
#include <QQueue>
#include <QTimer>

class EVBoxPort : public QObject
{
    Q_OBJECT
public:
    enum Command {
        Command68 = 68,
        Command69 = 69
    };
    Q_ENUM(Command)

    explicit EVBoxPort(const QString &portName, QObject *parent = nullptr);

    bool open();
    bool isOpen();
    void close();

    void sendCommand(Command command, quint16 timeout, quint16 maxChargingCurrent, const QString &serial = "00000000");

signals:
    void opened();
    void closed();
    void shortResponseReceived(EVBoxPort::Command command, const QString &serial);
    void responseReceived(EVBoxPort::Command command, const QString &serial, quint16 minChargingCurrent, quint16 maxChargingCurrent, quint16 chargingCurrentL1, quint16 chargingCurrentL2, quint16 chargingCurrentL3, quint32 totalEnergyConsumed);

private slots:
    void processQueue();
    void onReadyRead();
    void processDataPacket(const QByteArray &packet);

private:
    QByteArray createChecksum(const QByteArray &data) const;

private:
    QSerialPort *m_serialPort = nullptr;

    QByteArray m_inputBuffer;

    struct CommandWrapper {
        Command command = Command68;
        QString serial;
        quint16 timeout = 0;
        quint16 maxChargingCurrent = 0;
    };
    QQueue<CommandWrapper> m_commandQueue;
    QTimer m_waitTimer;
};

#endif // EVBOXPORT_H
