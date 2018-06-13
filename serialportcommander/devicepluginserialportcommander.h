/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DEVICEPLUGINSERIALPORTCOMMANDER_H
#define DEVICEPLUGINSERIALPORTCOMMANDER_H

#include "plugin/deviceplugin.h"
#include "devicemanager.h"
#include "serialportcommander.h"
#include <QSerialPort>
#include <QSerialPortInfo>

class DevicePluginSerialPortCommander : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "guru.guh.DevicePlugin" FILE "devicepluginserialportcommander.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginSerialPortCommander();

    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    DeviceManager::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params);

    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;
    void init() override;

private:
    //QHash<Device *, QSerialPort *> m_outputSerialPorts;
    //QHash<Device *, QSerialPort *> m_inputSerialPorts;
    QHash<QString, SerialPortCommander *> m_serialPortCommanders;

private slots:
    void onCommandReceived(Device *device);

signals:

};

#endif // DEVICEPLUGINSERIALPORTCOMMANDER_H
