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

    connect(m_udpSocket,SIGNAL(readyRead()),this,SLOT(readData()));
    connect(m_timeout,SIGNAL(timeout()),this,SLOT(discoverTimeout()));
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
