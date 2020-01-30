/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Michael Zanetti <michael.zanetti@nymea.io>          *
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

#ifndef DEVICEPLUGINTPLINK_H
#define DEVICEPLUGINTPLINK_H

#include "devices/deviceplugin.h"

#include <QUdpSocket>

#include <QNetworkAccessManager>

class PluginTimer;

class DevicePluginTPLink: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugintplink.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginTPLink();
    ~DevicePluginTPLink();

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

private:
    QByteArray encryptPayload(const QByteArray &payload);
    QByteArray decryptPayload(const QByteArray &payload);

    void connectToDevice(Device *device, const QHostAddress &address);
    void fetchState(Device *device, DeviceActionInfo *info = nullptr);

    void processQueue(Device *device);

private:
    class Job {
    public:
        int id = 0;
        QByteArray data;
        DeviceActionInfo *actionInfo = nullptr;
        bool operator==(const Job &other) { return id == other.id; }
    };
    QHash<Device*, Job> m_pendingJobs;
    QHash<Device*, QList<Job>> m_jobQueue;
    int m_jobIdx = 0;

    QUdpSocket *m_broadcastSocket = nullptr;
    QHash<Device*, QTcpSocket*> m_sockets;
    QHash<DeviceSetupInfo*, int> m_setupRetries;

    QHash<Device*, QByteArray> m_inputBuffers;

    PluginTimer *m_timer = nullptr;
};

#endif // DEVICEPLUGINANEL_H
