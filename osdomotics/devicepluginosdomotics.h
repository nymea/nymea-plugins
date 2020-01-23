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

#ifndef DEVICEPLUGINOSDOMOTICS_H
#define DEVICEPLUGINOSDOMOTICS_H

#include "devices/deviceplugin.h"

#include <QHash>
#include <QDebug>
#include <QNetworkReply>

#include "coap/coap.h"
#include "plugintimer.h"

class DevicePluginOsdomotics : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginosdomotics.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginOsdomotics();
    ~DevicePluginOsdomotics();

    void init() override;
    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;
    void postSetupDevice(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

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
