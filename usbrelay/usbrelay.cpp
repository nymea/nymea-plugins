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

#include "usbrelay.h"
#include "extern-plugininfo.h"

UsbRelay::UsbRelay(const QString &path, int relayCount, QObject *parent) :
    QObject(parent),
    m_path(path),
    m_relayCount(relayCount)
{
    m_deviceWatcher = new RawHidDeviceWatcher(this);

    connect(m_deviceWatcher, &RawHidDeviceWatcher::deviceAdded, this, &UsbRelay::onDeviceAdded);
    connect(m_deviceWatcher, &RawHidDeviceWatcher::deviceRemoved, this, &UsbRelay::onDeviceRemoved);

    if (m_deviceWatcher->devicePaths().contains(m_path)) {
        setConnected(true);
    }

    // Create initial relay map
    for (int i = 0; i < m_relayCount; i++) {
        m_relayMap.insert(i + 1, false);
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

int UsbRelay::relayCount() const
{
    return m_relayCount;
}

bool UsbRelay::connected() const
{
    return m_connected;
}

bool UsbRelay::relayPower(int relayNumber) const
{
    return m_relayMap[relayNumber];
}

bool UsbRelay::setRelayPower(int relayNumber, bool power)
{
    if (!m_connected) {
        qCWarning(dcUsbRelay()) << "Could not set power of relay" << relayNumber << "because the device is not connected.";
        return false;
    }

    if (relayNumber > m_relayCount) {
        qCWarning(dcUsbRelay()) << "Could not set power of relay power because the relay number is invalid" << relayNumber << ">" << m_relayCount;
        return false;
    }

    return switchRelay(relayNumber, power);
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

void UsbRelay::setRelayPowerInternally(int relayNumber, bool power)
{
    if (m_relayMap[relayNumber] == power)
        return;

    qCDebug(dcUsbRelay()) << "Relay power changed" << relayNumber << power;
    m_relayMap[relayNumber] = power;
    emit relayPowerChanged(relayNumber, power);
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

    for (int i = 0; i < m_relayCount; i++) {
        int relayNumber = i + 1;
        bool power = static_cast<bool>(relayStatus & 1 << i);
        qCDebug(dcUsbRelay()) << "--> Relay" << relayNumber << power;
        setRelayPowerInternally(relayNumber, power);
    }
}

bool UsbRelay::switchRelay(int relayNumber, bool power)
{
    if (!m_hidDevice) {
        qCWarning(dcUsbRelay()) << "Cannot switch power for" << m_path << "because there is no HID device.";
        return false;
    }

    unsigned char buf[9]; // 1 extra byte for the report ID
    memset(buf, 0x00, sizeof (buf));
    buf[1] = power ? 0xff : 0xfd;
    buf[2] = static_cast<unsigned char>(relayNumber); // Relay number

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
