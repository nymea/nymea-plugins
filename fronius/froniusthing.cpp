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
