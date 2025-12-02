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

#ifndef HUEDEVICE_H
#define HUEDEVICE_H

#include <QObject>
#include <QDebug>
#include <QHostAddress>
#include <QNetworkRequest>
#include <QJsonDocument>

#include "huebridge.h"


class HueDevice : public QObject
{
    Q_OBJECT
public:
    explicit HueDevice(HueBridge *bridge, QObject *parent = nullptr);

    int id() const;
    void setId(const int &id);

    QHostAddress hostAddress() const;
    QString apiKey() const;

    QString name() const;
    void setName(const QString &name);

    QString uuid();
    void setUuid(const QString &uuid);

    QString modelId() const;
    void setModelId(const QString &modelId);

    QString type() const;
    void setType(const QString &type);

    QString softwareVersion() const;
    void setSoftwareVersion(const QString &softwareVersion);

    bool reachable() const;
    void setReachable(const bool &reachable);

    static QString getBaseUuid(const QString &uuid);

private:
    HueBridge *m_bridge = nullptr;
    int m_id;
    QString m_name;
    QString m_modelId;
    QString m_uuid;
    QString m_type;
    QString m_softwareVersion;

    bool m_reachable;

signals:
    void reachableChanged(bool reachable);

};

#endif // HUEDEVICE_H
