/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016-2019 Bernhard Trinnes <bernhard.trinnes@nymea.io    *
 *  Copyright (C) 2016-2018 Simon Stuerz <simon.stuerz@guh.io>             *
 *  Copyright (C) 2016 Christian Stachowitz                                *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  Nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DEVICEPLUGINKEBA_H
#define DEVICEPLUGINKEBA_H

#include "plugin/deviceplugin.h"
#include "devicemanager.h"
#include "plugintimer.h"
#include "host.h"
#include "discovery.h"

#include <QHash>
#include <QNetworkReply>
#include <QUdpSocket>

class DevicePluginKeba : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginkeba.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginKeba();
    ~DevicePluginKeba();

    DeviceManager::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;

    void postSetupDevice(Device* device) override;
    void deviceRemoved(Device* device) override;

    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QUdpSocket *m_kebaSocket = nullptr;
    Discovery *m_discovery = nullptr;
    QHash<QHostAddress, Device *> m_kebaDevices;
    QHash<Device *, int> m_connectionTimoutCounter;


    void enableOutput(bool state, Device *device);
    void setMaxAmpere(int milliAmpere, Device *device);
    void getDeviceInformation(Device *device);
    void getReport1(Device *device);
    void getReport2(Device *device);
    void getReport3(Device *device);
    void unlockCharger(Device *device);
    void sendData(const QByteArray &data, const QHostAddress &address);
    void displayMessage(const QByteArray &message, Device *device);
    void rediscoverDevice(Device *device);
private slots:
    void readPendingDatagrams();
    void updateData();
    void discoveryFinished(const QList<Host> &hosts);
};

#endif // DEVICEPLUGINKEBA_H
