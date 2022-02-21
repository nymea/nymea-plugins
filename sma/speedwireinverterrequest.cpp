/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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

#include "speedwireinverterrequest.h"

SpeedwireInverterRequest::SpeedwireInverterRequest()
{

}

Speedwire::Command SpeedwireInverterRequest::command() const
{
    return m_command;
}

void SpeedwireInverterRequest::setCommand(Speedwire::Command command)
{
    m_command = command;
}

quint16 SpeedwireInverterRequest::packetId() const
{
    return m_packetId;
}

void SpeedwireInverterRequest::setPacketId(quint16 packetId)
{
    m_packetId = packetId;
}

QByteArray SpeedwireInverterRequest::requestData() const
{
    return m_requestData;
}

void SpeedwireInverterRequest::setRequestData(const QByteArray &requestData)
{
    m_requestData = requestData;
}

quint8 SpeedwireInverterRequest::retries() const
{
    return m_retries;
}

void SpeedwireInverterRequest::setRetries(quint8 retries)
{
    m_retries = retries;
}
