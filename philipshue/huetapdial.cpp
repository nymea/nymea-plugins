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

#include "huetapdial.h"
#include "extern-plugininfo.h"

#include <QtMath>

HueTapDial::HueTapDial(HueBridge *bridge, QObject *parent) :
    HueDevice(bridge, parent)
{
}

int HueTapDial::rotaryId() const
{
    return m_rotaryId;
}

void HueTapDial::setRotaryId(int sensorId)
{
    m_rotaryId = sensorId;
}

QString HueTapDial::rotaryUuid() const
{
    return m_rotaryUuid;
}

void HueTapDial::setRotaryUuid(const QString &rotaryUuid)
{
    m_rotaryUuid = rotaryUuid;
}

int HueTapDial::switchId() const
{
    return m_switchId;
}

void HueTapDial::setSwitchId(int sensorId)
{
    m_switchId = sensorId;
}

QString HueTapDial::switchUuid() const
{
    return m_switchUuid;
}

void HueTapDial::setSwitchUuid(const QString &switchUuid)
{
    m_switchUuid = switchUuid;
}

int HueTapDial::level() const
{
    return m_level;
}

int HueTapDial::batteryLevel() const
{
    return m_batteryLevel;
}

void HueTapDial::updateStates(const QVariantMap &sensorMap)
{
    qCDebug(dcPhilipsHue()) << "Hue Tap Dial data:" << qUtf8Printable(QJsonDocument::fromVariant(sensorMap).toJson(QJsonDocument::Indented));

    // Config
    QVariantMap configMap = sensorMap.value("config").toMap();
    if (configMap.contains("reachable")) {
        setReachable(configMap.value("reachable", false).toBool());
    }

    if (configMap.contains("battery")) {
        int batteryLevel = configMap.value("battery", 0).toInt();
        if (m_batteryLevel != batteryLevel) {
            m_batteryLevel = batteryLevel;
            emit batteryLevelChanged(m_batteryLevel);
        }
    }

    // States
    QVariantMap stateMap = sensorMap.value("state").toMap();

    // If rotated
    if (sensorMap.value("uniqueid").toString() == m_rotaryUuid) {
        QString lastUpdateRotation = stateMap.value("lastupdated").toString();
        int rotationCode = stateMap.value("expectedrotation").toInt();

        // If we never polled, just store lastUpdate/rotationCode and not emit a false rotated event
        if (m_lastUpdateRotation.isEmpty() || m_lastRotationCode == 0) {
            m_lastUpdateRotation = lastUpdateRotation;
            m_lastRotationCode = rotationCode;
        }
        if (m_lastUpdateRotation != lastUpdateRotation || m_lastRotationCode != rotationCode) {
            m_lastUpdateRotation = lastUpdateRotation;
            m_lastRotationCode = rotationCode;
            qCDebug(dcPhilipsHue) << "rotated" << rotationCode;
            emit rotated(rotationCode);
        }
    }

    // If button press
    if (sensorMap.value("uniqueid").toString() == m_switchUuid) {
        QString lastUpdateButton = stateMap.value("lastupdated").toString();
        int buttonCode = stateMap.value("buttonevent").toInt();

        // If we never polled, just store lastUpdate/buttonCode and not emit a false button pressed event
        if (m_lastUpdateButton.isEmpty() || m_lastButtonCode == -1) {
            m_lastUpdateButton = lastUpdateButton;
            m_lastButtonCode = buttonCode;
        }
        if (m_lastUpdateButton != lastUpdateButton || m_lastButtonCode != buttonCode) {
            m_lastUpdateButton = lastUpdateButton;
            m_lastButtonCode = buttonCode;
            qCDebug(dcPhilipsHue) << "button pressed" << buttonCode;
            emit buttonPressed(buttonCode);
        }
    }
}

bool HueTapDial::isValid()
{
    return !m_rotaryUuid.isEmpty() && !m_switchUuid.isEmpty();
}

bool HueTapDial::hasSensor(int sensorId)
{
    return m_rotaryId == sensorId || m_switchId == sensorId;
}

bool HueTapDial::hasSensor(const QString &sensorUuid)
{
    return m_rotaryUuid == sensorUuid || m_switchUuid == sensorUuid;
}
