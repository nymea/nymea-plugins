/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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

#include "kebaproductinfo.h"
#include "extern-plugininfo.h"

KebaProductInfo::KebaProductInfo(const QString &productString) :
    m_productString(productString)
{
    qCDebug(dcKeba()) << "Parsing product information from" << productString.count() << productString;
    if (m_productString.count() < 19) {
        qCWarning(dcKeba()) << "Invalid product information string size for" << productString << ". Cannot parse.";
        m_isValid = false;
        return;
    }

    // Parse the product string according to Keba Product code definitions
    m_model = m_productString.mid(3, 3);
    qCDebug(dcKeba()) << "Model:" << m_model;
    m_countryCode = m_productString.at(7);
    qCDebug(dcKeba()) << "Country:" << m_countryCode;

    QChar connectorValue = m_productString.at(8);
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

    QChar connectorTypeValue = m_productString.at(9);
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

    QChar connectorCurrentValue = m_productString.at(10);
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

    QString cableValue = m_productString.mid(11, 2);
    if (cableValue == "00") {
        m_cable = NoCable;
        qCDebug(dcKeba()) << "Cable: No cable";
    } else if (cableValue == "01") {
        m_cable = Cable4m;
        qCDebug(dcKeba()) << "Cable: 4 meter";
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

    QChar seriesValue = m_productString.at(13);
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
    } else {
        m_isValid = false;
        return;
    }

    QChar phaseCountValue = m_productString.at(14);
    if (phaseCountValue == QChar('1')) {
        m_phaseCount = 1;
    } else if (phaseCountValue == QChar('2')) {
        m_phaseCount = 3;
    } else {
        m_isValid = false;
        return;
    }

    qCDebug(dcKeba()) << "Phases:" << m_phaseCount;

    QChar meterValue = m_productString.at(16);
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


    QChar authValue = m_productString.at(18);
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
        m_isValid = false;
        return;
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
