/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#include "heosplayer.h"

HeosPlayer::HeosPlayer(int playerId, QObject *parent) :
    QObject(parent),
    m_playerId(playerId)
{

}

HeosPlayer::HeosPlayer(int playerId, QString name, QString serialNumber, QObject *parent) :
    QObject(parent),
    m_playerId(playerId),
    m_serialNumber(serialNumber),
    m_name(name)
{

}

QString HeosPlayer::name()
{
    return m_name;
}

void HeosPlayer::setName(QString name)
{
    m_name = name;
}

int HeosPlayer::playerId()
{
    return m_playerId;
}

int HeosPlayer::groupId()
{
    return m_groupId;
}

void HeosPlayer::setGroupId(int groupId)
{
    m_groupId = groupId;
}

QString HeosPlayer::playerModel()
{
    return m_playerModel;
}

QString HeosPlayer::playerVersion()
{
    return m_playerVersion;
}

QString HeosPlayer::network()
{
    return m_network;
}

QString HeosPlayer::serialNumber()
{
    return m_serialNumber;
}

QString HeosPlayer::lineOut()
{
    return m_lineOut;
}

QString HeosPlayer::control()
{
    return m_control;
}


