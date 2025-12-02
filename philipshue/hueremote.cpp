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

#include "hueremote.h"
#include "extern-plugininfo.h"

HueRemote::HueRemote(HueBridge *bridge, QObject *parent) :
    HueDevice(bridge, parent)
{
}

int HueRemote::battery() const
{
    return m_battery;
}

void HueRemote::setBattery(const int &battery)
{
    m_battery = battery;
}

void HueRemote::updateStates(const QVariantMap &statesMap, const QVariantMap &configMap)
{
    if (configMap.contains("reachable")) {
        setReachable(configMap.value("reachable", false).toBool());
    } else {
        // Hue Tap doesn't have a reachable property as it's a ultra low power thing. Let's mark it reachable by default as we only
        // get this response if the bridge is reachable.
        setReachable(true);
    }
    setBattery(configMap.value("battery", 0).toInt());

    emit stateChanged();

    QString lastUpdate = statesMap.value("lastupdated").toString();
    int buttonCode = statesMap.value("buttonevent").toInt();

    // If we never polled, just store lastUpdate/buttonCode/rotationCode and not emit a falsely button pressed event
    if (m_lastUpdate.isEmpty() || m_lastButtonCode == -1) {
        m_lastUpdate = lastUpdate;
        m_lastButtonCode = buttonCode;
    }

    if (m_lastUpdate != lastUpdate || m_lastButtonCode != buttonCode) {
        m_lastUpdate = lastUpdate;
        m_lastButtonCode = buttonCode;

        qCDebug(dcPhilipsHue) << "button pressed" << buttonCode;

        emit buttonPressed(buttonCode);
    }
}

