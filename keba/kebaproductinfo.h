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

#ifndef KEBAPRODUCTINFO_H
#define KEBAPRODUCTINFO_H

#include <QObject>
#include <QString>

class KebaProductInfo
{
    Q_GADGET
public:
    enum Connector {
        ConnectorSocket,
        ConnectorCable
    };
    Q_ENUM(Connector)

    enum ConnectorType {
        Type1,
        Type2,
        Shutter
    };
    Q_ENUM(ConnectorType)

    enum ConnectorCurrent {
        Current13A = 1,
        Current16A = 2,
        Current20A = 3,
        Current32A = 4
    };
    Q_ENUM(ConnectorCurrent)

    enum Cable {
        NoCable = 0,
        Cable4m = 1,
        Cable6m = 4,
        Cable5m = 5,
        Cable5p5m = 7
    };
    Q_ENUM(Cable)

    enum Series {
        SeriesE,
        SeriesB,
        SeriesC,
        SeriesA,
        SeriesXWlan,
        SeriesXWlan3G,
        SeriesXWlan4G,
        SeriesX3G,
        SeriesX4G,
        SeriesSpecial
    };
    Q_ENUM(Series)

    enum Meter {
        NoMeter,
        MeterNotCalibrated,
        MeterCalibrated,
        MeterCalibratedNationalCertified
    };
    Q_ENUM(Meter)

    enum Authorization {
        NoAuthorization,
        Rfid,
        Key
    };
    Q_ENUM(Authorization)

    KebaProductInfo(const QString &productString);

    bool isValid() const;

    QString productString() const;

    // Porperties in the string
    QString manufacturer() const; // KC (KeConnect), BMW...
    QString model() const; // P30
    QString countryCode() const; // E
    Connector connector() const; // Socket / Cable
    ConnectorType connectorType() const; // Type 1 / Type 2
    ConnectorCurrent current() const; // 13A, 16A ...
    Cable cable() const; // 4m, 6m...
    Series series() const; // x, c, a...
    int phaseCount() const; // 1 or 3
    Meter meter() const; // No meter, Calibrated, ...
    Authorization authorization() const;
    bool germanEdition() const;

private:
    bool m_isValid = true;

    QString m_productString;
    QString m_manufacturer;
    QString m_model;
    QString m_countryCode;
    Connector m_connector;
    ConnectorType m_connectorType;
    ConnectorCurrent m_current;
    Cable m_cable;
    Series m_series;
    int m_phaseCount;
    Meter m_meter;
    Authorization m_authorization;
    bool m_germanEdition = false;
};

#endif // KEBAPRODUCTINFO_H
