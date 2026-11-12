// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026, chargebyte austria GmbH

#ifndef EVERESTRFIDUTILS_H
#define EVERESTRFIDUTILS_H

#include "jsonrpc/everestjsonrpcclient.h"
#include <QHostAddress>

inline bool everestRfidEligible(const QUrl &url, const QList<EverestJsonRpcClient::EVSEInfo> &evses)
{
    if (!(QHostAddress(url.host()).isLoopback() || url.host().compare("localhost", Qt::CaseInsensitive) == 0)
            || evses.size() != 1 || evses.first().availableConnectors.size() != 1)
        return false;
    bool ac = false;
    for (auto mode : evses.first().supportedEnergyTransferModes) {
        switch (mode) {
        case EverestJsonRpcClient::EnergyTransferModeAC_single_phase_core:
        case EverestJsonRpcClient::EnergyTransferModeAC_two_phase:
        case EverestJsonRpcClient::EnergyTransferModeAC_three_phase_core:
        case EverestJsonRpcClient::EnergyTransferModeAC_BPT:
        case EverestJsonRpcClient::EnergyTransferModeAC_BPT_DER:
        case EverestJsonRpcClient::EnergyTransferModeAC_DER:
            ac = true;
            break;
        default:
            // Unknown modes and any DC capability cannot safely select this class.
            return false;
        }
    }
    return ac;
}

#endif // EVERESTRFIDUTILS_H
