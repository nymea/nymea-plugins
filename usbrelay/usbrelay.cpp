/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@nymea.io>                 *
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

#include "usbrelay.h"
#include "extern-plugininfo.h"

UsbRelay::UsbRelay(const QString &path, QObject *parent) :
    QObject(parent),
    m_path(path)
{
    m_deviceWatcher = new RawHidDeviceWatcher(this);

    connect(m_deviceWatcher, &RawHidDeviceWatcher::deviceAdded, this, &UsbRelay::onDeviceAdded);
    connect(m_deviceWatcher, &RawHidDeviceWatcher::deviceRemoved, this, &UsbRelay::onDeviceRemoved);

    if (m_deviceWatcher->devicePaths().contains(m_path)) {
        setConnected(true);
    }
}

UsbRelay::~UsbRelay()
{
    if (m_hidDevice)
        hid_close(m_hidDevice);

    hid_exit();
}

QString UsbRelay::path() const
{
    return m_path;
}

bool UsbRelay::connected() const
{
    return m_connected;
}

bool UsbRelay::relayOnePower() const
{
    return m_relayOnePower;
}

bool UsbRelay::setRelayOnePower(bool power)
{
    qCDebug(dcUsbRelay()) << m_path << "set power relay 1" << power;

    return switchRelay(1, power);
}

bool UsbRelay::relayTwoPower() const
{
    return m_relayTwoPower;
}

bool UsbRelay::setRelayTwoPower(bool power)
{
    qCDebug(dcUsbRelay()) << m_path << "set power relay 2" << power;
    return switchRelay(2, power);
}

void UsbRelay::setConnected(bool connected)
{
    if (m_connected == connected)
        return;

    qCDebug(dcUsbRelay()) << m_path << (connected ? "connected" : "disconnected");
    if (connected) {
        // Initialize the device
        m_hidDevice = hid_open_path(m_path.toLatin1().data());
        if (!m_hidDevice) {
            qCWarning(dcUsbRelay()) << "Could nor open HID device for" << m_path;
            m_connected = false;
            emit connectedChanged(m_connected);
        }

        readStatus();

    } else {
        // Deinitialize
        if (m_hidDevice) {
            hid_close(m_hidDevice);
            m_hidDevice = nullptr;
            hid_exit();
        }
    }

    m_connected = connected;
    emit connectedChanged(m_connected);
}

void UsbRelay::setRelayOnePowerInternal(bool power)
{
    if (m_relayOnePower == power)
        return;

    m_relayOnePower = power;
    emit relayOneChanged(m_relayOnePower);
}

void UsbRelay::setRelayTwoPowerInternal(bool power)
{
    if (m_relayTwoPower == power)
        return;

    m_relayTwoPower = power;
    emit relayTwoChanged(m_relayTwoPower);
}

void UsbRelay::readStatus()
{
    qCDebug(dcUsbRelay()) << "Read relay status of" << m_path;
    unsigned char buf[9];
    buf[0] = 0x01;
    int ret = hid_get_feature_report(m_hidDevice, buf, sizeof(buf));
    if (ret < 0) {
        qCWarning(dcUsbRelay()) << "Could not create HID device for reading" << m_path;
        hid_close(m_hidDevice);
        m_hidDevice = nullptr;

        m_connected = false;
        emit connectedChanged(m_connected);
        return;
    }

    quint8 relayStatus = static_cast<quint8>(buf[7]);
    setRelayOnePowerInternal(static_cast<bool>(relayStatus &1 << 0));
    setRelayTwoPowerInternal(static_cast<bool>(relayStatus &1 << 1));

    qCDebug(dcUsbRelay()) << m_path << "Relay 1:" << m_relayOnePower << "| Relay 2:" << m_relayTwoPower;
}

bool UsbRelay::switchRelay(int relayNumber, bool power)
{
    if (!m_hidDevice) {
        qCWarning(dcUsbRelay()) << "Cannot switch power for" << m_path << "because there is no HID device.";
        return false;
    }

    unsigned char buf[9]; // 1 extra byte for the report ID
    buf[0] = 0x0; // report number
    buf[1] = power ? 0xff : 0xfd;
    buf[2] = static_cast<unsigned char>(relayNumber); // Relay number
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x00;
    buf[8] = 0x00;

    qCDebug(dcUsbRelay()) << buf[0] << buf[1] << buf[2];

    if (hid_write(m_hidDevice, buf, sizeof(buf)) <= 0) {
        qCWarning(dcUsbRelay()) << "Cannot switch power for" << m_path << "because could not write to HID device.";
        return false;
    }

    readStatus();
    return true;
}

void UsbRelay::onDeviceAdded(const QString &devicePath)
{
    if (devicePath == m_path) {
        setConnected(true);
    }
}

void UsbRelay::onDeviceRemoved(const QString &devicePath)
{
    if (devicePath == m_path) {
        setConnected(false);
    }
}
