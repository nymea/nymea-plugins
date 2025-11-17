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

#ifndef USBDEVICEWATCHER_H
#define USBDEVICEWATCHER_H

#include <QDebug>
#include <QObject>
#include <QSocketNotifier>

#include <libudev.h>

class RawHidDeviceWatcher : public QObject
{
    Q_OBJECT
public:
    explicit RawHidDeviceWatcher(QObject *parent = nullptr);
    ~RawHidDeviceWatcher();

    QStringList devicePaths() const;

private:
    struct udev *m_udev = nullptr;
    struct udev_monitor *m_monitor = nullptr;
    QSocketNotifier *m_notifier = nullptr;
    QStringList m_devicePaths;

signals:
    void deviceAdded(const QString &devicePath);
    void deviceRemoved(const QString &devicePath);

};

#endif // USBDEVICEWATCHER_H
