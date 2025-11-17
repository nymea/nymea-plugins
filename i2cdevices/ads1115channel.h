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

#ifndef ADS1115_H
#define ADS1115_H

#include <QThread>
#include <QMutex>

#include <hardware/i2c/i2cdevice.h>

class ADS1115Channel: public I2CDevice
{
    Q_OBJECT
public:

    enum Gain {
        Gain_6_144 = 0,
        Gain_4_096 = 1,
        Gain_2_048 = 2,
        Gain_1_024 = 3,
        Gain_0_512 = 4,
        Gain_0_256 = 5
    };

    explicit ADS1115Channel(const QString &portName, int address, int channel, Gain gain, QObject *parent = nullptr);

    QByteArray readData(int fd) override;

private:
    int m_channel = 0;
    Gain m_gain = Gain_4_096;

};

#endif // ADS1115_H
