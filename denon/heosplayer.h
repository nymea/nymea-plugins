/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef HEOSPLAYER_H
#define HEOSPLAYER_H

#include <QObject>

class HeosPlayer : public QObject
{
    Q_OBJECT
public:
    explicit HeosPlayer(int playerId, QObject *parent = nullptr);
    explicit HeosPlayer(int playerId, QString name, QString serialNumber, QObject *parent = nullptr);

    QString name();
    void setName(QString name);
    int playerId();
    int groupId();
    void setGroupId(int groupId);
    QString playerModel();
    QString playerVersion();
    QString network();
    QString serialNumber();
    QString lineOut();
    QString control();

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
