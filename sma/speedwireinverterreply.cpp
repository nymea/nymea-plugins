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

#include "speedwireinverterreply.h"
#include "extern-plugininfo.h"

SpeedwireInverterReply::SpeedwireInverterReply(const SpeedwireInverterRequest &request, QObject *parent) :
    QObject(parent),
    m_request(request)
{
    m_maxRetries = m_request.retries();

    m_timer.setInterval(m_timeout);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SpeedwireInverterReply::timeout);
}

SpeedwireInverterRequest SpeedwireInverterReply::request() const
{
    return m_request;
}

SpeedwireInverterReply::Error SpeedwireInverterReply::error() const
{
    return m_error;
}

QByteArray SpeedwireInverterReply::responseData() const
{
    return m_responseData;
}

Speedwire::Header SpeedwireInverterReply::responseHeader() const
{
    return m_responseHeader;
}

Speedwire::InverterPacket SpeedwireInverterReply::responsePacket() const
{
    return m_responsePacket;
}

QByteArray SpeedwireInverterReply::responsePayload() const
{
    return m_responsePayload;
}

void SpeedwireInverterReply::startWaiting()
{
    m_timer.start();
}

void SpeedwireInverterReply::finishReply(Error error)
{
    m_timer.stop();
    m_error = error;
    emit finished();
}
