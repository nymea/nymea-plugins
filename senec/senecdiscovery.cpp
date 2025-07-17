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

#include "senecdiscovery.h"
#include "extern-plugininfo.h"

SenecDiscovery::SenecDiscovery(NetworkAccessManager *networkManager, NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent)
    : QObject{parent},
    m_networkManager{networkManager},
    m_networkDeviceDiscovery{networkDeviceDiscovery}
{
    m_gracePeriodTimer.setSingleShot(true);
    m_gracePeriodTimer.setInterval(3000);
    connect(&m_gracePeriodTimer, &QTimer::timeout, this, [this](){
        finishDiscovery();
    });
}

void SenecDiscovery::startDiscovery()
{
    qCInfo(dcSenec()) << "Discovery: Searching SENEC energy storages in the local network...";
    m_startDateTime = QDateTime::currentDateTime();

    NetworkDeviceDiscoveryReply *discoveryReply = m_networkDeviceDiscovery->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::hostAddressDiscovered, this, &SenecDiscovery::checkNetworkDevice);
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        qCDebug(dcSenec()) << "Discovery: Network discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "network devices";
        m_networkDeviceInfos = discoveryReply->networkDeviceInfos();
        m_gracePeriodTimer.start();
        discoveryReply->deleteLater();
    });
}

QList<SenecDiscovery::Result> SenecDiscovery::results() const
{
    return m_results;
}

void SenecDiscovery::checkNetworkDevice(const QHostAddress &address)
{
    qCDebug(dcSenec()) << "Discovery: Verifying" << address;

    SenecStorageLan *storage = new SenecStorageLan(m_networkManager, address, this);
    m_storages.append(storage);

    connect(storage, &SenecStorageLan::initializeFinished, this, [this, storage, address](bool success){
        if (!success) {
            qCDebug(dcSenec()) << "Discovery: Failed to initialize host" << address.toString() << "Continue ...";
            cleanupStorage(storage);
            return;
        }

        // Successfully initialized
        Result result;
        result.deviceId = storage->deviceId();
        result.address = address;
        m_results.append(result);

        qCInfo(dcSenec()) << "Discovery: Found SENEC storage on" << address.toString() << storage->deviceId();
        cleanupStorage(storage);
    });

    storage->initialize();
}

void SenecDiscovery::cleanupStorage(SenecStorageLan *storage)
{
    m_storages.removeAll(storage);
    storage->deleteLater();
}

void SenecDiscovery::finishDiscovery()
{
    qint64 durationMilliSeconds = QDateTime::currentMSecsSinceEpoch() - m_startDateTime.toMSecsSinceEpoch();

    // Fill in all network device infos we have
    for (int i = 0; i < m_results.count(); i++)
        m_results[i].networkDeviceInfo = m_networkDeviceInfos.get(m_results.at(i).address);

    qCInfo(dcSenec()) << "Discovery: Finished the discovery process. Found" << m_results.count()
                       << "SENEC devices in" << QTime::fromMSecsSinceStartOfDay(durationMilliSeconds).toString("mm:ss.zzz");
    m_gracePeriodTimer.stop();

    emit discoveryFinished();
}
