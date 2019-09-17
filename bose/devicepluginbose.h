/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io         *
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

#ifndef DEVICEPLUGINBOSE_H
#define DEVICEPLUGINBOSE_H

#include "devices/deviceplugin.h"
#include "plugintimer.h"
#include "soundtouch.h"
#include "soundtouchtypes.h"

#include <QHash>
#include <QDebug>

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

private:
    PluginTimer *m_pluginTimer = nullptr;

    QHash<Device *, SoundTouch *> m_soundTouch;
    QHash<int, ActionId> m_pendingActions;

private slots:
    void onPluginTimer();
    void onConnectionChanged(bool connected);
    void onDeviceNameChanged();

   void onInfoObjectReceived(InfoObject infoObject);
   void onNowPlayingObjectReceived(NowPlayingObject nowPlaying);
   void onVolumeObjectReceived(VolumeObject volume);
   void onSourcesObjectReceived(SourcesObject sources);
   void onBassObjectReceived(BassObject bass);
   void onBassCapabilitiesObjectReceived(BassCapabilitiesObject bassCapabilities);
   void onGroupObjectReceived(GroupObject group);
   void onZoneObjectReceived(ZoneObject zone);
};

#endif // DEVICEPLUGINBOSE_H
