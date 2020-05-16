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

#ifndef STREAMUNLIMITEDDEVICE_H
#define STREAMUNLIMITEDDEVICE_H

#include <QObject>
#include <QHostAddress>
#include <QUuid>
#include <QNetworkReply>

#include <types/browseritem.h>
#include <types/mediabrowseritem.h>

class NetworkAccessManager;

class StreamUnlimitedSetRequest;

class StreamUnlimitedDevice : public QObject
{
    Q_OBJECT
public:
    enum ConnectionStatus {
        ConnectionStatusDisconnected,
        ConnectionStatusConnecting,
        ConnectionStatusConnected,
        ConnectionStatusError
    };

    enum PlayStatus {
        PlayStatusStopped,
        PlayStatusPlaying,
        PlayStatusPaused
    };

    enum Repeat {
        RepeatNone,
        RepeatOne,
        RepeatAll
    };

    explicit StreamUnlimitedDevice(NetworkAccessManager *nam, QObject *parent = nullptr);

    void setHost(const QHostAddress &address, int port);
    QHostAddress address() const;
    int port() const;

    ConnectionStatus connectionStatus();

    QLocale language() const;

    PlayStatus playbackStatus() const;

    uint volume() const;
    int setVolume(uint volume);

    bool mute() const;
    int setMute(bool mute);

    bool shuffle() const;
    int setShuffle(bool shuffle);

    Repeat repeat() const;
    int setRepeat(Repeat repeat);

    QString title() const;
    QString artist() const;
    QString album() const;
    QString artwork() const;

    int play();
    int pause();
    int stop();
    int skipBack();
    int skipNext();

    int browseDevice(const QString &itemId);
    int playBrowserItem(const QString &itemId);
    int browserItem(const QString &itemId);

    int setLocaleOnBoard(const QLocale &locale);

    int storePreset(uint presetId);
    int loadPreset(uint presetId);

signals:
    void connectionStatusChanged(ConnectionStatus status);
    void commandCompleted(int commandId, bool success);

    void playbackStatusChanged(PlayStatus status);
    void volumeChanged(uint volume);
    void muteChanged(bool mute);

    void titleChanged(const QString &title);
    void artistChanged(const QString &artist);
    void albumChanged(const QString &album);
    void artworkChanged(const QString &artwork);

    void shuffleChanged(bool shuffle);
    void repeatChanged(Repeat repeat);

    void browseResults(int commandId, bool success, const BrowserItems &items = BrowserItems());
    void browserItemResult(int commandId, bool success, const BrowserItem &item = BrowserItem());

private:
    void pollQueue();

    void refreshPlayerData();
    void refreshVolume();
    void refreshMute();
    void refreshPlayMode();
    void refreshLanguage();

    StreamUnlimitedSetRequest* setPlayMode(bool shuffle, Repeat repeat);
    int executeControlCommand(const QString &command);

    int browseInternal(const QString &itemId, int commandIdOverride = -1);

private:
    NetworkAccessManager *m_nam = nullptr;
    QNetworkReply *m_pollReply = nullptr;
    QHostAddress m_adddress;
    int m_port = 80;

    ConnectionStatus m_connectionStatus = ConnectionStatusDisconnected;
    QUuid m_pollQueueId;

    int m_commandId = 0;

    PlayStatus m_playbackStatus = PlayStatusStopped;
    uint m_volume = 0;
    bool m_mute = false;
    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_artwork;

    bool m_shuffle = false;
    Repeat m_repeat = RepeatNone;

    QLocale m_language;
};

#endif // STREAMUNLIMITEDDEVICE_H
