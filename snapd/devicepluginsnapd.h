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

#ifndef DEVICEPLUGINSNAPD_H
#define DEVICEPLUGINSNAPD_H

#include "devices/deviceplugin.h"
#include "plugintimer.h"

#include <QDebug>
#include <QNetworkInterface>
#include <QProcess>
#include <QUrlQuery>

#include "snapdcontrol.h"


class DevicePluginSnapd: public DevicePlugin {
	Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginsnapd.json")
	Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginSnapd();
    ~DevicePluginSnapd();

    void init() override;
    void startMonitoringAutoDevices() override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

    void setupDevice(DeviceSetupInfo *info) override;
    void executeAction(DeviceActionInfo *info) override;

private:
    SnapdControl *m_snapdControl = nullptr;
    PluginTimer *m_refreshTimer = nullptr;
    PluginTimer *m_updateTimer = nullptr;

    bool m_advancedMode = false;
    int m_refreshTime = 2;

    // Snap list for faster access (snap id, device)
    QHash<QString, Device *> m_snapDevices;

private slots:
    void onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value);
    void onRefreshTimer();
    void onUpdateTimer();
    void onSnapListUpdated(const QVariantList &snapList);

};

#endif // DEVICEPLUGINSNAPD_H
