/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
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

#ifndef TOUCHSENSOR_H
#define TOUCHSENSOR_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>

#include "hardware/gpio.h"

// Registers of CAP1188 capacitive touch sensor (see doc folder for the datasheet)
#define CAP1188_MAIN 0x00
#define CAP1188_INPUTSTATUS 0x03
#define CAP1188_GENERAL_CONFIG 0x20
#define CAP1188_SENSITIVITY_CONFIG 0x1F
#define CAP1188_INPUT_ENABLE 0x21
#define CAP1188_SAMPLING_CONFIG 0x24
#define CAP1188_MULTITOUCH_CONFIG 0x2A
#define CAP1188_STANDBY_CONFIG 0x41
#define CAP1188_CONFIGURATION2  0x44
#define CAP1188_LED 0x72
#define CAP1188_LEDPOL 0x73
#define CAP1188_PROUCT_ID 0xFD
#define CAP1188_MANUFACTURER_ID 0xFE
#define CAP1188_REVISION 0xFF

class TouchSensor : public QThread
{
    Q_OBJECT
public:
    explicit TouchSensor(quint8 i2cRegister, int interruptGpio, int resetGpio, QObject *parent = nullptr);
    ~TouchSensor();

private:
    // Sensor wiring
    quint8 m_i2cRegister = 0x28;
    int m_interruptGpioPin = 4;
    int m_resetGpioPin = 17;

    Gpio *m_resetGpio = nullptr;
    quint8 m_currentInputStatus = 0;

    // I2C
    QString m_deviceFile = "/dev/i2c-1";
    int m_i2cDevice = -1;

    // Thread stuff
    QMutex m_stopMutex;
    bool m_stop = false;
    void run() override;

    // Setup methods
    void setupGpios();
    void initializeSensor();
    void cleanUp();

    // I2C methods
    quint8 readRegister(quint8 i2cRegister);
    void writeRegister(quint8 i2cRegister, quint8 value);

    // Sensor methods
    void clearInterrupt();
    void processInputData(quint8 data);

signals:
    void keyPressedChanged(int key, bool pressed);


public slots:
    void enableSensor();
    void disableSensor();

};

#endif // TOUCHSENSOR_H
