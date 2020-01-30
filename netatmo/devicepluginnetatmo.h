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

#ifndef DEVICEPLUGINNETATMO_H
#define DEVICEPLUGINNETATMO_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"
#include "network/oauth2.h"
#include "netatmobasestation.h"
#include "netatmooutdoormodule.h"

#include <QHash>
#include <QTimer>

class DevicePluginNetatmo : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginnetatmo.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginNetatmo();
    ~DevicePluginNetatmo();

    void init() override;
    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;
    void postSetupDevice(Device *device) override;

private:
    PluginTimer *m_pluginTimer = nullptr;

    QHash<QString, QVariantMap> m_indoorStationInitData;
    QHash<QString, QVariantMap> m_outdoorStationInitData;

    QHash<OAuth2 *, Device *> m_authentications;
    QHash<NetatmoBaseStation *, Device *> m_indoorDevices;
    QHash<NetatmoOutdoorModule *, Device *> m_outdoorDevices;

    QHash<QNetworkReply *, Device *> m_refreshRequest;

    void refreshData(Device *device, const QString &token);
    void processRefreshData(const QVariantMap &data, Device *connectionDevice);

    Device *findIndoorDevice(const QString &macAddress);
    Device *findOutdoorDevice(const QString &macAddress);

private slots:
    void onPluginTimer();
    void onNetworkReplyFinished();
    void onAuthenticationChanged();
    void onIndoorStatesChanged();
    void onOutdoorStatesChanged();

};

#endif // DEVICEPLUGINNETATMO_H
