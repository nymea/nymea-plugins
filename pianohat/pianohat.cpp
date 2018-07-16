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

#include <QAudioDeviceInfo>

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

    foreach (const QAudioDeviceInfo &audioInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        qCDebug(dcPianohat()) << audioInfo.deviceName();
    }

    // Create sound effects
    m_soundKeyC = new QSoundEffect(this);
    m_soundKeyC->setVolume(1);
    m_soundKeyC->setSource(QUrl::fromLocalFile(":/c1.wav"));

    m_soundKeyCSharp = new QSoundEffect(this);
    m_soundKeyCSharp->setVolume(1);
    m_soundKeyCSharp->setSource(QUrl::fromLocalFile(":/c1s.wav"));

    m_soundKeyD = new QSoundEffect(this);
    m_soundKeyD->setVolume(1);
    m_soundKeyD->setSource(QUrl::fromLocalFile(":/d1.wav"));

    m_soundKeyDSharp = new QSoundEffect(this);
    m_soundKeyDSharp->setVolume(1);
    m_soundKeyDSharp->setSource(QUrl::fromLocalFile(":/d1s.wav"));

    m_soundKeyE = new QSoundEffect(this);
    m_soundKeyE->setVolume(1);
    m_soundKeyE->setSource(QUrl::fromLocalFile(":/e1.wav"));

    m_soundKeyF = new QSoundEffect(this);
    m_soundKeyF->setVolume(1);
    m_soundKeyF->setSource(QUrl::fromLocalFile(":/f1.wav"));

    m_soundKeyFSharp = new QSoundEffect(this);
    m_soundKeyFSharp->setVolume(1);
    m_soundKeyFSharp->setSource(QUrl::fromLocalFile(":/f1s.wav"));

    m_soundKeyG = new QSoundEffect(this);
    m_soundKeyG->setVolume(1);
    m_soundKeyG->setSource(QUrl::fromLocalFile(":/g1.wav"));

    m_soundKeyGSharp = new QSoundEffect(this);
    m_soundKeyGSharp->setVolume(1);
    m_soundKeyGSharp->setSource(QUrl::fromLocalFile(":/g1s.wav"));

    m_soundKeyA = new QSoundEffect(this);
    m_soundKeyA->setVolume(1);
    m_soundKeyA->setSource(QUrl::fromLocalFile(":/a1.wav"));

    m_soundKeyASharp = new QSoundEffect(this);
    m_soundKeyASharp->setVolume(1);
    m_soundKeyASharp->setSource(QUrl::fromLocalFile(":/a1s.wav"));

    m_soundKeyB = new QSoundEffect(this);
    m_soundKeyB->setVolume(1);
    m_soundKeyB->setSource(QUrl::fromLocalFile(":/b1.wav"));

    m_soundKeyC2 = new QSoundEffect(this);
    m_soundKeyC2->setVolume(1);
    m_soundKeyC2->setSource(QUrl::fromLocalFile(":/c2.wav"));
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

void PianoHat::enableSounds(bool enable)
{
    m_soundsEnabled = enable;
}

void PianoHat::onSensorOneKeyPressedChanged(quint8 key, bool pressed)
{
    // Map the bit number to the actual pianohat key
    Key pianoHatKey = static_cast<Key>(key);
    emit keyPressed(pianoHatKey, pressed);

    if (!m_soundsEnabled || !pressed)
        return;

    switch (key) {
    case KeyC:
        m_soundKeyC->play();
        break;
    case KeyCSharp:
        m_soundKeyCSharp->play();
        break;
    case KeyD:
        m_soundKeyD->play();
        break;
    case KeyDSharp:
        m_soundKeyDSharp->play();
        break;
    case KeyE:
        m_soundKeyE->play();
        break;
    case KeyF:
        m_soundKeyF->play();
        break;
    case KeyFSharp:
        m_soundKeyFSharp->play();
        break;
    case KeyG:
        m_soundKeyG->play();
        break;
    default:
        break;
    }
}

void PianoHat::onSensorTwoKeyPressedChanged(quint8 key, bool pressed)
{
    // Map the bit number to the actual pianohat key (bit + 8)
    Key pianoHatKey = static_cast<Key>(key + 8);
    emit keyPressed(pianoHatKey, pressed);

    if (!m_soundsEnabled || !pressed)
        return;

    switch (pianoHatKey) {
    case KeyGSharp:
        m_soundKeyGSharp->play();
        break;
    case KeyA:
        m_soundKeyA->play();
        break;
    case KeyASharp:
        m_soundKeyASharp->play();
        break;
    case KeyB:
        m_soundKeyB->play();
        break;
    case KeyHighC:
        m_soundKeyC2->play();
        break;
    default:
        break;
    }
}
