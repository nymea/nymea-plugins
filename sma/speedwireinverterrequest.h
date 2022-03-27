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

#ifndef SPEEDWIREINVERTERREQUEST_H
#define SPEEDWIREINVERTERREQUEST_H

#include <QObject>

#include "speedwire.h"

class SpeedwireInverterRequest
{
public:
    explicit SpeedwireInverterRequest();

    Speedwire::Command command() const;
    void setCommand(Speedwire::Command command);

    quint16 packetId() const;
    void setPacketId(quint16 packetId);

    QByteArray requestData() const;
    void setRequestData(const QByteArray &requestData);

    quint8 retries() const;
    void setRetries(quint8 retries);

private:
    Speedwire::Command m_command;
    quint16 m_packetId = 0;
    QByteArray m_requestData;
    quint8 m_retries = 2; // Default try 2 times before timeout
};

#endif // SPEEDWIREINVERTERREQUEST_H
