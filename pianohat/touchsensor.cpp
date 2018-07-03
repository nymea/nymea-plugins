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

#include "touchsensor.h"
#include "extern-plugininfo.h"

#include <stdlib.h>
#include <unistd.h>
extern "C" {
#include "linux/i2c-dev.h"
}
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>


TouchSensor::TouchSensor(quint8 i2cRegister, int interruptGpio, int resetGpio, QObject *parent) :
    QThread(parent),
    m_i2cRegister(i2cRegister),
    m_interruptGpioPin(interruptGpio),
    m_resetGpioPin(resetGpio)
{

}

TouchSensor::~TouchSensor()
{
    disableSensor();
    wait();
}

void TouchSensor::setupGpios()
{
    qCDebug(dcPianohat()) << "Setup GPIOs for sensor" << QString().number(m_i2cRegister, 16).prepend("0x");

    // Init reset gpio
    m_resetGpio = new Gpio(m_resetGpioPin);
    if (!m_resetGpio->exportGpio()) {
        qCWarning(dcPianohat()) << "Could not export reset GPIO" << m_resetGpioPin;
    }

    msleep(500);

    if (!m_resetGpio->setDirection(Gpio::DirectionOutput)) {
        qCWarning(dcPianohat()) << "Could not set direction of reset GPIO" << m_resetGpioPin;
    }

    m_resetGpio->moveToThread(this);
}

void TouchSensor::initializeSensor()
{
    qCDebug(dcPianohat()) << "Start initializing sensor" << QString().number(m_i2cRegister, 16).prepend("0x");

    // Reset the controller
    qCDebug(dcPianohat()) << "Reset sensor" << QString().number(m_i2cRegister, 16).prepend("0x");
    m_resetGpio->setValue(Gpio::ValueLow);
    msleep(100);
    m_resetGpio->setValue(Gpio::ValueHigh);
    msleep(100);
    m_resetGpio->setValue(Gpio::ValueLow);
    msleep(100);

    // Configure sensor
    qCDebug(dcPianohat()) << "Setup sensor" << QString().number(m_i2cRegister, 16).prepend("0x");

    m_i2cDevice = open(m_deviceFile.toLocal8Bit(), O_RDWR);
    if(m_i2cDevice < 0){
        qCWarning(dcPianohat()) << "Could not open I2C device" << m_deviceFile;
        return;
    }

    int ret = ioctl(m_i2cDevice, I2C_SLAVE, m_i2cRegister);
    if (ret < 0) {
        qCWarning(dcPianohat()) << "Failed to aquire slave mode access to I2C device";
        return;
    }

    qCDebug(dcPianohat()) << "--> Product ID:" << QString().number(readRegister(CAP1188_PROUCT_ID), 16).prepend("0x");
    qCDebug(dcPianohat()) << "--> Manufacturer ID:" << QString().number(readRegister(CAP1188_MANUFACTURER_ID), 16).prepend("0x");
    qCDebug(dcPianohat()) << "--> Revision:" << QString().number(readRegister(CAP1188_REVISION), 16).prepend("0x");

    // Enable all inputs
    writeRegister(CAP1188_INPUT_ENABLE, 0xFF);

    // Enable interrupts
    writeRegister(0x27, 0xFF);

    // Disable repeat
    writeRegister(0x28, 0x00);

    // Enable multitouch
    quint8 multitouchConfig = readRegister(CAP1188_MULTITOUCH_CONFIG);
    writeRegister(CAP1188_MULTITOUCH_CONFIG, multitouchConfig & ~0x80);

    // Sampling config
    // Note: configurations from: https://github.com/pimoroni/cap1xxx/blob/master/library/cap1xxx.py#L305
    writeRegister(CAP1188_SAMPLING_CONFIG, 0b00001000);  // 1sample per measure, 1.28ms time, 35ms cycle

    // Sensitivity
    writeRegister(CAP1188_SENSITIVITY_CONFIG, 0b01100000);  // 2x sensitivity

    // General config
    writeRegister(CAP1188_GENERAL_CONFIG, 0b00111000);

    // Config 2
    writeRegister(CAP1188_CONFIGURATION2, 0b01100000);

    // Enable led for all keys
    writeRegister(CAP1188_LED, 0xFF);

    // Speed up
    writeRegister(CAP1188_STANDBY_CONFIG, 0x30);
}

void TouchSensor::cleanUp()
{
    // Clean up
    if (m_resetGpio) {
        delete m_resetGpio;
        m_resetGpio = nullptr;
    }

    if (m_i2cDevice > 0) {
        close(m_i2cDevice);
        m_i2cDevice = -1;
    }
}

void TouchSensor::run()
{
    setupGpios();
    initializeSensor();

    qCDebug(dcPianohat()) << "Initialized sensor" << QString().number(m_i2cRegister, 16).prepend("0x");

    // Poll GPIO for interrupts
    while (1) {

        quint8 inputStatus = readRegister(CAP1188_INPUTSTATUS);
        if (inputStatus) {
            processInputData(inputStatus);
            clearInterrupt();
        } else {
            processInputData(0);
        }

        QMutexLocker locker(&m_stopMutex);
        if (m_stop) break;
        msleep(50);
    }

    cleanUp();
    qCDebug(dcPianohat()) << "Sensor thread finished.";
}

quint8 TouchSensor::readRegister(quint8 i2cRegister)
{
    i2c_smbus_write_byte(m_i2cDevice, i2cRegister);
    return i2c_smbus_read_byte(m_i2cDevice);
}

void TouchSensor::writeRegister(quint8 i2cRegister, quint8 value)
{
    i2c_smbus_write_byte_data(m_i2cDevice, i2cRegister, value);
}

void TouchSensor::clearInterrupt()
{
    quint8 mainConfiguration = readRegister(CAP1188_MAIN);
    mainConfiguration &= ~0b00000001;
    writeRegister(CAP1188_MAIN, mainConfiguration);
}

void TouchSensor::processInputData(quint8 data)
{
    if (m_currentInputStatus != data) {
        //qCDebug(dcPianohat()) <<  "-->" << data;

        for (int i = 0; i < 8; i++) {
            bool keyPressed = data & (1 << i);
            bool previousStatus = m_currentInputStatus & (1 << i);
            if (previousStatus != keyPressed) {
                //qCDebug(dcPianohat()) << "Touched changed" << QString().number(m_i2cRegister, 16).prepend("0x") << " --> Key" << i << " = " << (keyPressed ? "pressed" : "released");
                emit keyPressedChanged(i, keyPressed);
            }
        }

        m_currentInputStatus = data;
    }
}

void TouchSensor::enableSensor()
{
    start();
}

void TouchSensor::disableSensor()
{
    qCDebug(dcPianohat()) << "Disable sensor";
    QMutexLocker locker(&m_stopMutex);
    m_stop = true;
}
