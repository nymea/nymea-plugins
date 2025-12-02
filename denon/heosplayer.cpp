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

#include "heosplayer.h"

HeosPlayer::HeosPlayer(int playerId) :
    m_playerId(playerId)
{

}

HeosPlayer::HeosPlayer(int playerId, const QString &name, const QString &serialNumber) :
    m_playerId(playerId),
    m_serialNumber(serialNumber),
    m_name(name)
{
}

QString HeosPlayer::name()
{
    return m_name;
}

void HeosPlayer::setName(const QString &name)
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

void HeosPlayer::setPlayerModel(const QString &playerModel)
{
    m_playerModel = playerModel;
}

QString HeosPlayer::playerVersion()
{
    return m_playerVersion;
}

void HeosPlayer::setPlayerVersion(const QString &playerVersion)
{
    m_playerVersion = playerVersion;
}

QString HeosPlayer::network()
{
    return m_network;
}

void HeosPlayer::setNetwork(const QString &network)
{
    m_network = network;
}

QString HeosPlayer::serialNumber()
{
    return m_serialNumber;
}

void HeosPlayer::setSerialNumber(const QString &serialNumber)
{
    m_serialNumber = serialNumber;
}

QString HeosPlayer::lineOut()
{
    return m_lineOut;
}

void HeosPlayer::setLineOut(const QString &lineout)
{
    m_lineOut = lineout;
}

QString HeosPlayer::control()
{
    return m_control;
}

void HeosPlayer::setControl(const QString &control)
{
    m_control = control;
}


