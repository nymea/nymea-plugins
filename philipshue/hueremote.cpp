/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "hueremote.h"
#include "extern-plugininfo.h"

HueRemote::HueRemote(QObject *parent) :
    HueDevice(parent)
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
        // Hue Tap doesn't have a reachable property as it's a ultra low power device. Let's mark it reachable by default as we only
        // get this response if the bridge is reachable.
        setReachable(true);
    }
    setBattery(configMap.value("battery", 0).toInt());

    emit stateChanged();

    QString lastUpdate = statesMap.value("lastupdated").toString();
    if (m_lastUpdate != lastUpdate) {
        m_lastUpdate = lastUpdate;

        int buttonCode = statesMap.value("buttonevent").toInt();
        qCDebug(dcPhilipsHue) << "button pressed" << buttonCode;

        emit buttonPressed(buttonCode);
    }
}

