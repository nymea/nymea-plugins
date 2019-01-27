/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#ifndef DEVICEPLUGINMODBUSCOMMANDER_H
#define DEVICEPLUGINMODBUSCOMMANDER_H

#include "plugin/deviceplugin.h"
#include "devicemanager.h"
#include "plugintimer.h"
#include "modbustcpmaster.h"
#include "modbusrtumaster.h"
#include <QSerialPortInfo>

class DevicePluginModbusCommander : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginmodbuscommander.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginModbusCommander();
    ~DevicePluginModbusCommander();
    void init() override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    DeviceManager::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    void postSetupDevice(Device *device) override;
    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;

private:

    void readData(Device *device);
    void writeData(Device *device, Action action);
    PluginTimer *m_refreshTimer = nullptr;
    QList<QString> m_usedSerialPorts;
    QHash<Device *, ModbusRTUMaster *> m_modbusRTUMasters;
    QHash<Device *, ModbusTCPMaster *> m_modbusTCPMasters;


private slots:
    void onRefreshTimer();

signals:

};

#endif // DEVICEPLUGINMODBUSCOMMANDER_H
