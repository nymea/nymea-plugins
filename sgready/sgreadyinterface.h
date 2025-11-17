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

#ifndef SGREADYINTERFACE_H
#define SGREADYINTERFACE_H

#include <QObject>
#include <gpio.h>

class SgReadyInterface : public QObject
{
    Q_OBJECT
public:
    enum SgReadyMode {
        SgReadyModeOff,
        SgReadyModeLow,
        SgReadyModeStandard,
        SgReadyModeHigh
    };
    Q_ENUM(SgReadyMode)

    explicit SgReadyInterface(int gpioNumber1, int gpioNumber2, QObject *parent = nullptr);

    SgReadyMode sgReadyMode() const;
    bool setSgReadyMode(SgReadyMode sgReadyMode);

    bool setup(bool gpio1Enabled, bool gpio2Enabled);
    bool isValid() const;

    Gpio *gpio1() const;
    Gpio *gpio2() const;

signals:
    void sgReadyModeChanged(SgReadyMode sgReadyMode);

private:
    SgReadyMode m_sgReadyMode = SgReadyModeStandard;
    int m_gpioNumber1 = -1;
    int m_gpioNumber2 = -1;

    Gpio *m_gpio1 = nullptr;
    Gpio *m_gpio2 = nullptr;

    Gpio *setupGpio(int gpioNumber, bool initialValue);
};

#endif // SGREADYINTERFACE_H
