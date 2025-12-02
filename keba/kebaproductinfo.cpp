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

#include "kebaproductinfo.h"
#include "extern-plugininfo.h"

KebaProductInfo::KebaProductInfo(const QString &productString) :
    m_productString(productString)
{
    // Examples

    // BMW-10-EC240522-E1R
    // KC-P30-EC240122-E0R
    // KC-P30-EC220112-000-DE
    // KC-P30-EC2404B2-M0A-GE
    // KC-P30-EC2204U2-E00-PV

    qCDebug(dcKeba()) << "Parsing product information from" << productString.count() << productString;
    if (m_productString.count() < 19) {
        qCWarning(dcKeba()) << "Invalid product information string size for" << productString << ". Cannot parse.";
        m_isValid = false;
        return;
    }

    QStringList subStrings = productString.split('-');
    if (subStrings.count() < 4) {
        qCWarning(dcKeba()) << "Invalid product information format" << subStrings << ". Cannot parse" << productString;
        m_isValid = false;
        return;
    }

    // 1. Manufacturer
    // 2. Model
    // 3. Desciptor
    // 4. Meter infos

    // Parse the product string according to Keba Product code definitions
    m_manufacturer = subStrings.at(0);
    if (m_manufacturer == "KC") {
        m_manufacturer = "KeConnect";
    }
    qCDebug(dcKeba()) << "Manufacturer:" << m_manufacturer;
    m_model = subStrings.at(1);
    qCDebug(dcKeba()) << "Model:" << m_model;

    QString descriptor = subStrings.at(2); // EC240522
    m_countryCode = descriptor.at(0); // E
    qCDebug(dcKeba()) << "Country:" << m_countryCode;

    QChar connectorValue = descriptor.at(1);
    if (connectorValue.toLower() == QChar('s')) {
        m_connector = ConnectorSocket;
        qCDebug(dcKeba()) << "Connector: Socket";
    } else if (connectorValue.toLower() == QChar('c')) {
        m_connector = ConnectorCable;
        qCDebug(dcKeba()) << "Connector: Cable";
    } else {
        m_isValid = false;
        return;
    }

    QChar connectorTypeValue = descriptor.at(2);
    if (connectorTypeValue.isDigit() && connectorTypeValue == QChar('1')) {
        m_connectorType = Type1;
    } else if (connectorTypeValue.isDigit() && connectorTypeValue == QChar('2')) {
        m_connectorType = Type2;
    } else if (connectorTypeValue.toLower() == QChar('s')) {
        m_connectorType = Shutter;
    } else {
        m_isValid = false;
        return;
    }

    qCDebug(dcKeba()) << "Connector type:" << m_connectorType;

    QChar connectorCurrentValue = descriptor.at(3);
    if (connectorCurrentValue.isDigit() && connectorTypeValue == QChar('1')) {
        m_current = Current13A;
    } else if (connectorCurrentValue.isDigit() && connectorTypeValue == QChar('2')) {
        m_current = Current16A;
    } else if (connectorCurrentValue.isDigit() && connectorTypeValue == QChar('3')) {
        m_current = Current20A;
    } else if (connectorCurrentValue.isDigit() && connectorTypeValue == QChar('4')) {
        m_current = Current32A;
    } else {
        m_isValid = false;
        return;
    }

    qCDebug(dcKeba()) << "Current:" << m_current;

    // KC-P30-EC24 01 22-E0R
    QString cableValue = descriptor.mid(4, 2);
    if (cableValue == "00") {
        m_cable = NoCable;
        qCDebug(dcKeba()) << "Cable: No cable";
    } else if (cableValue == "01") {
        m_cable = Cable4m;
        qCDebug(dcKeba()) << "Cable: 4 meter";
    } else if (cableValue == "05") {
        m_cable = Cable5m;
        qCDebug(dcKeba()) << "Cable: 5 meter";
    } else if (cableValue == "04") {
        m_cable = Cable6m;
        qCDebug(dcKeba()) << "Cable: 6 meter";
    } else if (cableValue == "07") {
        m_cable = Cable5p5m;
        qCDebug(dcKeba()) << "Cable: 5.5 meter";
    } else {
        m_isValid = false;
        return;
    }

    // KC-P30-EC2401 2 2-E0R
    QChar seriesValue = descriptor.at(6);
    if (seriesValue == QChar('0')) {
        m_series = SeriesE;
        qCDebug(dcKeba()) << "Series: E";
    } else if (seriesValue == QChar('1')) {
        m_series = SeriesB;
        qCDebug(dcKeba()) << "Series: B";
    } else if (seriesValue == QChar('2')) {
        m_series = SeriesC;
        qCDebug(dcKeba()) << "Series: C";
    } else if (seriesValue == QChar('3')) {
        m_series = SeriesA;
        qCDebug(dcKeba()) << "Series: A";
    } else if (seriesValue.toLower() == QChar('b')) {
        m_series = SeriesXWlan;
        qCDebug(dcKeba()) << "Series: X (Wlan)";
    } else if (seriesValue.toLower() == QChar('c')) {
        m_series = SeriesXWlan3G;
        qCDebug(dcKeba()) << "Series: X (Wlan + 3G)";
    } else if (seriesValue.toLower() == QChar('e')) {
        m_series = SeriesXWlan4G;
        qCDebug(dcKeba()) << "Series: X (Wlan + 4G)";
    } else if (seriesValue.toLower() == QChar('g')) {
        m_series = SeriesX3G;
        qCDebug(dcKeba()) << "Series: X (3G)";
    } else if (seriesValue.toLower() == QChar('h')) {
        m_series = SeriesX4G;
        qCDebug(dcKeba()) << "Series: X (4G)";
    } else if (seriesValue.toLower() == QChar('u')) {
        m_series = SeriesSpecial;
        qCDebug(dcKeba()) << "Series: Special" + m_productString.right(2);
    } else {
        qCWarning(dcKeba()) << "Series: Unknown" << productString << "value:" << seriesValue;
        m_isValid = false;
        return;
    }

    // KC-P30-EC24012 2 -E0R
    QChar phaseCountValue = descriptor.at(7);
    if (phaseCountValue == QChar('1')) {
        m_phaseCount = 1;
    } else if (phaseCountValue == QChar('2')) {
        m_phaseCount = 3;
    } else {
        m_isValid = false;
        return;
    }

    qCDebug(dcKeba()) << "Phases:" << m_phaseCount;

    // Meter infos
    QString meterInfos = subStrings.at(3);

    QChar meterValue = meterInfos.at(0);
    if (meterValue == QChar('0')) {
        m_meter = NoMeter;
        qCDebug(dcKeba()) << "Meter: No meter";
    } else if (meterValue.toLower() == QChar('e')) {
        m_meter = MeterNotCalibrated;
        qCDebug(dcKeba()) << "Meter: Not calibrated meter";
    } else if (meterValue.toLower() == QChar('m')) {
        m_meter = MeterCalibrated;
        qCDebug(dcKeba()) << "Meter: Calibrated meter";
    } else if (meterValue.toLower() == QChar('l')) {
        m_meter = MeterCalibratedNationalCertified;
        qCDebug(dcKeba()) << "Meter: Calibrated meter (national certified)";
    } else {
        m_isValid = false;
        return;
    }

    QChar authValue = meterInfos.at(2);
    if (authValue == QChar('0')) {
        m_authorization = NoAuthorization;
        qCDebug(dcKeba()) << "Authorization: No authorization";
    } else if (authValue.toLower() == QChar('r')) {
        m_authorization = Rfid;
        qCDebug(dcKeba()) << "Authorization: RFID";
    } else if (authValue.toLower() == QChar('k')) {
        m_authorization = Key;
        qCDebug(dcKeba()) << "Authorization: Key";
    } else {
        // Note: we don't require this info, so we don't mark it as invalid.
        qCDebug(dcKeba()) << "Authorization: Unknown" << authValue;
    }

    m_germanEdition = m_productString.toUpper().endsWith("DE");
    qCDebug(dcKeba()) << "German Edition:" << m_germanEdition;
}

bool KebaProductInfo::isValid() const
{
    return m_isValid;
}

QString KebaProductInfo::productString() const
{
    return m_productString;
}

QString KebaProductInfo::manufacturer() const
{
    return m_manufacturer;
}

QString KebaProductInfo::model() const
{
    return m_model;
}

QString KebaProductInfo::countryCode() const
{
    return m_countryCode;
}

KebaProductInfo::Connector KebaProductInfo::connector() const
{
    return m_connector;
}

KebaProductInfo::ConnectorType KebaProductInfo::connectorType() const
{
    return m_connectorType;
}

KebaProductInfo::ConnectorCurrent KebaProductInfo::current() const
{
    return m_current;
}

KebaProductInfo::Cable KebaProductInfo::cable() const
{
    return m_cable;
}

KebaProductInfo::Series KebaProductInfo::series() const
{
    return m_series;
}

int KebaProductInfo::phaseCount() const
{
    return m_phaseCount;
}

KebaProductInfo::Meter KebaProductInfo::meter() const
{
    return m_meter;
}

KebaProductInfo::Authorization KebaProductInfo::authorization() const
{
    return m_authorization;
}

bool KebaProductInfo::germanEdition() const
{
    return m_germanEdition;
}
