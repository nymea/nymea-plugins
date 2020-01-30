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

#ifndef DEVICEPLUGINUNIFI_H
#define DEVICEPLUGINUNIFI_H

#include <QObject>

#include "devices/deviceplugin.h"

#include <QNetworkRequest>

class PluginTimer;

class DevicePluginUnifi : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginunifi.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginUnifi(QObject *parent = nullptr);
    ~DevicePluginUnifi() override;

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void startPairing(DevicePairingInfo *info) override;
    void confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
//    void executeAction(DeviceActionInfo *info) override;

private:
    QNetworkRequest createRequest(const QString &address, const QString &path);
    QNetworkRequest createRequest(Device *device, const QString &path);

    void markOffline(Device *device);
private:
    QHash<DeviceDiscoveryInfo*, Devices> m_pendingDiscoveries;
    QHash<Device*, QStringList> m_pendingSiteDiscoveries;

    PluginTimer *m_loginTimer = nullptr;
    PluginTimer *m_pollTimer = nullptr;
};

#endif // DEVICEPLUGINUNIFI_H
