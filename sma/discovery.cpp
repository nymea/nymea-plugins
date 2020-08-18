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

#include "discovery.h"
#include "extern-plugininfo.h"

#include <QDebug>
#include <QXmlStreamReader>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QTimer>

Discovery::Discovery(QObject *parent) : QObject(parent)
{
    connect(&m_timeoutTimer, &QTimer::timeout, this, &Discovery::onTimeout);
}

void Discovery::discoverHosts(int timeout)
{
    if (isRunning()) {
        qWarning(dcSma()) << "Discovery already running. Cannot start twice.";
        return;
    }
    m_timeoutTimer.start(timeout * 1000);

    foreach (const QString &target, getDefaultTargets()) {
        QProcess *discoveryProcess = new QProcess(this);
        m_discoveryProcesses.append(discoveryProcess);
        connect(discoveryProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(discoveryFinished(int,QProcess::ExitStatus)));

        QStringList arguments;
        arguments << "-oX" << "-" << "-n" << "-sn";
        arguments << target;

        qCDebug(dcSma()) << "Scanning network:" << "nmap" << arguments.join(" ");
        discoveryProcess->start(QStringLiteral("nmap"), arguments);
    }

}

void Discovery::abort()
{
    foreach (QProcess *discoveryProcess, m_discoveryProcesses) {
        if (discoveryProcess->state() == QProcess::Running) {
            qCDebug(dcSma()) << "Kill running discovery process";
            discoveryProcess->terminate();
            discoveryProcess->waitForFinished(5000);
        }
    }
    foreach (QProcess *p, m_pendingArpLookups.keys()) {
        p->terminate();
        delete p;
    }
    m_pendingArpLookups.clear();
    m_pendingNameLookups.clear();
    qDeleteAll(m_scanResults);
    m_scanResults.clear();
}

bool Discovery::isRunning() const
{
    return !m_discoveryProcesses.isEmpty() || !m_pendingArpLookups.isEmpty() || !m_pendingNameLookups.isEmpty();
}

void Discovery::discoveryFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess *discoveryProcess = static_cast<QProcess*>(sender());

    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        qCWarning(dcSma()) << "Nmap error failed. Is nmap installed correctly?";
        m_discoveryProcesses.removeAll(discoveryProcess);
        discoveryProcess->deleteLater();
        discoveryProcess = nullptr;
        finishDiscovery();
        return;
    }

    QByteArray data = discoveryProcess->readAll();
    m_discoveryProcesses.removeAll(discoveryProcess);
    discoveryProcess->deleteLater();
    discoveryProcess = nullptr;

    QXmlStreamReader reader(data);

    int foundHosts = 0;

    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if(token == QXmlStreamReader::StartDocument)
            continue;

        if(token == QXmlStreamReader::StartElement && reader.name() == "host") {
            bool isUp = false;
            QString address;
            QString macAddress;
            QString vendor;
            while (!reader.atEnd() && !reader.hasError() && !(token == QXmlStreamReader::EndElement && reader.name() == "host")) {
                token = reader.readNext();

                if (reader.name() == "address") {
                    QString addr = reader.attributes().value("addr").toString();
                    QString type = reader.attributes().value("addrtype").toString();
                    if (type == "ipv4" && !addr.isEmpty()) {
                        address = addr;
                    } else if (type == "mac") {
                        macAddress = addr;
                        vendor = reader.attributes().value("vendor").toString();
                    }
                }

                if (reader.name() == "status") {
                    QString state = reader.attributes().value("state").toString();
                    if (!state.isEmpty())
                        isUp = state == "up";
                }
            }

            if (isUp) {
                foundHosts++;
                qCDebug(dcSma()) << "Have host:" << address;

                Host *host = new Host();
                host->setAddress(address);

                if (!macAddress.isEmpty()) {
                    host->setMacAddress(macAddress);
                } else {
                    QProcess *arpLookup = new QProcess(this);
                    m_pendingArpLookups.insert(arpLookup, host);
                    connect(arpLookup, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(arpLookupDone(int,QProcess::ExitStatus)));
                    arpLookup->start("arp", {"-vn"});
                }

                host->setHostName(vendor);
                QHostInfo::lookupHost(address, this, SLOT(hostLookupDone(QHostInfo)));
                m_pendingNameLookups.insert(address, host);

                m_scanResults.append(host);
            }
        }
    }

    if (foundHosts == 0 && m_discoveryProcesses.isEmpty()) {
        qCDebug(dcSma()) << "Network scan successful but no hosts found in this network";
        finishDiscovery();
    }
}

