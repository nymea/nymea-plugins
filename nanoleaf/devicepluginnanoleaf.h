/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#ifndef DEVICEPLUGINNANOLEAF_H
#define DEVICEPLUGINNANOLEAF_H

#include "devices/deviceplugin.h"
#include "nanoleaf.h"

#include "plugintimer.h"
#include "network/networkaccessmanager.h"
#include "network/zeroconf/zeroconfservicebrowser.h"

#include <QHostAddress>

class DevicePluginNanoleaf: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginnanoleaf.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginNanoleaf();

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void startPairing(DevicePairingInfo *info) override;
    void confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

    void browseDevice(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;

private:
    ZeroConfServiceBrowser *m_zeroconfBrowser = nullptr;
    PluginTimer *m_pluginTimer = nullptr;
    QHash<DeviceId, Nanoleaf*> m_nanoleafConnections;
    QHash<DeviceId, Nanoleaf*> m_unfinishedNanoleafConnections;
    QHash<QUuid, DeviceActionInfo *> m_asyncActions;
    QHash<Nanoleaf *, DevicePairingInfo *> m_unfinishedPairing;
    QHash<Nanoleaf *, DeviceSetupInfo *> m_asyncDeviceSetup;

    QHash<Nanoleaf *, BrowseResult *> m_asyncBrowseResults;
    QHash<QUuid, BrowserActionInfo *> m_asyncBrowserItem;
    QHash<DeviceId, int> m_hues;

    void getDeviceStates(Nanoleaf *nanoleaf);

public slots:
    void onAuthTokenReceived(const QString &token);
    void onAuthenticationStatusChanged(bool authenticated);
    void onRequestExecuted(QUuid requestId, bool success);
    void onConnectionChanged(bool connected);

    void onPowerReceived(bool power);
    void onBrightnessReceived(int percentage);
    void onColorModeReceived(const QString &colorMode);
    void onHueReceived(int hue);
    void onSaturationReceived(int percentage);
    void onEffectListReceived(const QStringList &effects);
    void onColorTemperatureReceived(int mired);
    void onSelectedEffectReceived(const QString &effect);
};

#endif // DEVICEPLUGINNANOLEAF_H
