/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Michael Zanetti <michael.zanetti@nymea.io>          *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DEVICEPLUGINDOORBIRD_H
#define DEVICEPLUGINDOORBIRD_H

#include <QImage>

#include "devices/deviceplugin.h"
#include "devices/devicemanager.h"
#include "doorbird.h"

class QNetworkAccessManager;
class QNetworkReply;

class DevicePluginDoorbird: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugindoorbird.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginDoorbird();

    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

    void startPairing(DevicePairingInfo *info) override;
    void confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret) override;

    void deviceRemoved(Device *device)override;

private:
    QHash<Device*, Doorbird *> m_doorbirdConnections;
    QHash<QUuid, DeviceActionInfo *> m_asyncActions;

private slots:
    void onDoorBirdConnected(bool status);
    void onDoorBirdEvent(Doorbird::EventType eventType, bool status);
    void onDoorBirdRequestSent(QUuid requestId, bool success);
    void onImageReceived(QImage image);
};

#endif // DEVICEPLUGINDOORBIRD_H
