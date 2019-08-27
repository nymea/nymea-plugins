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

#include "pairinginfo.h"

PairingInfo::PairingInfo(QObject *parent) :
    QObject(parent)
{
}

DevicePairingInfo PairingInfo::pairingInfo() const
{
    return m_pairingInfo;
}

void PairingInfo::setPairingInfo(const DevicePairingInfo &pairingInfo)
{
    m_pairingInfo = pairingInfo;
}

QHostAddress PairingInfo::host() const
{
    return m_host;
}

void PairingInfo::setHost(const QHostAddress &host)
{
    m_host = host;
}

QString PairingInfo::apiKey() const
{
    return m_apiKey;
}

void PairingInfo::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}
