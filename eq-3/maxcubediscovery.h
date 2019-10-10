/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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
