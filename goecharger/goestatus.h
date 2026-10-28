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

#ifndef GOESTATUS_H
#define GOESTATUS_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QStringList>

namespace GoeStatus {
inline QStringList pollingFields()
{
    return QStringLiteral("alw,car,ast,tma,eto,wh,upd,fwv,amp,adi,fhz,cbl,ama,var,pnp,nrg,pha").split(',');
}

inline QStringList supportedFields(const QStringList &requested, const QVariantMap &status)
{
    QStringList supported;
    for (const QString &field : requested) {
        if (status.contains(field))
            supported.append(field);
    }
    return supported;
}

inline bool isStatus(const QJsonDocument &document)
{
    if (!document.isObject())
        return false;
    const QJsonObject object = document.object();
    return object.value("alw").isBool()
            && object.value("car").isDouble() && object.value("nrg").isArray();
}

inline bool acceptableReply(QNetworkReply *reply, bool apiV2)
{
    if (reply->error() == QNetworkReply::NoError) {
        if (!apiV2)
            return true;
        const QJsonDocument document = QJsonDocument::fromJson(reply->peek(reply->bytesAvailable()));
        if (!document.isObject())
            return false;
        // A successful status read may be partial, for example during boot.
        // Still reject empty objects and responses containing only an error.
        const QJsonObject object = document.object();
        return object.value("alw").isBool() || object.value("car").isDouble()
                || object.value("nrg").isArray();
    }
    // Only status reads may recover useful data from an unsupported-field response.
    return apiV2 && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 400
            && reply->error() == QNetworkReply::ProtocolInvalidOperationError
            && isStatus(QJsonDocument::fromJson(reply->peek(reply->bytesAvailable())));
}
}
#endif
