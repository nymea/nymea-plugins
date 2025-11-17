// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "discovery.h"

#include <QUdpSocket>
#include <QTimer>
#include <QMetaObject>

#include "extern-plugininfo.h"

Discovery::Discovery(QObject *parent) : QObject(parent)
{

}

void Discovery::discover()
{
    QUdpSocket *searchSocket = new QUdpSocket(this);

    // Note: This will fail, and it's not a problem, but it is required to force the socket to stick to IPv4...
    searchSocket->bind(QHostAddress::AnyIPv4, 30303);

    QString discoveryString = "Durchsuchen: Wer ist da?";
    qint64 len = searchSocket->writeDatagram(discoveryString.toUtf8(), QHostAddress("255.255.255.255"), 30303);
    if (len != discoveryString.length()) {
        searchSocket->deleteLater();
        qCWarning(dcAnelElektronik()) << "Error sending discovery";
        QTimer::singleShot(0, this, [=](){
            emit finished(true);
        });
        return;
    }

    QTimer::singleShot(2000, this, [this, searchSocket](){
        while(searchSocket->hasPendingDatagrams()) {
            char buffer[1024];
            QHostAddress senderAddress;
            int len = searchSocket->readDatagram(buffer, 1024, &senderAddress);
            QByteArray data = QByteArray::fromRawData(buffer, len);
            qCDebug(dcAnelElektronik()) << "Have datagram:" << data;
            if (!data.startsWith("NET-CONTROL")) {
                qCDebug(dcAnelElektronik()) << "Failed to parse discovery datagram from" << senderAddress << data;
                continue;
            }
            QStringList parts = QString(data).split("\r\n");
            if (parts.count() != 4) {
                qCDebug(dcAnelElektronik()) << "Failed to parse discovery datagram from" << senderAddress << data;
                continue;
            }
            qCDebug(dcAnelElektronik()) << "Found NET-CONTROL:" << senderAddress << parts.at(0) << parts.at(1) << parts.at(2) << parts.at(3) << senderAddress.protocol();
            Result result;
            result.name = parts.at(2);
            result.macAddress = parts.at(1);
            result.ipAddress = senderAddress.toString();
            result.port = parts.at(3).toInt();

            m_results.insert(result.macAddress, result);
        }
        emit finished(false);
        searchSocket->deleteLater();
    });
}

QHash<QString, Discovery::Result> Discovery::results() const
{
    return m_results;
}
