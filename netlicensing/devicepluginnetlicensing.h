/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#ifndef DEVICEPLUGINNETLICENSING_H
#define DEVICEPLUGINNETLICENSING_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"

#include <QNetworkReply>
#include <QHostInfo>

class DevicePluginNetlicensing : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginnetlicensing.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginNetlicensing();

    void init() override;
    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;
    void postSetupDevice(Device *device) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QString m_baseUrl = "https://go.netlicensing.io/core/v2/rest/";
    QHash<QNetworkReply *, Device *> m_httpRequests;

    void validateLicensee(const QString &licenseeNumber);
    void transferLicense(const QString &licenseeNumber, const QString &sourceLicenseeNumber);

private slots:
    void onPluginTimer();
    void onNetworkReplyFinished();
    void onAuthenticationChanged();
};

#endif // DEVICEPLUGINNETLICENSING_H
