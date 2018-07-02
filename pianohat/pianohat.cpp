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

#include "pianohat.h"
#include "extern-plugininfo.h"

PianoHat::PianoHat(QObject *parent) :
    QObject(parent)
{
    qCDebug(dcPianohat()) << "Create touch sensor from main trhead" << QThread::currentThreadId();

    /* Note: the pianohat has 2 CAP1188 chips on following Addresses
     * https://pinout.xyz/pinout/piano_hat#
     * - Register 0x28
     *      - Alert GPIO: 4
     *      - Reset GPIO: 17
     * - Register 0x2b
     *      - Alert GPIO: 27
     *      - Reset GPIO: 22
     */

    m_sensorOne = new TouchSensor(0x28, 4, 17, this);
    connect(m_sensorOne, &TouchSensor::keyPressedChanged, this, &PianoHat::onSensorOneKeyPressedChanged);

    m_sensorTwo = new TouchSensor(0x2b, 27, 22, this);
    connect(m_sensorTwo, &TouchSensor::keyPressedChanged, this, &PianoHat::onSensorTwoKeyPressedChanged);
}

void PianoHat::enable()
{
    // Enable the sensors
    m_sensorOne->enableSensor();
    m_sensorTwo->enableSensor();
}

void PianoHat::disable()
{
    // Enable the sensors
    m_sensorOne->disableSensor();
    m_sensorTwo->disableSensor();
}

void PianoHat::onSensorOneKeyPressedChanged(quint8 key, bool pressed)
{
    // Map the bit number to the actual pianohat key
    Key pianoHatKey = static_cast<Key>(key);
    emit keyPressed(pianoHatKey, pressed);
}

void PianoHat::onSensorTwoKeyPressedChanged(quint8 key, bool pressed)
{
    // Map the bit number to the actual pianohat key (bit + 8)
    Key pianoHatKey = static_cast<Key>(key + 8);
    emit keyPressed(pianoHatKey, pressed);
}
