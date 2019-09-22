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

#ifndef DEVICEPLUGINTUYA_H
#define DEVICEPLUGINTUYA_H

#include "devices/deviceplugin.h"

class PluginTimer;

class DevicePluginTuya: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugintuya.json")
    Q_INTERFACES(DevicePlugin)


public:
    explicit DevicePluginTuya(QObject *parent = nullptr);
    ~DevicePluginTuya() override;

    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    void startPairing(DevicePairingInfo *info) override;
    void confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret) override;
    void executeAction(DeviceActionInfo *info) override;

signals:
    void tokenRefreshed(Device *device, bool success);

private:
    void refreshAccessToken(Device *device);
    void updateChildDevices(Device *device);

    void controlTuyaSwitch(const QString &command, const QString &value, DeviceActionInfo *info);

    QHash<DeviceId, QTimer*> m_tokenExpiryTimers;
    PluginTimer *m_pluginTimer = nullptr;
};

#endif // DEVICEPLUGINTUYA_H
