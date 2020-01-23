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


