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

#ifndef WALLTHERMOSTAT_H
#define WALLTHERMOSTAT_H

#include <QObject>
#include "maxdevice.h"

class WallThermostat : public MaxDevice
{
    Q_OBJECT
public:
    explicit WallThermostat(QObject *parent = 0);

    double comfortTemp() const;
    void setComfortTemp(const double &comfortTemp);

    double ecoTemp() const;
    void setEcoTemp(const double &ecoTemp);

    double maxSetPointTemp() const;
    void setMaxSetPointTemp(const double &maxSetPointTemp);

    double minSetPointTemp() const;
    void setMinSetPointTemp(const double &minSetPointTemp);

    bool informationValid() const;
    void setInformationValid(const bool &informationValid);

    bool errorOccurred() const;
    void setErrorOccurred(const bool &errorOccurred);

    bool isAnswereToCommand() const;
    void setIsAnswereToCommand(const bool &isAnswereToCommand);

    bool initialized() const;
    void setInitialized(const bool &initialized);

    bool batteryLow() const;
    void setBatteryLow(const bool &batteryLow);

    bool linkStatusOK() const;
    void setLinkStatusOK(const bool &linkStatusOK);

    bool panelLocked() const;
    void setPanelLocked(const bool &panelLocked);

    bool gatewayKnown() const;
    void setGatewayKnown(const bool &gatewayKnown);

    bool dtsActive() const;
    void setDtsActive(const bool &dtsActive);

    int deviceMode() const;
    void setDeviceMode(const int &deviceMode);

    QString deviceModeString() const;

    int valvePosition() const;
    void setValvePosition(const int &valvePosition);

    double setpointTemperature() const;
    void setSetpointTemperatre(const double &setpointTemperature);

    double currentTemperature() const;
    void setCurrentTemperatre(const double &currentTemperature);

private:
    double m_comfortTemp;
    double m_ecoTemp;
    double m_maxSetPointTemp;
    double m_minSetPointTemp;

    bool m_informationValid;
    bool m_errorOccurred;
    bool m_isAnswerToCommand;
    bool m_initialized;
    bool m_batteryLow;
    bool m_linkStatusOK;
    bool m_panelLocked;
    bool m_gatewayKnown;
    bool m_dtsActive;
    int m_deviceMode;
    QString m_deviceModeString;
    int m_valvePosition;
    double m_setpointTemperature;
    double m_currentTemperature;

signals:

public slots:

};

#endif // WALLTHERMOSTAT_H
