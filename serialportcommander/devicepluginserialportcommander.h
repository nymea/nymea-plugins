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

#ifndef DEVICEPLUGINSERIALPORTCOMMANDER_H
#define DEVICEPLUGINSERIALPORTCOMMANDER_H

#include "plugin/deviceplugin.h"
#include "devicemanager.h"
#include <QSerialPort>
#include <QSerialPortInfo>

class DevicePluginSerialPortCommander : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginserialportcommander.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginSerialPortCommander();

    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    DeviceManager::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params);

    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;
    void init() override;

private:
    QHash<QString, QSerialPort *> m_serialPorts;

private slots:
    void onCommandReceived(Device *device);

signals:

};

#endif // DEVICEPLUGINSERIALPORTCOMMANDER_H
