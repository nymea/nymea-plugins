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
    EverestJsonRpcClient::MeterData m_meterData;

    void initialize();

    void processEvseStatus();
    void processHardwareCapabilities();
    void processMeterData();
};

#endif // EVERESTEVSE_H
