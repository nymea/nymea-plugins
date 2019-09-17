/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

#ifndef DEVICEPLUGINEQ3_H
#define DEVICEPLUGINEQ3_H

#include "devices/deviceplugin.h"
#include "maxcubediscovery.h"
#include "plugintimer.h"
#include "eqivabluetooth.h"

#include <QHostAddress>

class QNetworkReply;

class DevicePluginEQ3: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugineq-3.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginEQ3();
    ~DevicePluginEQ3();

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;

    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;

    void executeAction(DeviceActionInfo *info) override;

private:
    QString modeToString(EqivaBluetooth::Mode mode);
    EqivaBluetooth::Mode stringToMode(const QString &string);

    PluginTimer *m_pluginTimer = nullptr;
    QList<Param> m_config;
    QHash<MaxCube *, Device *> m_cubes;

    QHash<Device*, EqivaBluetooth*> m_eqivaDevices;

private slots:
    void onPluginTimer();
    void cubeConnectionStatusChanged(const bool &connected);

    void wallThermostatFound();
    void radiatorThermostatFound();

    void updateCubeConfig();
    void wallThermostatDataUpdated();
    void radiatorThermostatDataUpdated();

};

#endif // DEVICEPLUGINEQ3_H
