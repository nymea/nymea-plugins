/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by copyright law, and
* remains the property of nymea GmbH. All rights, including reproduction, publication,
* editing and translation, are reserved. The use of this project is subject to the terms of a
* license agreement to be concluded with nymea GmbH in accordance with the terms
* of use of nymea GmbH, available under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* This project may also contain libraries licensed under the open source software license GNU GPL v.3.
* Alternatively, this project may be redistributed and/or modified under the terms of the GNU
* Lesser General Public License as published by the Free Software Foundation; version 3.
* this project is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along with this project.
* If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under contact@nymea.io
* or see our FAQ/Licensing Information on https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DEVICEPLUGINLIFX_H
#define DEVICEPLUGINLIFX_H

#include "devices/deviceplugin.h"
#include "plugintimer.h"
#include "lifx.h"

#include <QTimer>

class DevicePluginLifx : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginlifx.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginLifx();

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;
    void deviceRemoved(Device *device) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<int, DeviceActionInfo *> m_asyncActions;
    Lifx *m_lifxConnection;

    QHash<DeviceClassId, StateTypeId> m_connectedStateTypeIds;
    QHash<DeviceClassId, StateTypeId> m_powerStateTypeIds;
    QHash<DeviceClassId, StateTypeId> m_brightnessStateTypeIds;

    QHash<DeviceId, DeviceActionInfo *> m_pendingBrightnessAction;

private slots:
    void onDeviceNameChanged();
    void onConnectionChanged(bool connected);
    void onRequestExecuted(int requestId, bool success);

    /*void onPowerNotificationReceived(bool status);
    void onBrightnessNotificationReceived(int percentage);
    void onColorTemperatureNotificationReceived(int kelvin);
    void onRgbNotificationReceived(QRgb rgbColor);
    void onNameNotificationReceived(const QString &name);*/
};

#endif // DEVICEPLUGINLIFX_H
