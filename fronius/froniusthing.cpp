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

#include "froniusthing.h"

FroniusThing::FroniusThing(Thing *thing, QObject *parent) :
    QObject(parent)
{
  m_thing = thing;
}

QString FroniusThing::name() const
{
    return m_name;
}

void FroniusThing::setName(const QString &name)
{
    m_name = name;
}

QString FroniusThing::hostId() const
{
    return m_hostId;
}

void FroniusThing::setHostId(const QString &hostId)
{
    m_hostId = hostId;
}

QString FroniusThing::hostAddress() const
{
    return m_hostAddress;
}

void FroniusThing::setHostAddress(const QString &hostAddress)
{
    m_hostAddress = hostAddress;
}

QString FroniusThing::baseUrl() const
{
    return m_baseUrl;
}

void FroniusThing::setBaseUrl(const QString &baseUrl)
{
    m_baseUrl = baseUrl;
}

QString FroniusThing::uniqueId() const
{
    return m_uniqueId;
}

void FroniusThing::setUniqueId(const QString &uniqueId)
{
    m_uniqueId = uniqueId;
}

QString FroniusThing::deviceId() const
{
    return m_thingId;
}

void FroniusThing::setDeviceId(const QString &thingId)
{
    m_thingId = thingId;
}

Thing* FroniusThing::pluginThing() const
{
    return m_thing;
}
