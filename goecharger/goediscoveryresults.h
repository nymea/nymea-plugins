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

#ifndef GOEDISCOVERYRESULTS_H
#define GOEDISCOVERYRESULTS_H

#include <QHash>
#include <QHostAddress>
#include <QMap>

namespace GoeDiscoveryData {
// Higher priority sources override nonempty metadata from lower priority sources.
enum Source { SourceApiV1, SourceApiV2, SourceZeroConf };

template<typename Result>
class Results
{
public:
    void clear() { m_sources.clear(); }

    void add(const Result &result, Source source)
    {
        merge(m_sources[result.address][source], result);
    }

    QList<Result> results() const
    {
        QList<Result> results;
        for (const auto &sources : m_sources) {
            Result result;
            for (const Result &source : sources)
                merge(result, source);
            if (result.serialNumber.isEmpty() || (!result.apiAvailableV1 && !result.apiAvailableV2))
                continue;
            if (result.product.isEmpty())
                result.product = "go-eCharger";
            if (result.manufacturer.isEmpty())
                result.manufacturer = "go-e";
            results.append(result);
        }
        return results;
    }

private:
    static void merge(Result &target, const Result &incoming)
    {
        if (!incoming.product.isEmpty())
            target.product = incoming.product;
        if (!incoming.manufacturer.isEmpty())
            target.manufacturer = incoming.manufacturer;
        if (!incoming.friendlyName.isEmpty())
            target.friendlyName = incoming.friendlyName;
        if (!incoming.serialNumber.isEmpty())
            target.serialNumber = incoming.serialNumber;
        if (!incoming.firmwareVersion.isEmpty())
            target.firmwareVersion = incoming.firmwareVersion;
        target.apiAvailableV1 |= incoming.apiAvailableV1;
        target.apiAvailableV2 |= incoming.apiAvailableV2;
        target.address = incoming.address;
        target.discoveryMethod = incoming.discoveryMethod;
    }

    QHash<QHostAddress, QMap<Source, Result>> m_sources;
};
}
#endif
