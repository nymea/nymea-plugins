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

    // If we never polled, just store lastUpdate/buttonCode and not emit a falsely button pressed event
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

