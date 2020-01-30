/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Simon Stürz <simon.stuerz@nymea.io>                 *
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
