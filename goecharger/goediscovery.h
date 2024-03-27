/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2024, nymea GmbH
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

#ifndef GOEDISCOVERY_H
#define GOEDISCOVERY_H

#include <QObject>
#include <QDebug>

#include <network/networkaccessmanager.h>
#include <network/networkdevicediscovery.h>
#include <network/networkdevicediscovery.h>
#include <platform/platformzeroconfcontroller.h>
#include <network/zeroconf/zeroconfservicebrowser.h>

class GoeDiscovery : public QObject
{
    Q_OBJECT
public:
    enum DiscoveryMethod {
        DiscoveryMethodNetwork,
        DiscoveryMethodZeroConf,
    };
    Q_ENUM(DiscoveryMethod)

    typedef struct Result {
        QString product = "go-eCharger";
        QString manufacturer = "go-e";
        QString friendlyName;
        QString serialNumber;
        QString firmwareVersion;
        DiscoveryMethod discoveryMethod;
        NetworkDeviceInfo networkDeviceInfo; // Network discovery
        QHostAddress address; // ZeroConf
        bool apiAvailableV1 = false;
        bool apiAvailableV2 = false;
    } Result;

    explicit GoeDiscovery(NetworkAccessManager *networkAccessManager, NetworkDeviceDiscovery *networkDeviceDiscovery, ZeroConfServiceBrowser *serviceBrowser, QObject *parent = nullptr);
    ~GoeDiscovery();

    void startDiscovery();

    QList<GoeDiscovery::Result> discoveryResults() const;

    static QNetworkRequest buildRequestV1(const QHostAddress &address);
    static QNetworkRequest buildRequestV2(const QHostAddress &address);

    // Zeroconf service helpers
    static bool isGoeCharger(const ZeroConfServiceEntry &serviceEntry);

signals:
    void discoveryFinished();

private:
    QDateTime m_startDateTime;
    NetworkAccessManager *m_networkAccessManager = nullptr;
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;
    NetworkDeviceDiscoveryReply *m_discoveryReply = nullptr;
    ZeroConfServiceBrowser *m_serviceBrowser = nullptr;

    QHash<QHostAddress, GoeDiscovery::Result> m_discoveryResults;
    NetworkDeviceInfos m_discoveredNetworkDeviceInfos;
    NetworkDeviceInfos m_verifiedNetworkDeviceInfos;

    QList<QNetworkReply *> m_pendingReplies;

private slots:
    void checkNetworkDevice(const NetworkDeviceInfo &networkDeviceInfo);
    void checkNetworkDeviceApiV1(const NetworkDeviceInfo &networkDeviceInfo);
    void checkNetworkDeviceApiV2(const NetworkDeviceInfo &networkDeviceInfo);

    void onServiceEntryAdded(const ZeroConfServiceEntry &serviceEntry);

    void cleanupPendingReplies();

    void finishDiscovery();
};

QDebug operator<<(QDebug debug, const GoeDiscovery::Result &result);

#endif // GOEDISCOVERY_H
