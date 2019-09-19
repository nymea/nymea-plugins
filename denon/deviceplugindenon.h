/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@nymea.io>                 *
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

#ifndef DEVICEPLUGINDENON_H
#define DEVICEPLUGINDENON_H

#include "heos.h"
#include "avrconnection.h"
#include "plugintimer.h"
#include "devices/deviceplugin.h"
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "network/zeroconf/zeroconfserviceentry.h"

#include <QProcess>
#include <QPair>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QHostAddress>
#include <QNetworkReply>

class DevicePluginDenon : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugindenon.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginDenon();

    Device::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    Device::DeviceSetupStatus setupDevice(Device *device) override;
    void postSetupDevice(Device * device) override;
    Device::DeviceError executeAction(Device *device, const Action &action) override;
    void deviceRemoved(Device *device) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    ZeroConfServiceBrowser *m_serviceBrowser = nullptr;

    QHash<Device *, AvrConnection*> m_avrConnections;
    QHash<Device *, Heos*> m_heos;

    QList<AvrConnection *> m_asyncAvrSetups;
    QList<Heos *> m_asyncHeosSetups;

    QHash<int, Device *> m_playerIds;
    QHash<int, Device *> m_discoveredPlayerIds;
    QHash<const Action *, int> m_asyncActions;


private slots:
    void onPluginTimer();
    void onUpnpDiscoveryFinished();

    void onHeosConnectionChanged(bool status);
    void onHeosPlayerDiscovered(HeosPlayer *heosPlayer);
    void onHeosPlayStateReceived(int playerId, PLAYER_STATE state);
    void onHeosShuffleModeReceived(int playerId, bool shuffle);
    void onHeosRepeatModeReceived(int playerId, REPEAT_MODE repeatMode);
    void onHeosMuteStatusReceived(int playerId, bool mute);
    void onHeosVolumeStatusReceived(int playerId, int volume);
    void onHeosNowPlayingMediaStatusReceived(int playerId, SOURCE_ID source, QString artist, QString album, QString Song, QString artwork);

    void onAvahiServiceEntryAdded(const ZeroConfServiceEntry &serviceEntry);
    void onAvahiServiceEntryRemoved(const ZeroConfServiceEntry &serviceEntry);
    void onAvrConnectionChanged(bool status);
    void onAvrSocketError();
    void onAvrVolumeChanged(int volume);
    void onAvrChannelChanged(const QByteArray &channel);
    void onAvrMuteChanged(bool mute);
    void onAvrPowerChanged(bool power);
    void onAvrSurroundModeChanged(const QByteArray &surroundMode);
};

#endif // DEVICEPLUGINDENON_H
