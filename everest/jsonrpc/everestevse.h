/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#ifndef EVERESTEVSE_H
#define EVERESTEVSE_H

#include <QObject>

#include "jsonrpc/everestjsonrpcclient.h"

class EverestEvse : public QObject
{
    Q_OBJECT
public:
    explicit EverestEvse(EverestJsonRpcClient *client, Thing *thing, QObject *parent = nullptr);

    int index() const;

    EverestJsonRpcReply *setChargingAllowed(bool allowed);
    EverestJsonRpcReply *setACChargingCurrent(double current);
    EverestJsonRpcReply *setACChargingPhaseCount(int phaseCount);

private:
    EverestJsonRpcClient *m_client = nullptr;
    Thing *m_thing = nullptr;

    int m_index = -1;
    bool m_initialized = false;

    EverestJsonRpcClient::EVSEInfo m_evseInfo;
    EverestJsonRpcClient::EVSEStatus m_evseStatus;
    EverestJsonRpcClient::HardwareCapabilities m_hardwareCapabilities;

    QVector<EverestJsonRpcReply *> m_pendingInitReplies;

    void initialize();
    void evaluateInitFinished(EverestJsonRpcReply *reply);

    void processEvseStatus();
    void processHardwareCapabilities();
};

#endif // EVERESTEVSE_H
