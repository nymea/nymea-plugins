/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017-2018 Simon Stürz <simon.stuerz@guh.io               *
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

#ifndef DEVICEPLUGINSNAPD_H
#define DEVICEPLUGINSNAPD_H

#include "devicemanager.h"
#include "plugin/deviceplugin.h"
#include "plugintimer.h"

#include <QDebug>
#include <QNetworkInterface>
#include <QProcess>
#include <QUrlQuery>

#include "snapdcontrol.h"


class DevicePluginSnapd: public DevicePlugin {
	Q_OBJECT
    Q_PLUGIN_METADATA(IID "guru.guh.DevicePlugin" FILE "devicepluginsnapd.json")
	Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginSnapd();
    void init() override;
    void startMonitoringAutoDevices() override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

	DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;    
	DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;

private:
    SnapdControl *m_snapdControl = nullptr;
    PluginTimer *m_refreshTimer = nullptr;
    PluginTimer *m_updateTimer = nullptr;

    bool m_advancedMode = false;

    // Snap list for faster access (snap id, device)
    QHash<QString, Device *> m_snapDevices;

private slots:
    void onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value);
    void onRefreshTimer();
    void onUpdateTimer();
    void onSnapListUpdated(const QVariantList &snapList);

};

#endif // DEVICEPLUGINSNAPD_H
