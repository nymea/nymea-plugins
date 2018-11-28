/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Michael Zanetti <michael.zanetti@nymea.io>          *
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

#ifndef DEVICEPLUGINMQTTCLIENT_H
#define DEVICEPLUGINMQTTCLIENT_H

#include "plugin/deviceplugin.h"

#include <QHash>
#include <QDebug>
#include <QUdpSocket>

class MqttClient;

class DevicePluginMqttClient: public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginmqttclient.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginMqttClient();

    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;

private slots:
    void subscribe(Device *device);

    void publishReceived(const QString &topic, const QByteArray &payload, bool retained);
    void published(quint16 packetId);


private:
    QHash<Device*, MqttClient*> m_clients;

    QHash<quint16, Action> m_pendingPublishes;
};

#endif // DEVICEPLUGINMQTTCLIENT_H
