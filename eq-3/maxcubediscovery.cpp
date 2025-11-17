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

#include "maxcubediscovery.h"
#include "extern-plugininfo.h"

MaxCubeDiscovery::MaxCubeDiscovery(QObject *parent) :
    QObject(parent)
{
    // UDP broadcast for cube detection in the network
    m_udpSocket = new QUdpSocket(this);
    m_port = 23272;
    m_udpSocket->bind(m_port,QUdpSocket::ShareAddress);

    m_timeout = new QTimer(this);
    m_timeout->setSingleShot(true);

    connect(m_udpSocket, &QUdpSocket::readyRead, this, &MaxCubeDiscovery::readData);
    connect(m_timeout, &QTimer::timeout, this, &MaxCubeDiscovery::discoverTimeout);
}

void MaxCubeDiscovery::detectCubes()
{
    m_cubeList.clear();

    // broadcast the hello message, every cube should respond with a 26 byte message
    m_udpSocket->writeDatagram("eQ3Max*.**********I", QHostAddress::Broadcast, m_port);
    m_timeout->start(1500);
}

void MaxCubeDiscovery::readData()
{
    QByteArray data;
    QHostAddress sender;
    quint16 udpPort;

    // read the answere from the
    while (m_udpSocket->hasPendingDatagrams()) {
        data.resize(m_udpSocket->pendingDatagramSize());
        m_udpSocket->readDatagram(data.data(), data.size(), &sender, &udpPort);
    }
    if(!data.isEmpty() && data.contains("eQ3MaxAp")){

        CubeInfo cube;
        cube.hostAddress = sender;
        cube.serialNumber = data.mid(8,10);
        cube.rfAddress = data.mid(21,3).toHex();
        cube.firmware = data.mid(24,2).toHex().toInt();

        // set port depending on the firmware
        if(cube.firmware < 109){
            cube.port= 80;
        }else{
            cube.port = 62910;
        }

        m_cubeList.append(cube);
    }
}

void MaxCubeDiscovery::discoverTimeout()
{
    emit cubesDetected(m_cubeList);
}
