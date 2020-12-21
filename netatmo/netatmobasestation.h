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

#ifndef NETATMOBASESTATION_H
#define NETATMOBASESTATION_H

#include <QObject>
#include <QString>

class NetatmoBaseStation : public QObject
{
    Q_OBJECT
public:
    explicit NetatmoBaseStation(const QString &name, const QString &macAddress, QObject *parent = nullptr);

    // Params
    QString name() const;
    QString macAddress() const;
    QString connectionId() const;

    // States
    int lastUpdate() const;
    double temperature() const;
    double minTemperature() const;
    double maxTemperature() const;
    double pressure() const;
    int humidity() const;
    int noise() const;
    int co2() const;
    int wifiStrength() const;
    bool reachable() const;

    void updateStates(const QVariantMap &data);

private:
    // Params
    QString m_name;
    QString m_macAddress;

    // States
    int m_lastUpdate;
    double m_temperature;
    double m_minTemperature;
    double m_maxTemperature;
    double m_pressure;
    int m_humidity;
    int m_noise;
    int m_co2;
    int m_wifiStrength;
    bool m_reachable;

signals:
    void statesChanged();

};

#endif // NETATMOBASESTATION_H
