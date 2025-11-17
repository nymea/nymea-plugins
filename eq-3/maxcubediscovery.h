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

#ifndef MAXCUBEDISCOVERY_H
#define MAXCUBEDISCOVERY_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTimer>

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
    void cubesDetected(const QList<MaxCubeDiscovery::CubeInfo> &cubeList);

private:
    QUdpSocket *m_udpSocket = nullptr;
    QTimer *m_timeout = nullptr;

    quint16 m_port;

    QList<CubeInfo> m_cubeList;

};

#endif // MAXCUBEDISCOVERY_H
