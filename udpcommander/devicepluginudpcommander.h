/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015-2018 Simon Stürz <simon.stuerz@guh.io>              *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
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

#ifndef DEVICEPLUGINUDPCOMMANDER_H
#define DEVICEPLUGINUDPCOMMANDER_H

#include "devices/deviceplugin.h"

#include <QHash>
#include <QDebug>
#include <QUdpSocket>

class DevicePluginUdpCommander : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginudpcommander.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginUdpCommander();

    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;

    void executeAction(DeviceActionInfo *info) override;

private:
    QHash<QUdpSocket *, Device *> m_receiverList;
    QHash<QUdpSocket *, Device *> m_commanderList;

private slots:
    void readPendingDatagrams();

};

#endif // DEVICEPLUGINUDPCOMMANDER_H
