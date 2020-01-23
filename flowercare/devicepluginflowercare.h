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

#ifndef DEVICEPLUGINFLOWERCARE_H
#define DEVICEPLUGINFLOWERCARE_H


#include <QPointer>
#include <QHash>
#include "devices/deviceplugin.h"
#include "plugintimer.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"

class FlowerCare;

class DevicePluginFlowercare : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginflowercare.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginFlowercare();
    ~DevicePluginFlowercare();

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

private:
    PluginTimer *m_reconnectTimer = nullptr;
    QHash<Device*, FlowerCare*> m_list;
    QHash<FlowerCare*, int> m_refreshMinutes;

    bool verifyExistingDevices(const QBluetoothDeviceInfo &deviceInfo);

private slots:
    void onPluginTimer();
    void onSensorDataReceived(quint8 batteryLevel, double degreeCelsius, double lux, double moisture, double fertility);

};

#endif // DEVICEPLUGINFLOWERCARE_H
