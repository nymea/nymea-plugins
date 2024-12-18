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

#ifndef ESPSOMFYRTSDISCOVERY_H
#define ESPSOMFYRTSDISCOVERY_H

#include <QObject>
#include <QTimer>

#include <network/networkaccessmanager.h>
#include <network/networkdevicediscovery.h>

class EspSomfyRtsDiscovery : public QObject
{
    Q_OBJECT
public:
    explicit EspSomfyRtsDiscovery(NetworkAccessManager *networkManager, NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent = nullptr);

    typedef struct Result {
        QString name;
        QString firmwareVersion;
        QHostAddress address;
        NetworkDeviceInfo networkDeviceInfo;
    } Result;

    void startDiscovery();

    QList<EspSomfyRtsDiscovery::Result> results() const;

signals:
    void discoveryFinished();

private:
    NetworkAccessManager *m_networkManager = nullptr;
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;

    QTimer m_gracePeriodTimer;
    QDateTime m_startDateTime;

    NetworkDeviceInfos m_networkDeviceInfos;
    QList<EspSomfyRtsDiscovery::Result> m_results;

    void checkNetworkDevice(const QHostAddress &address);

    void finishDiscovery();
};

#endif // ESPSOMFYRTSDISCOVERY_H
