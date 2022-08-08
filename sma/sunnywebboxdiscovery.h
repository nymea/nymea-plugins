/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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

#ifndef SUNNYWEBBOXDISCOVERY_H
#define SUNNYWEBBOXDISCOVERY_H

#include <QObject>

#include <network/networkaccessmanager.h>
#include <network/networkdevicediscovery.h>

class SunnyWebBoxDiscovery : public QObject
{
    Q_OBJECT
public:
    explicit SunnyWebBoxDiscovery(NetworkAccessManager *networkAccessManager, NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent = nullptr);

    void startDiscovery();

    NetworkDeviceInfos discoveryResults() const;

signals:
    void discoveryFinished();

private slots:
    void checkNetworkDevice(const NetworkDeviceInfo &networkDeviceInfo);
    void cleanupPendingReplies();
    void finishDiscovery();

private:
    NetworkAccessManager *m_networkAccessManager = nullptr;
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;
    NetworkDeviceDiscoveryReply *m_discoveryReply = nullptr;

    NetworkDeviceInfos m_discoveryResults;
    NetworkDeviceInfos m_discoveredNetworkDeviceInfos;
    NetworkDeviceInfos m_verifiedNetworkDeviceInfos;

    QDateTime m_startDateTime;
    QList<QNetworkReply *> m_pendingReplies;

};

#endif // SUNNYWEBBOXDISCOVERY_H
