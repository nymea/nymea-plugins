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

#ifndef FRONIUSTHING_H
#define FRONIUSTHING_H

#include "extern-plugininfo.h"
#include "integrations/thing.h"

#include <QObject>
#include <QUrl>
#include <QUrlQuery>

class FroniusThing : public QObject
{
    Q_OBJECT

public:
    explicit FroniusThing(Thing *thing, QObject *parrent = 0);

    QString name() const;
    void setName(const QString &name);

    QString hostId() const;
    void setHostId(const QString &hostId);

    QString hostAddress() const;
    void setHostAddress(const QString &hostAddress);

    QString baseUrl() const;
    void setBaseUrl(const QString &baseUrl);

    QString uniqueId() const;
    void setUniqueId(const QString &uniqueId);

    QString deviceId() const;
    void setDeviceId(const QString &deviceId);

    Thing* pluginThing() const;

private:

    Thing* m_thing;

    QString m_name;
    QString m_hostId;
    QString m_hostAddress;
    QString m_apiVersion;
    QString m_baseUrl;
    QString m_uniqueId;
    QString m_thingId;

};

#endif // FRONIUSTHING_H
