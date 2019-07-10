#ifndef SOUNDTOUCH_H
#define SOUNDTOUCH_H

#include <QObject>
#include <QXmlStreamReader>
#include <QtWebSockets/QtWebSockets>

#include "extern-plugininfo.h"
#include "soundtouchtypes.h"

#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"


class SoundTouch : public QObject
{
    Q_OBJECT
public:
    explicit SoundTouch(NetworkAccessManager *networkAccessManager, QString ipAddress, QObject *parent = nullptr);

    void getInfo();             //Get basic information about a product.
    void getVolume();           //Get the current volume and mute status of a product.
    void getNowPlaying();       //Get information about what's playing on a product.
    void getBass();             //Get the bass level of a product, if supported.
    void getGroup();            //Get the current left/right stereo pair configuration of a product.
    void getSources();           //Get available sources for a product.
    void getZone();             //Get the current multiroom zone state of a product.
    //void getPresets(); //Get information related to the user's SoundTouch presets.
    //void getBassCapabilities(); //Get whether a product supports reducing bass.
    //void speaker();             //initiate playback of a specified network-accessible audio file on a Bose SoundTouch product.
    void setKey(KEY_VALUE keyValue);        //Start and control playback on a product.
    void setVolume(int volume);             //Set the volume of a product.
    void setSource(SOURCE_STATUS source);         //Select a product's source.
    /*void setZone(QString zone); //Create a zone of synced products.
    void addZoneSlave(); //Add one or more slave product(s) to a multiroom zone.
    void removeZoneSlave(); //Remove one or more slave product(s) from a multiroom zone.
    void setBass(int volume);  //Set the bass level of a product, if supported.*/
    void setName(QString name); //Set the products user-facing name.

private:
    NetworkAccessManager *m_networkAccessManager = nullptr;
    QString m_ipAddress;
    int m_port = 8090;
    QWebSocket *m_websocket = nullptr;
    //QHash<QNetworkReply *, Device*> m_requests;

signals:
    void connectionChanged(bool connected);
    void infoReceived(InfoObject info);
    void nowPlayingReceived(NowPlayingObject nowPlaying);
    void volumeReceived(VolumeObject volume);
    void sourcesReceived(SourcesObject sources);
    void zoneReceived(ZoneObject);
    void bassCapabilitiesReceived(BassCapabilitiesObject bassCapabilities);
    void bassReceived(BassObject bass);
    void presetsReceived(PresetObject presets);
    void groupReceived(GroupObject group);


private slots:
    void onWebsocketConnected();
    void onWebsocketDisconnected();
    void onWebsocketMessageReceived(QString message);
    void onRestRequestFinished();
};

#endif // SOUNDTOUCH_H
