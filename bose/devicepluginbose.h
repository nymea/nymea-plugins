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

#ifndef DEVICEPLUGINBOSE_H
#define DEVICEPLUGINBOSE_H

#include "devices/deviceplugin.h"
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "plugintimer.h"
#include "soundtouch.h"
#include "soundtouchtypes.h"

#include <QHash>
#include <QDebug>
#include <QUuid>

class DevicePluginBose : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginbose.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginBose();
    ~DevicePluginBose() override;

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

    void browseDevice(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;

private:
    ZeroConfServiceBrowser *m_serviceBrowser = nullptr;
    PluginTimer *m_pluginTimer = nullptr;

    QHash<Device *, SoundTouch *> m_soundTouch;
    QHash<QUuid, DeviceActionInfo *> m_pendingActions;
    QHash<QUuid, BrowseResult *> m_asyncBrowseResults;
    QHash<QUuid, BrowserActionInfo *> m_asyncExecuteBrowseItems;
    QHash<QUuid, BrowserItemResult *> m_asyncBrowseItemResults;
    QHash<Device *, SourcesObject> m_sourcesObjects;

private slots:
    void onPluginTimer();
    void onConnectionChanged(bool connected);
    void onDeviceNameChanged();
    void onRequestExecuted(QUuid requestId, bool success);

   void onInfoObjectReceived(QUuid requestId, InfoObject infoObject);
   void onNowPlayingObjectReceived(QUuid requestId, NowPlayingObject nowPlaying);
   void onVolumeObjectReceived(QUuid requestId, VolumeObject volume);
   void onSourcesObjectReceived(QUuid requestId, SourcesObject sources);
   void onBassObjectReceived(QUuid requestId, BassObject bass);
   void onBassCapabilitiesObjectReceived(QUuid requestId, BassCapabilitiesObject bassCapabilities);
   void onGroupObjectReceived(QUuid requestId, GroupObject group);
   void onZoneObjectReceived(QUuid requestId, ZoneObject zone);
   void onPresetsReceived(QUuid requestId, QList<PresetObject> presets);
};

#endif // DEVICEPLUGINBOSE_H
