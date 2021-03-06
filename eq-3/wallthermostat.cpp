/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "wallthermostat.h"
#include "extern-plugininfo.h"

WallThermostat::WallThermostat(QObject *parent) :
    MaxDevice(parent)
{
}

double WallThermostat::comfortTemp() const
{
    return m_comfortTemp;
}

void WallThermostat::setComfortTemp(const double &comfortTemp)
{
    m_comfortTemp = comfortTemp;
}

double WallThermostat::ecoTemp() const
{
    return m_ecoTemp;
}

void WallThermostat::setEcoTemp(const double &ecoTemp)
{
    m_ecoTemp = ecoTemp;
}

double WallThermostat::maxSetPointTemp() const
{
    return m_maxSetPointTemp;
}

void WallThermostat::setMaxSetPointTemp(const double &maxSetPointTemp)
{
    m_maxSetPointTemp = maxSetPointTemp;
}

double WallThermostat::minSetPointTemp() const
{
    return m_minSetPointTemp;
}

void WallThermostat::setMinSetPointTemp(const double &minSetPointTemp)
{
    m_minSetPointTemp = minSetPointTemp;
}

bool WallThermostat::informationValid() const
{
    return m_informationValid;
}

void WallThermostat::setInformationValid(const bool &informationValid)
{
    m_informationValid = informationValid;
}

bool WallThermostat::errorOccurred() const
{
    return m_errorOccurred;
}

void WallThermostat::setErrorOccurred(const bool &errorOccurred)
{
    m_errorOccurred = errorOccurred;
}

bool WallThermostat::isAnswereToCommand() const
{
    return m_isAnswerToCommand;
}

void WallThermostat::setIsAnswereToCommand(const bool &isAnswereToCommand)
{
    m_isAnswerToCommand = isAnswereToCommand;
}

bool WallThermostat::initialized() const
{
    return m_initialized;
}

void WallThermostat::setInitialized(const bool &initialized)
{
    m_initialized = initialized;
}

bool WallThermostat::batteryLow() const
{
    return m_batteryLow;
}

void WallThermostat::setBatteryLow(const bool &batteryLow)
{
    m_batteryLow = batteryLow;
}

bool WallThermostat::linkStatusOK() const
{
    return m_linkStatusOK;
}

void WallThermostat::setLinkStatusOK(const bool &linkStatusOK)
{
    m_linkStatusOK = linkStatusOK;
}

bool WallThermostat::panelLocked() const
{
    return m_panelLocked;
}

void WallThermostat::setPanelLocked(const bool &panelLocked)
{
    m_panelLocked = panelLocked;
}

bool WallThermostat::gatewayKnown() const
{
    return m_gatewayKnown;
}

void WallThermostat::setGatewayKnown(const bool &gatewayKnown)
{
    m_gatewayKnown = gatewayKnown;
}

bool WallThermostat::dtsActive() const
{
    return m_dtsActive;
}

void WallThermostat::setDtsActive(const bool &dtsActive)
{
    m_dtsActive = dtsActive;
}

int WallThermostat::deviceMode() const
{
    return m_deviceMode;
}

void WallThermostat::setDeviceMode(const int &deviceMode)
{
    m_deviceMode = deviceMode;

    switch (deviceMode) {
    case Auto:
        m_deviceModeString = "Auto";
        break;
    case Manual:
        m_deviceModeString = "Manuel";
        break;
    case Temporary:
        m_deviceModeString = "Temporary";
        break;
    case Boost:
        m_deviceModeString = "Boost";
        break;
    default:
        m_deviceModeString = "-";
        break;
    }
}

QString WallThermostat::deviceModeString() const
{
    return m_deviceModeString;
}

double WallThermostat::setpointTemperature() const
{
    return m_setpointTemperature;
}

void WallThermostat::setSetpointTemperatre(const double &setpointTemperature)
{
    m_setpointTemperature = setpointTemperature;
}

double WallThermostat::currentTemperature() const
{
    return m_currentTemperature;
}

void WallThermostat::setCurrentTemperatre(const double &currentTemperature)
{
    m_currentTemperature = currentTemperature;
}
