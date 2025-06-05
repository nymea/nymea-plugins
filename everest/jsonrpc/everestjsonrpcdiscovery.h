/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#ifndef EVERESTJSONRPCDISCOVERY_H
#define EVERESTJSONRPCDISCOVERY_H

#include <QObject>

#include <network/networkdevicediscovery.h>

#include "jsonrpc/everestjsonrpcclient.h"

class EverestJsonRpcDiscovery : public QObject
{
    Q_OBJECT
public:
    typedef struct Result {
        QHostAddress address;
        NetworkDeviceInfo networkDeviceInfo;
    } Result;

    explicit EverestJsonRpcDiscovery(NetworkDeviceDiscovery *networkDeviceDiscovery, quint16 port = 8080, QObject *parent = nullptr);

    void start();
    void startLocalhost();

    QList<EverestJsonRpcDiscovery::Result> results() const;

signals:
    void finished();

private:
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;
    quint16 m_port = 8080;

    QDateTime m_startDateTime;
    NetworkDeviceInfos m_networkDeviceInfos;
    QList<EverestJsonRpcClient *> m_clients;
    QList<EverestJsonRpcDiscovery::Result> m_results;

    bool m_localhostDiscovery = false;

    void checkHostAddress(const QHostAddress &address);
    void cleanupClient(EverestJsonRpcClient *client);
    void finishDiscovery();

};

#endif // EVERESTJSONRPCDISCOVERY_H
