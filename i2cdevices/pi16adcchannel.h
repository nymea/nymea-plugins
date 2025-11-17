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

#ifndef PI16ADC_H
#define PI16ADC_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QHash>

#include <hardware/i2c/i2cdevice.h>

class Pi16ADCChannel : public I2CDevice
{
    Q_OBJECT
public:
    explicit Pi16ADCChannel(const QString &portName, int address, int channel, QObject *parent = nullptr);

    QByteArray readData(int fileDescriptor) override;

private:
    int m_channel = 0;
};

#endif // PI16ADC_H
