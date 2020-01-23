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

#include "broadcastping.h"
#include "extern-plugininfo.h"

#include <QNetworkInterface>
#include <QProcess>

BroadcastPing::BroadcastPing(QObject *parent) : QObject(parent)
{

}

void BroadcastPing::run()
{
    qDeleteAll(m_runningPings.keys());
    m_runningPings.clear();

    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        if (!interface.flags().testFlag(QNetworkInterface::IsUp) || !interface.flags().testFlag(QNetworkInterface::CanBroadcast) || interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            continue;
        }
        foreach (const QNetworkAddressEntry &addressEntry, interface.addressEntries()) {
            if (addressEntry.broadcast().isNull()) {
                continue;
            }
            qCDebug(dcNetworkDetector()) << "Sending Broadcast Ping on" << addressEntry.broadcast().toString() + '/' + QString::number(addressEntry.prefixLength()) + "...";
            QProcess *p = new QProcess(this);
            m_runningPings.insert(p, addressEntry);
            p->start("fping", {"-a", "-c", "1", "-g", addressEntry.broadcast().toString() + "/" + QString::number(addressEntry.prefixLength())});
            connect(p, SIGNAL(finished(int)), this, SLOT(broadcastPingFinished(int)));
        }
    }
    if (m_runningPings.isEmpty()) {
        qCWarning(dcNetworkDetector()) << "Cound not find any suitable interface for broadcast pinging";
        emit finished();
    }
}

void BroadcastPing::broadcastPingFinished(int exitCode)
{
    Q_UNUSED(exitCode);
    QProcess *p = static_cast<QProcess*>(sender());
    QNetworkAddressEntry addressEntry = m_runningPings.take(p);
    qCDebug(dcNetworkDetector()) << "Broadcast ping finished for network" << addressEntry.broadcast().toString() + '/' + QString::number(addressEntry.prefixLength());
//        qCDebug(dcNetworkDetector()) << p->readAllStandardError();
    p->deleteLater();

    if (m_runningPings.isEmpty()) {
        qCDebug(dcNetworkDetector()) << "All broadcast pings finished";
        emit finished();
    }
}
