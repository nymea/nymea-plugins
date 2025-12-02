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

#ifndef SERIALPORTMONITOR_H
#define SERIALPORTMONITOR_H

#include <QHash>
#include <QObject>
#include <QSerialPortInfo>
#include <QSocketNotifier>

#include <libudev.h>

class SerialPortMonitor : public QObject
{
    Q_OBJECT
public:
    typedef struct SerialPortInfo {
        QString manufacturer;
        QString product;
        QString serialNumber;
        QString systemLocation;
        quint16 vendorId;
        quint16 productId;
    } SerialPortInfo;

    explicit SerialPortMonitor(QObject *parent = nullptr);
    ~SerialPortMonitor();

    QList<SerialPortInfo> serialPortInfos() const;

signals:
    void serialPortAdded(const SerialPortInfo &serialPortInfo);
    void serialPortRemoved(const SerialPortInfo &serialPortInfo);

private:
    struct udev *m_udev = nullptr;
    struct udev_monitor *m_monitor = nullptr;
    QSocketNotifier *m_notifier = nullptr;

    QHash<QString, SerialPortInfo> m_serialPortInfos;

};

QDebug operator<< (QDebug dbg, const SerialPortMonitor::SerialPortInfo &serialPortInfo);


#endif // SERIALPORTMONITOR_H
