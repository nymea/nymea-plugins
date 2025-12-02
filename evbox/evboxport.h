// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
