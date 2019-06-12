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

#include "devicemanager.h"
#include "plugin/deviceplugin.h"
#include "plugintimer.h"
#include "denonconnection.h"
#include "heos.h"

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

    DeviceManager::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    void postSetupDevice(Device * device) override;
    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;
    void deviceRemoved(Device *device) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<Device *, DenonConnection*> m_denonConnections;
    QHash<Device *, Heos*> m_heos;

    QList<DenonConnection *> m_asyncSetups;

    QHash<int, Device *> m_playerIds;
    QHash<int, Device *> m_discoveredPlayerIds;
    QHash<const Action *, int> m_asyncActions;


private slots:
    void onPluginTimer();

    void onAVRConnectionChanged();
    void onAVRDataReceived(const QByteArray &data);
    void onAVRSocketError();

    void onUpnpDiscoveryFinished();

    void onHeosConnectionChanged();
    void onHeosPlayerDiscovered(HeosPlayer *heosPlayer);
    void onHeosPlayStateReceived(int playerId, Heos::HeosPlayerState state);
    void onHeosShuffleModeReceived(int playerId, bool shuffle);
    void onHeosRepeatModeReceived(int playerId, Heos::HeosRepeatMode repeatMode);
    void onHeosMuteStatusReceived(int playerId, bool mute);
    void onHeosVolumeStatusReceived(int playerId, int volume);
    void onHeosNowPlayingMediaStatusReceived(int playerId, QString source, QString artist, QString album, QString Song, QString artwork);
};

#endif // DEVICEPLUGINDENON_H
