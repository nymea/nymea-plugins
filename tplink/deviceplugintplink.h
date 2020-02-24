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

    QHash<DeviceClassId, ParamTypeId> m_idParamTypesMap;
    QHash<DeviceClassId, StateTypeId> m_connectedStateTypesMap;
    QHash<DeviceClassId, StateTypeId> m_powerStatetTypesMap;
    QHash<DeviceClassId, StateTypeId> m_currentPowerStatetTypesMap;
    QHash<DeviceClassId, StateTypeId> m_totalEnergyConsumedStatetTypesMap;
    QHash<DeviceClassId, ParamTypeId> m_powerActionParamTypesMap;
};

#endif // DEVICEPLUGINANEL_H
