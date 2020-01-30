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

    Nanoleaf *createNanoleafConnection(const QHostAddress &address, int port);

public slots:
    void onAuthTokenReceived(const QString &token);
    void onAuthenticationStatusChanged(bool authenticated);
    void onRequestExecuted(QUuid requestId, bool success);
    void onConnectionChanged(bool connected);

    void onControllerInfoReceived(const Nanoleaf::ControllerInfo &controllerInfo);
    void onPowerReceived(bool power);
    void onBrightnessReceived(int percentage);
    void onColorReceived(QColor color);
    void onColorModeReceived(Nanoleaf::ColorMode colorMode);
    void onHueReceived(int hue);
    void onSaturationReceived(int percentage);
    void onEffectListReceived(const QStringList &effects);
    void onColorTemperatureReceived(int kelvin);
    void onSelectedEffectReceived(const QString &effect);
};

#endif // DEVICEPLUGINNANOLEAF_H
