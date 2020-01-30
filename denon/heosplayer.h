/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
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
