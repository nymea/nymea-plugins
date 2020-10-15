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

#include "rawhiddevicewatcher.h"
#include "extern-plugininfo.h"

RawHidDeviceWatcher::RawHidDeviceWatcher(QObject *parent) : QObject(parent)
{
    m_udev = udev_new();
    if (!m_udev) {
        qCWarning(dcUsbRelay()) << "Could not initialize udev";
        return;
    }

    // Create udev monitor
    m_monitor = udev_monitor_new_from_netlink(m_udev, "udev");
    if (!m_monitor) {
        qCWarning(dcUsbRelay()) << "Could not initialize udev monitor.";
        udev_unref(m_udev);
        m_udev = nullptr;
        return;
    }

    // Set monitor filter to USB subsystem
    if (udev_monitor_filter_add_match_subsystem_devtype(m_monitor, "hidraw", nullptr) < 0) {
        qCWarning(dcUsbRelay()) << "Could not set seubsystem thing type filter to usb_device.";
        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;
        udev_unref(m_udev);
        m_udev = nullptr;
        return;
    }

    // Enable the monitor
    if (udev_monitor_enable_receiving(m_monitor) < 0) {
        qCWarning(dcUsbRelay()) << "Could not enable udev monitor.";
        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;
        udev_unref(m_udev);
        m_udev = nullptr;
        return;
    }

    // Read initially all devices
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    enumerate = udev_enumerate_new(m_udev);
    if (!enumerate) {
        qCWarning(dcUsbRelay()) << "Could not create udev enumerate for initial thing reading.";
        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;
        udev_unref(m_udev);
        m_udev = nullptr;
        return;
    }


    udev_enumerate_add_match_subsystem(enumerate, "hidraw");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices) {
        struct udev_device *thing;
        const char *path;
        path = udev_list_entry_get_name(dev_list_entry);
        thing = udev_device_new_from_syspath(m_udev, path);

        QString devicePath = QString::fromLatin1(udev_device_get_property_value(thing,"DEVNAME"));
        udev_device_unref(thing);

        qCDebug(dcUsbRelay()) << "[+]" << devicePath;
        m_devicePaths.append(devicePath);
        emit deviceAdded(devicePath);
    }

    udev_enumerate_unref(enumerate);

    // Create socket notifier for read
    m_notifier = new QSocketNotifier(udev_monitor_get_fd(m_monitor), QSocketNotifier::Read, this );
    connect(m_notifier, &QSocketNotifier::activated, this, [this](int socket){

        Q_UNUSED(socket)

        // Create udev thing
        udev_device *thing = udev_monitor_receive_device(m_monitor);
        if (!thing) {
            qCWarning(dcUsbRelay()) << "Got socket sotification but could not read thing information.";
            return;
        }

        QString actionString = QString::fromLatin1(udev_device_get_action(thing));
        QString devicePath = QString::fromLatin1(udev_device_get_property_value(thing,"DEVNAME"));

        // Clean udev thing
        udev_device_unref(thing);

        if (actionString.isEmpty())
            return;

        if (actionString == "add") {
            qCDebug(dcUsbRelay()) << "[+]" << devicePath;
            if (!m_devicePaths.contains(devicePath)) {
                m_devicePaths.append(devicePath);
                emit deviceAdded(devicePath);
            }
        }

        if (actionString == "remove") {
            qCDebug(dcUsbRelay()) << "[-]" << devicePath;
            if (m_devicePaths.contains(devicePath)) {
                m_devicePaths.removeAll(devicePath);
                emit deviceRemoved(devicePath);
            }
        }

    });

    m_notifier->setEnabled(true);
    qCDebug(dcUsbRelay()) << "Usb thing watcher initialized successfully.";
}

RawHidDeviceWatcher::~RawHidDeviceWatcher()
{
    if (m_monitor)
        udev_monitor_unref(m_monitor);

    if (m_udev)
        udev_unref(m_udev);
}

QStringList RawHidDeviceWatcher::devicePaths() const
{
    return m_devicePaths;
}

