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
