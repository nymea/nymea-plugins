#ifndef HEOS_H
#define HEOS_H

#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>
#include "heosplayer.h"

class Heos : public QObject
{
    Q_OBJECT
public:
    enum HeosPlayerState {
        Play = 0,
        Pause = 1,
        Stop = 2
    };

    enum HeosRepeatMode {
        Off = 0,
        One = 1,
        All = 2
    };

    explicit Heos(const QHostAddress &hostAddress, QObject *parent = 0);
    ~Heos();

    void setAddress(QHostAddress address);
    QHostAddress getAddress();
    bool connected();
    void connectHeos();

    void getPlayers();
    HeosPlayer *getPlayer(int playerId);
    void getPlayerState(int playerId);
    void setPlayerState(int playerId, HeosPlayerState state);
    void getVolume(int playerId);
    void setVolume(int playerId, int volume);
    void getMute(int playerId);
    void setMute(int playerId, bool state);
    void setPlayMode(int playerId, HeosRepeatMode repeatMode, bool shuffle); //shuffle and repead mode
    void getPlayMode(int playerId);
    void playNext(int playerId);
    void playPrevious(int playerId);
    void getNowPlayingMedia(int playerId);
    void registerForChangeEvents(bool state);
    void sendHeartbeat();

private:
    bool m_connected;
    bool m_eventRegistered;
    QHostAddress m_hostAddress;
    QTcpSocket *m_socket;
    QHash<int, HeosPlayer*> m_heosPlayers;
    void setConnected(const bool &connected);

signals:
    void playerDiscovered(HeosPlayer *heosPlayer);
    void connectionStatusChanged();

    void playStateReceived(int playerId, HeosPlayerState state);
    void shuffleModeReceived(int playerId, bool shuffle);
    void repeatModeReceived(int playerId, HeosRepeatMode repeatMode);
    void muteStatusReceived(int playerId, bool mute);
    void volumeStatusReceived(int playerId, int volume);
    void nowPlayingMediaStatusReceived(int playerId, QString source, QString artist, QString album, QString Song, QString artwork);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void readData();
};


#endif // HEOS_H
