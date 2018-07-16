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

#ifndef PIANOHAT_H
#define PIANOHAT_H

#include <QObject>
#include <QSoundEffect>

#include "touchsensor.h"
#include "plugin/device.h"

class PianoHat : public QObject
{
    Q_OBJECT
public:
    enum Key {
        // Sensor one 0x28
        KeyC            = 0,
        KeyCSharp       = 1,
        KeyD            = 2,
        KeyDSharp       = 3,
        KeyE            = 4,
        KeyF            = 5,
        KeyFSharp       = 6,
        KeyG            = 7,
        // Sensor two 0x2b
        KeyGSharp       = 8,
        KeyA            = 9,
        KeyASharp       = 10,
        KeyB            = 11,
        KeyHighC        = 12,
        KeyOctaveDown   = 13,
        KeyOctaveUp     = 14,
        KeyInstrument   = 15
    };
    Q_ENUM(Key)

    explicit PianoHat(QObject *parent = nullptr);

    void enable();
    void disable();

    void enableSounds(bool enable);

private:
    TouchSensor *m_sensorOne = nullptr;
    TouchSensor *m_sensorTwo = nullptr;

    bool m_soundsEnabled = true;

    QSoundEffect *m_soundKeyC = nullptr;
    QSoundEffect *m_soundKeyCSharp = nullptr;
    QSoundEffect *m_soundKeyD = nullptr;
    QSoundEffect *m_soundKeyDSharp = nullptr;
    QSoundEffect *m_soundKeyE = nullptr;
    QSoundEffect *m_soundKeyF = nullptr;
    QSoundEffect *m_soundKeyFSharp = nullptr;
    QSoundEffect *m_soundKeyG = nullptr;
    QSoundEffect *m_soundKeyGSharp = nullptr;
    QSoundEffect *m_soundKeyA = nullptr;
    QSoundEffect *m_soundKeyASharp = nullptr;
    QSoundEffect *m_soundKeyB = nullptr;
    QSoundEffect *m_soundKeyC2 = nullptr;

signals:
    void keyPressed(const Key &key, bool pressed);

private slots:
    void onSensorOneKeyPressedChanged(quint8 key, bool pressed);
    void onSensorTwoKeyPressedChanged(quint8 key, bool pressed);

};

#endif // PIANOHAT_H
