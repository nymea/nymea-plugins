/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
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

#ifndef KEBADISCOVERY_H
#define KEBADISCOVERY_H

#include <QTimer>
#include <QObject>

#include <network/networkdeviceinfos.h>

class KeContactDataLayer;
class NetworkDeviceDiscovery;

class KebaDiscovery : public QObject
{
    Q_OBJECT
public:
    typedef struct KebaDiscoveryResult {
        QString product;
        QString serialNumber;
        QString firmwareVersion;
        NetworkDeviceInfo networkDeviceInfo;
    } KebaDiscoveryResult;

    explicit KebaDiscovery(KeContactDataLayer *kebaDataLayer, NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent = nullptr);
    ~KebaDiscovery();

    void startDiscovery();

    QList<KebaDiscoveryResult> discoveryResults() const;

signals:
    void discoveryFinished();

private:
    KeContactDataLayer *m_kebaDataLayer = nullptr;
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;
    QTimer m_responseTimer;

    NetworkDeviceInfos m_networkDeviceInfos;
    NetworkDeviceInfos m_verifiedNetworkDeviceInfos;
    QList<KebaDiscoveryResult> m_results;

    bool alreadyDiscovered(const QHostAddress &address);

    void cleanup();

private slots:
    void sendReportRequest(const NetworkDeviceInfo &networkDeviceInfo);

};

#endif // KEBADISCOVERY_H
