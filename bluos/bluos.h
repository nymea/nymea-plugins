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

#ifndef BLUOS_H
#define BLUOS_H

#include <QObject>
#include <QTimer>
#include <QHostAddress>
#include <QUuid>

#include "network/networkaccessmanager.h"
#include "integrations/thing.h"

class BluOS : public QObject
{
    Q_OBJECT
public:

    enum PlaybackCommand {
        Play,
        Pause,
        Stop,
        Skip,
        Back
    };

    enum RepeatMode {
        All,
        One,
        None
    };

    enum PlaybackState {
        Playing,
        Paused,
        Stopped,
        Connecting,
        Streaming
    };

    struct StatusResponse {
      QString Album;
      QString Artist;
      QString Name;
      QString Title;
      QString Service;
      QUrl ServiceIcon;
      PlaybackState State;
      QUrl StationUrl;
      int Volume;
      bool Mute;
      RepeatMode Repeat;
      bool Shuffle;
      QUrl Image;
    };

    struct Preset {
        QString Name;
        int Id;
        QString Url;
    };

    struct Source {
        //<item image="/images/ci_myplaylists.png" browseKey="playlists" text="Playlists" type="link"/>
        QString Image;
        QString BrowseKey;
        QString Text;
        QString Type;
    };

    explicit BluOS(NetworkAccessManager *networkManager, QHostAddress hostAddress, int port, QObject *parent = nullptr);
    int port();
    QHostAddress hostAddress();
    
    // Status Queries
    QUuid getStatus();
    
    // Volume Control
    QUuid setVolume(uint volume);
    QUuid setMute(bool mute);

    // Playback Control
    QUuid play();
    QUuid pause();
    QUuid stop();
    QUuid skip();
    QUuid back();
    QUuid setShuffle(bool shuffle);
    QUuid setRepeat(RepeatMode repeatMode);
    
    // Presets
    QUuid listPresets();
    QUuid loadPreset(int preset); //1 for next preset, -1 for previous preset
    
    // Content Browsing
    QUuid getSources();
    
    // Player Grouping
    QUuid addGroupPlayer(QHostAddress address, int port); //adds player as slave
    QUuid removeGroupPlayer(QHostAddress address, int port);
    
private:
    QHostAddress m_hostAddress;
    int m_port;
    NetworkAccessManager *m_networkManager = nullptr;

    QUuid playBackControl(PlaybackCommand command);
    bool parseState(const QByteArray &state);

signals:
    void connectionChanged(bool connected);
    void actionExecuted(QUuid actionId, bool success);

    void statusReceived(const StatusResponse &status);
    void volumeReceived(int volume, bool mute);

    void presetsReceived(QUuid requestId, const QList<Preset> &presets);
    void sourcesReceived(QUuid requestId, const QList<Source> &sources);
};
#endif // BLUOS_H
