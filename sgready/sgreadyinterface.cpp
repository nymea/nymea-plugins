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

#include "sgreadyinterface.h"
#include "extern-plugininfo.h"

SgReadyInterface::SgReadyInterface(int gpioNumber1, int gpioNumber2, QObject *parent) :
    QObject(parent),
    m_gpioNumber1(gpioNumber1),
    m_gpioNumber2(gpioNumber2)
{

}

SgReadyInterface::SgReadyMode SgReadyInterface::sgReadyMode() const
{
    return m_sgReadyMode;
}

bool SgReadyInterface::setSgReadyMode(SgReadyMode sgReadyMode)
{
    if (!isValid())
        return false;

    /* https://www.waermepumpe.de/normen-technik/sg-ready/
     *
     * Off: 1,0
     * Low: 0,0
     * Standard: 0,1
     * High: 1,1
     */

    QPair<bool, bool> gpioSettings;
    switch (sgReadyMode) {
    case SgReadyModeOff:
        gpioSettings.first = true;
        gpioSettings.second = false;
        break;
    case SgReadyModeLow:
        gpioSettings.first = false;
        gpioSettings.second = false;
        break;
    case SgReadyModeStandard:
        gpioSettings.first = false;
        gpioSettings.second = true;
        break;
    case SgReadyModeHigh:
        gpioSettings.first = true;
        gpioSettings.second = true;
        break;
    }

    if (!m_gpio1->setValue(gpioSettings.first ? Gpio::ValueHigh : Gpio::ValueLow)) {
        qCWarning(dcSgReady()) << "Could not switch GPIO 1 for setting" << sgReadyMode;
        return false;
    }

    if (!m_gpio2->setValue(gpioSettings.second ? Gpio::ValueHigh : Gpio::ValueLow)) {
        qCWarning(dcSgReady()) << "Could not switch GPIO 2 for setting" << sgReadyMode;
        return false;
    }

    if (m_sgReadyMode != sgReadyMode) {
        m_sgReadyMode = sgReadyMode;
        emit sgReadyModeChanged(m_sgReadyMode);
    }

    return true;
}

bool SgReadyInterface::setup(bool gpio1Enabled, bool gpio2Enabled)
{
    m_gpio1 = setupGpio(m_gpioNumber1, gpio1Enabled);
    if (!m_gpio1) {
        qCWarning(dcSgReady()) << "Failed to set up SG ready interface gpio 1" << m_gpioNumber1;
        return false;
    }

    m_gpio2 = setupGpio(m_gpioNumber2, gpio2Enabled);
    if (!m_gpio2) {
        delete m_gpio1;
        m_gpio1 = nullptr;
        qCWarning(dcSgReady()) << "Failed to set up SG ready interface gpio 2" << m_gpioNumber1;
        return false;
    }

    /* https://www.waermepumpe.de/normen-technik/sg-ready/
     *
     * Off: 1,0
     * Low: 0,0
     * Standard: 0,1
     * High: 1,1
     */

    if (gpio1Enabled && !gpio2Enabled) {
        m_sgReadyMode = SgReadyModeOff;
    } else if (!gpio1Enabled && !gpio2Enabled) {
        m_sgReadyMode = SgReadyModeLow;
    } else if (!gpio1Enabled && gpio2Enabled) {
        m_sgReadyMode = SgReadyModeStandard;
    } else {
        m_sgReadyMode = SgReadyModeHigh;
    }

    emit sgReadyModeChanged(m_sgReadyMode);

    return true;
}

bool SgReadyInterface::isValid() const
{
    return m_gpioNumber1 >= 0 && m_gpioNumber2 >= 0 && m_gpio1 && m_gpio2;
}

Gpio *SgReadyInterface::gpio1() const
{
    return m_gpio1;
}

Gpio *SgReadyInterface::gpio2() const
{
    return m_gpio2;
}

Gpio *SgReadyInterface::setupGpio(int gpioNumber, bool initialValue)
{
    if (gpioNumber < 0) {
        qCWarning(dcSgReady()) << "Invalid gpio number for setting up gpio" << gpioNumber;
        return nullptr;
    }

    // Create and configure gpio
    Gpio *gpio = new Gpio(gpioNumber, this);
    if (!gpio->exportGpio()) {
        qCWarning(dcSgReady()) << "Could not export gpio" << gpioNumber;
        gpio->deleteLater();
        return nullptr;
    }

    if (!gpio->setDirection(Gpio::DirectionOutput)) {
        qCWarning(dcSgReady()) << "Failed to configure gpio" << gpioNumber << "as output";
        gpio->deleteLater();
        return nullptr;
    }

    // Set the pin to the initial value
    if (!gpio->setValue(initialValue ? Gpio::ValueHigh : Gpio::ValueLow)) {
        qCWarning(dcSgReady()) << "Failed to set initially value" << initialValue << "for gpio" << gpioNumber;
        gpio->deleteLater();
        return nullptr;
    }

    return gpio;
}