void Discovery::hostLookupDone(const QHostInfo &info)
{
    Host *host = m_pendingNameLookups.take(info.addresses().first().toString());
    if (!host) {
        // Probably aborted...
        return;
    }
    if (info.error() != QHostInfo::NoError) {
        qWarning(dcSma()) << "Host lookup failed:" << info.errorString();
    }
    if (host->hostName().isEmpty() || info.hostName() != host->address()) {
        host->setHostName(info.hostName());
    }

    finishDiscovery();
}

void Discovery::arpLookupDone(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess *p = static_cast<QProcess*>(sender());
    p->deleteLater();

    Host *host = m_pendingArpLookups.take(p);

    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        qCWarning(dcSma()) << "ARP lookup process failed for host" << host->address();
        finishDiscovery();
        return;
    }

    QString data = QString::fromLatin1(p->readAll());
    foreach (QString line, data.split('\n')) {
        line.replace(QRegExp("[ ]{1,}"), " ");
        QStringList parts = line.split(" ");
        if (parts.count() >= 3 && parts.first() == host->address() && parts.at(1) == "ether") {
            host->setMacAddress(parts.at(2));
            break;
        }
    }
    finishDiscovery();
}

void Discovery::onTimeout()
{
    qWarning(dcSma()) << "Timeout hit. Stopping discovery";
    while (!m_discoveryProcesses.isEmpty()) {
        QProcess *discoveryProcess = m_discoveryProcesses.takeFirst();
        disconnect(this, SLOT(discoveryFinished(int,QProcess::ExitStatus)));
        discoveryProcess->terminate();
        delete discoveryProcess;
    }
    foreach (QProcess *p, m_pendingArpLookups.keys()) {
        p->terminate();
        m_scanResults.removeAll(m_pendingArpLookups.value(p));
        delete p;
    }
    m_pendingArpLookups.clear();
    m_pendingNameLookups.clear();
    finishDiscovery();
}

QStringList Discovery::getDefaultTargets()
{
    QStringList targets;
    foreach (const QHostAddress &interface, QNetworkInterface::allAddresses()) {
        if (!interface.isLoopback() && interface.scopeId().isEmpty() && interface.protocol() == QAbstractSocket::IPv4Protocol) {
            QPair<QHostAddress, int> pair = QHostAddress::parseSubnet(interface.toString() + "/24");
            QString newTarget = QString("%1/%2").arg(pair.first.toString()).arg(pair.second);
            if (!targets.contains(newTarget)) {
                targets.append(newTarget);
            }
        }
    }
    return targets;
}

void Discovery::finishDiscovery()
{
    if (m_discoveryProcesses.count() > 0 || m_pendingNameLookups.count() > 0 || m_pendingArpLookups.count() > 0) {
        // Still busy...
        return;
    }

    QList<Host> hosts;
    foreach (Host *host, m_scanResults) {
        if (!host->macAddress().isEmpty()) {
            hosts.append(*host);
        }
    }
    qDeleteAll(m_scanResults);
    m_scanResults.clear();

    qCDebug(dcSma()) << "Emitting thing discovered for" << hosts.count() << "devices";
    m_timeoutTimer.stop();
    emit finished(hosts);
}
