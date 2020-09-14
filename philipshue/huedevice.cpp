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

#include "huedevice.h"

HueDevice::HueDevice(HueBridge *bridge, QObject *parent) :
    QObject(parent),
    m_bridge(bridge)
{

}

int HueDevice::id() const
{
    return m_id;
}

void HueDevice::setId(const int &id)
{
    m_id = id;
}

QHostAddress HueDevice::hostAddress() const
{
    return m_bridge->hostAddress();
}

QString HueDevice::apiKey() const
{
    return m_bridge->apiKey();
}

QString HueDevice::name() const
{
    return m_name;
}

void HueDevice::setName(const QString &name)
{
    m_name = name;
}

QString HueDevice::uuid()
{
    return m_uuid;
}

void HueDevice::setUuid(const QString &uuid)
{
    m_uuid = uuid;
}

QString HueDevice::modelId() const
{
    return m_modelId;
}

void HueDevice::setModelId(const QString &modelId)
{
    m_modelId = modelId;
}

QString HueDevice::type() const
{
    return m_type;
}

void HueDevice::setType(const QString &type)
{
    m_type = type;
}

QString HueDevice::softwareVersion() const
{
    return m_softwareVersion;
}

void HueDevice::setSoftwareVersion(const QString &softwareVersion)
{
    m_softwareVersion = softwareVersion;
}

bool HueDevice::reachable() const
{
    return m_reachable;
}

void HueDevice::setReachable(const bool &reachable)
{
    if (m_reachable == reachable)
        return;

    m_reachable = reachable;
    emit reachableChanged(m_reachable);
}

QString HueDevice::getBaseUuid(const QString &uuid)
{
    // Example: the hue gives uuid's starting with a mac address, followd by an id
    //          "00:17:88:01:06:44:36:86-02-0406" -> "00:17:88:01:06:44:36:86"
    int dashIndex = uuid.indexOf("-");
    if (dashIndex < 0) {
        return uuid;
    }

    return uuid.left(dashIndex);
}

