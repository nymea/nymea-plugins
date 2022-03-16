/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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

#include "serialportmonitor.h"
#include "extern-plugininfo.h"

#include <QSerialPortInfo>

SerialPortMonitor::SerialPortMonitor(QObject *parent) : QObject(parent)
{
    m_udev = udev_new();
    if (!m_udev) {
        qCWarning(dcUsbRly82()) << "SerialPortMonitor: Could not initialize udev";
        return;
    }

    // Read initially all tty devices
    struct udev_enumerate *enumerate = udev_enumerate_new(m_udev);
    if (!enumerate) {
        qCWarning(dcUsbRly82()) << "SerialPortMonitor: Could not create udev enumerate for initial device reading.";
        udev_unref(m_udev);
        m_udev = nullptr;
        return;
    }

    // We are only interested in FTDI devices
    udev_enumerate_add_match_subsystem(enumerate, "tty");
//    udev_enumerate_add_match_property(enumerate, "ID_VENDOR_ID", "04d8");
//    udev_enumerate_add_match_property(enumerate, "ID_MODEL_ID", "ffee");

    if (udev_enumerate_scan_devices(enumerate) < 0) {
        qCWarning(dcUsbRly82()) << "SerialPortMonitor: Failed to scan devices from udev enumerate.";
        udev_enumerate_unref(enumerate);
        enumerate = nullptr;
        udev_unref(m_udev);
        m_udev = nullptr;
        return;
    }

    qCDebug(dcUsbRly82()) << "SerialPortMonitor: Load initial list of available serial ports...";
    struct udev_list_entry *devices = nullptr;
    devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *dev_list_entry = nullptr;
    udev_list_entry_foreach(dev_list_entry, devices) {
        struct udev_device *device = nullptr;
        const char *path;
        path = udev_list_entry_get_name(dev_list_entry);
        device = udev_device_new_from_syspath(m_udev, path);

        // Print properties
        struct udev_list_entry *properties = udev_device_get_properties_list_entry(device);
        struct udev_list_entry *property_list_entry = nullptr;
        udev_list_entry_foreach(property_list_entry, properties) {
            qCDebug(dcUsbRly82()) << "SerialPortMonitor:  - Property" << udev_list_entry_get_name(property_list_entry) << udev_list_entry_get_value(property_list_entry);
        }

        QString vendorIdString = QString::fromLatin1(udev_device_get_property_value(device, "ID_VENDOR_ID"));
        QString productIdString = QString::fromLatin1(udev_device_get_property_value(device, "ID_MODEL_ID"));

        SerialPortInfo info;
        info.systemLocation = QString::fromLatin1(udev_device_get_property_value(device,"DEVNAME"));
        info.manufacturer = QString::fromLatin1(udev_device_get_property_value(device,"ID_VENDOR_ENC"));
        info.product = QString::fromLatin1(udev_device_get_property_value(device,"ID_MODEL_ENC"));
        info.serialNumber = QString::fromLatin1(udev_device_get_property_value(device, "ID_SERIAL_SHORT"));
        info.vendorId = static_cast<quint16>(vendorIdString.toUInt(nullptr, 16));
        info.productId = static_cast<quint16>(productIdString.toUInt(nullptr, 16));

        // Clean up this device since we have all information
        udev_device_unref(device);

        qCDebug(dcUsbRly82()) << "SerialPortMonitor: [+]" << info;
        m_serialPortInfos.insert(info.systemLocation, info);
        emit serialPortAdded(info);
    }

    udev_enumerate_unref(enumerate);
    enumerate = nullptr;

    // Create udev monitor
    m_monitor = udev_monitor_new_from_netlink(m_udev, "udev");
    if (!m_monitor) {
        qCWarning(dcUsbRly82()) << "SerialPortMonitor: Could not initialize udev monitor.";
        udev_unref(m_udev);
        m_udev = nullptr;
        return;
    }

    // Set monitor filter to tty subsystem
    if (udev_monitor_filter_add_match_subsystem_devtype(m_monitor, "tty", nullptr) < 0) {
        qCWarning(dcUsbRly82()) << "SerialPortMonitor: Could not set subsystem device type filter to tty.";
        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;
        udev_unref(m_udev);
        m_udev = nullptr;
        return;
    }

    // Enable the monitor
    if (udev_monitor_enable_receiving(m_monitor) < 0) {
        qCWarning(dcUsbRly82()) << "SerialPortMonitor: Could not enable udev monitor.";
        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;
        udev_unref(m_udev);
        m_udev = nullptr;
        return;
    }

    // Create socket notifier for read
    int socketDescriptor = udev_monitor_get_fd(m_monitor);
    m_notifier = new QSocketNotifier(socketDescriptor, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, [this, socketDescriptor](int socket){

        if (socketDescriptor != socket) {
            qCWarning(dcUsbRly82()) << "SerialPortMonitor: socket != socketdescriptor";
            return;
        }

        // Create udev device
        udev_device *device = udev_monitor_receive_device(m_monitor);
        if (!device) {
            qCWarning(dcUsbRly82()) << "SerialPortMonitor: Got socket sotification but could not read device information.";
            return;
        }

        QString actionString = QString::fromUtf8(udev_device_get_action(device));

        QString vendorIdString = QString::fromUtf8(udev_device_get_property_value(device, "ID_VENDOR_ID"));
        QString productIdString = QString::fromUtf8(udev_device_get_property_value(device, "ID_MODEL_ID"));

        SerialPortInfo info;
        info.systemLocation = QString::fromUtf8(udev_device_get_property_value(device,"DEVNAME"));
        info.manufacturer = QString::fromUtf8(udev_device_get_property_value(device,"ID_VENDOR_ENC"));
        info.product = QString::fromUtf8(udev_device_get_property_value(device,"ID_MODEL_ENC"));
        info.serialNumber = QString::fromUtf8(udev_device_get_property_value(device, "ID_SERIAL_SHORT"));
        info.vendorId = static_cast<quint16>(vendorIdString.toUInt(nullptr, 16));
        info.productId = static_cast<quint16>(productIdString.toUInt(nullptr, 16));

        // Clean udev device
        udev_device_unref(device);

        // Make sure we know the action
        if (actionString.isEmpty())
            return;

        if (actionString == "add") {
            qCDebug(dcUsbRly82()) << "SerialPortMonitor: [+]" << info;
            if (!m_serialPortInfos.contains(info.systemLocation)) {
                m_serialPortInfos.insert(info.systemLocation, info);
                emit serialPortAdded(info);
            }
        }

        if (actionString == "remove") {
            qCDebug(dcUsbRly82()) << "SerialPortMonitor: [-]" << info;

            if (m_serialPortInfos.contains(info.systemLocation)) {
                m_serialPortInfos.remove(info.systemLocation);
                emit serialPortRemoved(info);
            }
        }
    });

    m_notifier->setEnabled(true);
}

SerialPortMonitor::~SerialPortMonitor()
{
    if (m_notifier)
        delete m_notifier;

    if (m_monitor)
        udev_monitor_unref(m_monitor);

    if (m_udev)
        udev_unref(m_udev);

}

QList<SerialPortMonitor::SerialPortInfo> SerialPortMonitor::serialPortInfos() const
{
    return m_serialPortInfos.values();
}

QDebug operator<<(QDebug dbg, const SerialPortMonitor::SerialPortInfo &serialPortInfo)
{
    dbg.nospace().noquote() << "SerialPort(" << QString("%1:%2")
                               .arg(serialPortInfo.vendorId, 4, 16, QLatin1Char('0'))
                               .arg(serialPortInfo.productId, 4, 16, QLatin1Char('0'))
                            << ", " << serialPortInfo.systemLocation
                            << ", " << serialPortInfo.manufacturer
                            << ", " << serialPortInfo.product << ") ";
    return dbg.maybeSpace().maybeQuote();
}
