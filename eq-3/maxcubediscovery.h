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

#ifndef MAXCUBEDISCOVERY_H
#define MAXCUBEDISCOVERY_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTimer>

#include "maxcube.h"

class MaxCubeDiscovery : public QObject
{
    Q_OBJECT
public:
    class CubeInfo {
    public:
        QString serialNumber;
        QHostAddress hostAddress;
        int port = 0;
        QByteArray rfAddress;
        int firmware;
    };

    explicit MaxCubeDiscovery(QObject *parent = nullptr);

    void detectCubes();

private slots:
    void readData();
    void discoverTimeout();

signals:
    void cubesDetected(const QList<CubeInfo> &cubeList);

private:
    QUdpSocket *m_udpSocket;
    QTimer *m_timeout;

    quint16 m_port;

    QList<CubeInfo> m_cubeList;

};

#endif // MAXCUBEDISCOVERY_H
