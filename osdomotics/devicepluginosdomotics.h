/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of guh.                                              *
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

#ifndef DEVICEPLUGINOSDOMOTICS_H
#define DEVICEPLUGINOSDOMOTICS_H

#include "plugin/deviceplugin.h"

#include <QHash>
#include <QDebug>
#include <QNetworkReply>

#include "coap/coap.h"
#include "plugintimer.h"

class DevicePluginOsdomotics : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "guru.guh.DevicePlugin" FILE "devicepluginosdomotics.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginOsdomotics();
    ~DevicePluginOsdomotics();

    void init() override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    void postSetupDevice(Device *device) override;
    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    Coap *m_coap = nullptr;
    QHash<QNetworkReply *, Device *> m_asyncSetup;
    QHash<QNetworkReply *, Device *> m_asyncNodeRescans;

    QHash<CoapReply *, Device *> m_discoveryRequests;
    QHash<CoapReply *, Device *> m_updateRequests;
    QHash<CoapReply *, Action> m_toggleLightRequests;

    void scanNodes(Device *device);
    void parseNodes(Device *device, const QByteArray &data);
    void updateNode(Device *device);

    Device *findDevice(const QHostAddress &address);

private slots:
    void onPluginTimer();
    void onNetworkReplyFinished();
    void coapReplyFinished(CoapReply *reply);

};

#endif // DEVICEPLUGINOSDOMOTICS_H
