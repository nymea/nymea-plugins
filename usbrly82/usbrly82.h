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

#ifndef USBRLY82_H
#define USBRLY82_H

#include <QHash>
#include <QTimer>
#include <QQueue>
#include <QObject>
#include <QSerialPort>

class UsbRly82Reply : public QObject
{
    Q_OBJECT

    friend class UsbRly82;

public:
    enum Error {
        ErrorNoError,
        ErrorTimeout
    };
    Q_ENUM(Error)

    Error error() const;
    QByteArray requestData() const;
    QByteArray responseData() const;

signals:
    void finished();

private:
    explicit UsbRly82Reply(QObject *parent = nullptr);

    Error m_error = ErrorNoError;
    QTimer m_timer;
    bool m_expectsResponse = true;

    QByteArray m_requestData;
    QByteArray m_responseData;
};

class UsbRly82 : public QObject
{
    Q_OBJECT
public:
    explicit UsbRly82(QObject *parent = nullptr);

    bool available() const;
    QString serialNumber() const;
    QString softwareVersion() const;

    bool powerRelay1() const;
    UsbRly82Reply *setRelay1Power(bool power);

    bool powerRelay2() const;
    UsbRly82Reply *setRelay2Power(bool power);

    uint analogRefreshRate() const;
    void setAnalogRefreshRate(uint analogRefreshRate);

    quint8 digitalInputs() const;

    bool connectRelay(const QString &serialPort);
    void disconnectRelay();

    UsbRly82Reply *getSerialNumber();
    UsbRly82Reply *getSoftwareVersion();
    UsbRly82Reply *getRelayStates();

    UsbRly82Reply *getDigitalInputs();
    UsbRly82Reply *getAdcValues();
    UsbRly82Reply *getAdcReference();

    static bool checkBit(quint8 byte, uint bitNumber);

signals:
    void availableChanged(bool available);

    void powerRelay1Changed(bool powerRelay1);
    void powerRelay2Changed(bool powerRelay2);

    void digitalInputsChanged();

private:
    QTimer m_digitalRefreshTimer;
    QTimer m_analogRefreshTimer;
    QSerialPort *m_serialPort = nullptr;

    bool m_available = false;

    QString m_serialNumber;
    QString m_softwareVersion;

    uint m_analogRefreshRate = 1000;

    bool m_powerRelay1 = false;
    bool m_powerRelay2 = false;

    UsbRly82Reply *m_currentReply = nullptr;
    QQueue<UsbRly82Reply *> m_replyQueue;

    UsbRly82Reply *m_updateDigitalInputsReply = nullptr;
    UsbRly82Reply *m_updateAnalogInputsReply = nullptr;

    UsbRly82Reply *createReply(const QByteArray &requestData, bool expectsResponse = true);
    void sendNextRequest();

    quint8 m_digitalInputs = 0x00;
    QHash<int, quint16> m_analogValues;

private slots:
    void onReadyRead();
    void onError(QSerialPort::SerialPortError error);

    void updateDigitalInputs();
    void updateAnalogInputs();
};

Q_DECLARE_METATYPE(QSerialPort::SerialPortError)

#endif // USBRLY82_H
