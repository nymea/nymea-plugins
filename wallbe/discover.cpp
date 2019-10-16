/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stürz <simon.stuerz@nymea.io>                 *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <QHostAddress>
#include <QNetworkInterface>
#include "discover.h"
#include "extern-plugininfo.h"

Discover::Discover(QStringList arguments, QObject *parent) :
    QObject(parent),
    m_arguments(arguments),
    m_discoveryProcess(nullptr),
    m_aboutToQuit(false)
{
}

Discover::~Discover()
{
    // Stop running processes
    m_aboutToQuit = true;

    if (m_discoveryProcess && m_discoveryProcess->state() == QProcess::Running) {
        qCDebug(dcWallbe()) << "Kill running discovery process";
        m_discoveryProcess->terminate();
        m_discoveryProcess->waitForFinished(5000);
    }
}

QStringList Discover::getDefaultTargets()
{
    QStringList targets;
    foreach (const QHostAddress &interface, QNetworkInterface::allAddresses()) {
        if (!interface.isLoopback() && interface.scopeId().isEmpty()) {
            QPair<QHostAddress, int> pair = QHostAddress::parseSubnet(interface.toString() + "/24");
            targets << QString("%1/%2").arg(pair.first.toString()).arg(pair.second);
        }
    }
    return targets;
}

QList<Host> Discover::parseProcessOutput(const QByteArray &processData)
{
    m_reader.clear();
    m_reader.addData(processData);

    QList<Host> hosts;

    while (!m_reader.atEnd() && !m_reader.hasError()) {

        QXmlStreamReader::TokenType token = m_reader.readNext();
        if(token == QXmlStreamReader::StartDocument)
            continue;

        if(token == QXmlStreamReader::StartElement && m_reader.name() == "host") {
            Host host = parseHost();
            if (host.isValid()) {
                hosts.append(host);
            }
        }
    }
    return hosts;
}

Host Discover::parseHost()
{
    if (!m_reader.isStartElement() || m_reader.name() != "host")
        return Host();

    QString address; QString hostName; QString status; QString mac;
    while(!(m_reader.tokenType() == QXmlStreamReader::EndElement && m_reader.name() == "host")){

        m_reader.readNext();

        if (m_reader.isStartElement() && m_reader.name() == "hostname") {
            QString name = m_reader.attributes().value("name").toString();
            if (!name.isEmpty())
                hostName = name;

            m_reader.readNext();
        }

        if (m_reader.name() == "address") {
            QString addr = m_reader.attributes().value("addr").toString();
            if (!addr.isEmpty())
                address = addr;
        }

        if (m_reader.name() == "state") {
            QString state = m_reader.attributes().value("state").toString();
            if (!state.isEmpty())
                status = state;
        }

        if (m_reader.name() == "mac") { //TODO CHeck Keyword
            QString macAddress = m_reader.attributes().value("state").toString();
            if (!macAddress.isEmpty())
                mac = macAddress;
        }
    }
    return Host(hostName, address, mac, (status == "open" ? true : false));
}

void Discover::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess *process = static_cast<QProcess*>(sender());

    // If the process was killed because nymead is shutting down...we dont't care any more about the result
    if (m_aboutToQuit)
        return;

    // Discovery
    if (process == m_discoveryProcess) {

        qCDebug(dcWallbe()) << "Discovery process finished";

        process->deleteLater();
        m_discoveryProcess = nullptr;

        if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
            qCWarning(dcWallbe()) << "Network scan error:" << process->readAllStandardError();
            return;
        }

        QByteArray outputData = process->readAllStandardOutput();
        emit devicesDiscovered(parseProcessOutput(outputData));

    }
}

bool Discover::getProcessStatus()
{
    return m_discoveryProcess;
}

void Discover::onTimeout()
{

}
