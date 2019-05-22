/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Simon Stürz <simon.stuerz@nymea.io>                 *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef HONEYWELLSCANNER_H
#define HONEYWELLSCANNER_H

#include <QTimer>
#include <QObject>
#include <QSerialPort>

class HoneywellScanner : public QObject
{
    Q_OBJECT


public:
    enum Mode {
        ModeManual,
        ModePresentation
    };
    Q_ENUM(Mode)

    explicit HoneywellScanner(const QString &serialPortName, qint32 baudrate, QObject *parent = nullptr);

    bool connected() const;

private:
    QTimer *m_reconnectTimer = nullptr;
    QSerialPort *m_serialPort = nullptr;

    QString m_serialPortName;
    qint32 m_baudrate = 115200;

    bool m_connected = false;
    QByteArray m_asyncCommand;

    void init();
    void setConnected(bool connected);
    void sendData(const QByteArray &data);
    QByteArray buildCommand(const QString &command);


signals:
    void connectedChanged(bool connected);
    void codeScanned(const QString &code);

private slots:
    void onReconnectTimeout();
    void onReadyRead();
    void onError(const QSerialPort::SerialPortError &error);

public slots:
    void enable();
    void disable();

    // Actions
    bool reset();
    bool configureDefaults();
    bool configurePowerUpBeep(bool enabled);
    bool configureLedPower(bool power);
    bool configureTrigger(bool enabled);
    bool configureMode(Mode mode);


};

#endif // HONEYWELLSCANNER_H
