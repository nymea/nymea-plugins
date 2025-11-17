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

#ifndef USBRELAY_H
#define USBRELAY_H

#include <QObject>
#include <hidapi/hidapi.h>

#include "rawhiddevicewatcher.h"

class UsbRelay : public QObject
{
    Q_OBJECT
public:
    explicit UsbRelay(const QString &path, int relayCount, QObject *parent = nullptr);
    ~UsbRelay();

    QString path() const;
    int relayCount() const;

    bool connected() const;

    bool relayPower(int relayNumber) const;
    bool setRelayPower(int relayNumber, bool power);

private:
    RawHidDeviceWatcher *m_deviceWatcher = nullptr;
    hid_device *m_hidDevice = nullptr;

    QString m_path;
    int m_relayCount = 0;
    bool m_connected = false;
    bool m_relayOnePower = false;
    bool m_relayTwoPower = false;

    void setConnected(bool connected);

    // Relay map <relay number>, bool value
    QHash<int, bool> m_relayMap;

    void setRelayPowerInternally(int relayNumber, bool power);

    void readStatus();

    bool switchRelay(int relayNumber, bool power);

signals:
    void connectedChanged(bool connected);
    void relayPowerChanged(int relayNumber, bool power);

private slots:
    void onDeviceAdded(const QString &devicePath);
    void onDeviceRemoved(const QString &devicePath);

};

#endif // USBRELAY_H
