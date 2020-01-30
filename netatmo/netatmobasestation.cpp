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

#include "netatmobasestation.h"

#include <QVariantMap>

NetatmoBaseStation::NetatmoBaseStation(const QString &name, const QString &macAddress, QObject *parent) :
    QObject(parent),
    m_name(name),
    m_macAddress(macAddress)
{
}

QString NetatmoBaseStation::name() const
{
    return m_name;
}

QString NetatmoBaseStation::macAddress() const
{
    return m_macAddress;
}

int NetatmoBaseStation::lastUpdate() const
{
    return m_lastUpdate;
}

double NetatmoBaseStation::temperature() const
{
    return m_temperature;
}

double NetatmoBaseStation::minTemperature() const
{
    return m_minTemperature;
}

double NetatmoBaseStation::maxTemperature() const
{
    return m_maxTemperature;
}

double NetatmoBaseStation::pressure() const
{
    return m_pressure;
}

int NetatmoBaseStation::humidity() const
{
    return m_humidity;
}

int NetatmoBaseStation::noise() const
{
    return m_noise;
}

int NetatmoBaseStation::co2() const
{
    return m_co2;
}

int NetatmoBaseStation::wifiStrength() const
{
    return m_wifiStrength;
}

void NetatmoBaseStation::updateStates(const QVariantMap &data)
{
    // check data timestamp
    if (data.contains("last_status_store")) {
        m_lastUpdate = data.value("last_status_store").toInt();
    }

    // update dashboard data
    if (data.contains("dashboard_data")) {
        QVariantMap measurments = data.value("dashboard_data").toMap();
        m_pressure = measurments.value("AbsolutePressure").toDouble();
        m_humidity = measurments.value("Humidity").toInt();
        m_noise = measurments.value("Noise").toInt();
        m_temperature = measurments.value("Temperature").toDouble();
        m_minTemperature = measurments.value("min_temp").toDouble();
        m_maxTemperature = measurments.value("max_temp").toDouble();
        m_co2 = measurments.value("CO2").toInt();
    }
    // update wifi signal strength
    if (data.contains("wifi_status")) {
        int wifiStrength = data.value("wifi_status").toInt();
        if (wifiStrength <= 56) {
            m_wifiStrength = 100;
        } else if (wifiStrength >= 86) {
            m_wifiStrength = 0;
        } else {
            int delta = 30 - (wifiStrength - 56);
            m_wifiStrength = qRound(100.0 * delta / 30.0);
        }
    }
    emit statesChanged();
}
