/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2023, nymea GmbH
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

#ifndef FRONIUSDISCOVERY_H
#define FRONIUSDISCOVERY_H

#include <QObject>
#include <QTimer>

#include <network/networkdevicediscovery.h>
#include "froniussolarconnection.h"

class FroniusDiscovery : public QObject
{
    Q_OBJECT
public:
    explicit FroniusDiscovery(NetworkAccessManager *networkManager, NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent = nullptr);

    void startDiscovery();

    QList<NetworkDeviceInfo> discoveryResults() const;

signals:
    void discoveryFinished();

private:
    NetworkAccessManager *m_networkManager = nullptr;
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;

    QTimer m_gracePeriodTimer;
    QDateTime m_startDateTime;

    QList<FroniusSolarConnection *> m_connections;

    NetworkDeviceInfos m_networkDeviceInfos;
    QList<QHostAddress> m_discoveredAddresses;
    QList<NetworkDeviceInfo> m_discoveryResults;

    void checkHostAddress(const QHostAddress &address);
    void cleanupConnection(FroniusSolarConnection *connection);

    void finishDiscovery();
};

#endif // FRONIUSDISCOVERY_H
