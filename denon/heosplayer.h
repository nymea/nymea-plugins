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

#ifndef HEOSPLAYER_H
#define HEOSPLAYER_H

#include <QString>

class HeosPlayer
{
public:
    explicit HeosPlayer(int playerId);
    explicit HeosPlayer(int playerId, const QString &name, const QString &serialNumber);

    QString name();
    void setName(const QString &name);
    int playerId();
    int groupId();
    void setGroupId(int groupId);
    QString playerModel();
    void setPlayerModel(const QString &playerModel);
    QString playerVersion();
    void setPlayerVersion(const QString &playerVersion);
    QString network();
    void setNetwork(const QString &network);
    QString serialNumber();
    void setSerialNumber(const QString &serialNumber);
    QString lineOut();
    void setLineOut(const QString &lineout);
    QString control();
    void setControl(const QString &control);

private:
    int m_playerId;
    int m_groupId;
    QString m_serialNumber;
    QString m_name;
    QString m_lineOut;
    QString m_network;
    QString m_playerModel;
    QString m_playerVersion;
    QString m_control;
};

#endif // HEOSPLAYER_H
