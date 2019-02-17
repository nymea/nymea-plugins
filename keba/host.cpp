/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "host.h"

Host::Host()
{
    qRegisterMetaType<Host>();
    qRegisterMetaType<QList<Host> >();
}

QString Host::macAddress() const
{
    return m_macAddress;
}

void Host::setMacAddress(const QString &macAddress)
{
    m_macAddress = macAddress;
}

QString Host::hostName() const
{
    return m_hostName;
}

void Host::setHostName(const QString &hostName)
{
    m_hostName = hostName;
}

QString Host::address() const
{
    return m_address;
}

void Host::setAddress(const QString &address)
{
    m_address = address;
}

void Host::seen()
{
    m_lastSeenTime = QDateTime::currentDateTime();
}

QDateTime Host::lastSeenTime() const
{
    return m_lastSeenTime;
}

bool Host::reachable() const
{
    return m_reachable;
}

void Host::setReachable(bool reachable)
{
    m_reachable = reachable;
}

QDebug operator<<(QDebug dbg, const Host &host)
{
    dbg.nospace() << "Host(" << host.macAddress() << "," << host.hostName() << ", " << host.address() << ", " << (host.reachable() ? "up" : "down") << ")";
    return dbg.space();
}
