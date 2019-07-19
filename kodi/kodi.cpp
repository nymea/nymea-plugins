/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "kodi.h"
#include <QDebug>
#include "extern-plugininfo.h"
#include <QUrl>

Kodi::Kodi(const QHostAddress &hostAddress, int port, int httpPort, QObject *parent) :
    QObject(parent),
    m_httpPort(httpPort),
    m_muted(false),
    m_volume(-1)
{
    m_connection = new KodiConnection(hostAddress, port, this);
    connect (m_connection, &KodiConnection::connectionStatusChanged, this, &Kodi::connectionStatusChanged);

    m_jsonHandler = new KodiJsonHandler(m_connection, this);
    connect(m_jsonHandler, &KodiJsonHandler::notificationReceived, this, &Kodi::processNotification);
    connect(m_jsonHandler, &KodiJsonHandler::replyReceived, this, &Kodi::processResponse);


    // Init FS
    m_virtualFs = new VirtualFsNode(BrowserItem());

    QVariantMap sort;
    sort.insert("method", "label");
    sort.insert("ignorearticle", true);

    QVariantList properties;
    properties.append("thumbnail");

    // Music
    BrowserItem item = BrowserItem("audiolibrary", tr("Music library"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *audioLibrary = new VirtualFsNode(item);
    m_virtualFs->addChild(audioLibrary);

    item = BrowserItem("artists", tr("Artists"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *artists = new VirtualFsNode(item);
    artists->getMethod = "AudioLibrary.GetArtists";
    artists->getParams.insert("sort", sort);
    artists->getParams.insert("properties", properties);
    audioLibrary->addChild(artists);

    item = BrowserItem("albums", tr("Albums"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *albums = new VirtualFsNode(item);
    albums->getMethod = "AudioLibrary.GetAlbums";
    albums->getParams.insert("sort", sort);
    albums->getParams.insert("properties", properties);
    audioLibrary->addChild(albums);

    item = BrowserItem("songs", tr("Songs"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *songs = new VirtualFsNode(item);
    songs->getMethod = "AudioLibrary.GetSongs";
    songs->getParams.insert("sort", sort);
    songs->getParams.insert("properties", properties);
    audioLibrary->addChild(songs);

    // Video
    item = BrowserItem("videolibrary", tr("Video library"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *videoLibrary = new VirtualFsNode(item);
    m_virtualFs->addChild(videoLibrary);

    item = BrowserItem("movies", tr("Movies"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *movies = new VirtualFsNode(item);
    movies->getMethod = "VideoLibrary.GetMovies";
    movies->getParams.insert("sort", sort);
    movies->getParams.insert("properties", properties);
    videoLibrary->addChild(movies);

    item = BrowserItem("tvshows", tr("TV Shows"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *tvShows = new VirtualFsNode(item);
    tvShows->getMethod = "VideoLibrary.GetTVShows";
    tvShows->getParams.insert("sort", sort);
    tvShows->getParams.insert("properties", properties);
    videoLibrary->addChild(tvShows);

    item = BrowserItem("musicvideos", tr("Music Videos"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *musicVideos = new VirtualFsNode(item);
    musicVideos->getMethod = "VideoLibrary.GetMusicVideos";
    musicVideos->getParams.insert("sort", sort);
    musicVideos->getParams.insert("properties", properties);
    videoLibrary->addChild(musicVideos);

    item = BrowserItem("addons", tr("Add-ons"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *addons = new VirtualFsNode(item);
    m_virtualFs->addChild(addons);

    item = BrowserItem("videoaddons", tr("Video add-ons"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *videoAddons = new VirtualFsNode(item);
    videoAddons->getMethod = "Files.GetDirectory";
    videoAddons->getParams.insert("directory", "addons://sources/video");
    videoAddons->getParams.insert("sort", sort);
    videoAddons->getParams.insert("properties", properties);
    addons->addChild(videoAddons);

    item = BrowserItem("musicaddons", tr("Music add-ons"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *musicAddons = new VirtualFsNode(item);
    musicAddons->getMethod = "Files.GetDirectory";
    musicAddons->getParams.insert("directory", "addons://sources/audio");
    musicAddons->getParams.insert("sort", sort);
    musicAddons->getParams.insert("properties", properties);
    addons->addChild(musicAddons);


}

QHostAddress Kodi::hostAddress() const
{
    return m_connection->hostAddress();
}

int Kodi::port() const
{
    return m_connection->port();
}

bool Kodi::connected() const
{
    return m_connection->connected();
}

int Kodi::setMuted(const bool &muted)
{
    QVariantMap params;
    params.insert("mute", muted);

    return m_jsonHandler->sendData("Application.SetMute", params);
}

bool Kodi::muted() const
{
    return m_muted;
}

int Kodi::setVolume(const int &volume)
{
    QVariantMap params;
    params.insert("volume", volume);

    return m_jsonHandler->sendData("Application.SetVolume", params);
}

int Kodi::volume() const
{
    return m_volume;
}

int Kodi::setShuffle(bool shuffle)
{
    QVariantMap params;
    params.insert("playerid", m_activePlayer);
    params.insert("shuffle", shuffle);
    return m_jsonHandler->sendData("Player.SetShuffle", params);
}

int Kodi::setRepeat(const QString &repeat)
{
    QVariantMap params;
    params.insert("playerid", m_activePlayer);
    params.insert("repeat", repeat);
    return m_jsonHandler->sendData("Player.SetRepeat", params);
}

int Kodi::showNotification(const QString &message, const int &displayTime, const QString &notificationType)
{
    QVariantMap params;
    params.insert("title", "nymea notification");
    params.insert("message", message);
    params.insert("displaytime", displayTime);
    params.insert("image", notificationType);

    return m_jsonHandler->sendData("GUI.ShowNotification", params);
}

int Kodi::pressButton(const QString &button)
{
    QVariantMap params;
    params.insert("action", button);
    return m_jsonHandler->sendData("Input.ExecuteAction", params);
}

int Kodi::systemCommand(const QString &command)
{
    QString method;
    if (command == "hibernate") {
        method = "Hibernate";
    } else if (command == "reboot") {
        method = "Reboot";
    } else if (command == "shutdown") {
        method = "Shutdown";
    } else if (command == "suspend") {
        method = "Suspend";
    } else {
        // already checkt with allowed values
    }

    return m_jsonHandler->sendData("System." + method, QVariantMap());
}

int Kodi::videoLibrary(const QString &command)
{
    QString method;
    if (command == "scan") {
        method = "Scan";
    } else if (command == "clean") {
        method = "Clean";
    } else {
        // already checkt with allowed values
    }

    return m_jsonHandler->sendData("VideoLibrary." + method, QVariantMap());
}

int Kodi::audioLibrary(const QString &command)
{
    QString method;
    if (command == "scan") {
        method = "Scan";
    } else if (command == "clean") {
        method = "Clean";
    } else {
        // already checkt with allowed values
    }

    return m_jsonHandler->sendData("AudioLibrary." + method, QVariantMap());
}

void Kodi::update()
{
    QVariantMap params;
    QVariantList properties;
    properties.append("volume");
    properties.append("muted");
    properties.append("name");
    properties.append("version");
    params.insert("properties", properties);

    m_jsonHandler->sendData("Application.GetProperties", params);

    params.clear();
    m_jsonHandler->sendData("Player.GetActivePlayers", params);
}

void Kodi::checkVersion()
{
    m_jsonHandler->sendData("JSONRPC.Version", QVariantMap());
}

void Kodi::connectKodi()
{
    m_connection->connectKodi();
}

void Kodi::disconnectKodi()
{
    m_connection->disconnectKodi();
}

Device::BrowseResult Kodi::browse(const QString &itemId, Device::BrowseResult &result)
{
//    m_jsonHandler->sendData()
    VirtualFsNode *node = m_virtualFs->findNode(itemId);

    if (node) {
        if (node->getMethod.isEmpty()) {
            foreach (VirtualFsNode *child, node->childs) {
                result.items.append(child->item);
            }
            return result;
        }

        qCDebug(dcKodi()) << "Sending:" << node->getMethod << node->getParams;
        int id = m_jsonHandler->sendData(node->getMethod, node->getParams);
        m_pendingBrowseRequests.insert(id, result);
        result.status = Device::DeviceErrorAsync;
        return result;
    }

    QVariantMap sort;
    sort.insert("method", "label");
    sort.insert("ignorearticle", true);
    QVariantList properties;
    properties.append("thumbnail");

    if (itemId.startsWith("artist:")) {
        QString idString = itemId;
        idString.remove(QRegExp("^artist:"));
        QVariantMap filter;
        filter.insert("artistid", idString.toInt());
        QVariantMap params;
        params.insert("filter", filter);
        params.insert("properties", properties);
        int id = m_jsonHandler->sendData("AudioLibrary.GetAlbums", params);
        m_pendingBrowseRequests.insert(id, result);
        result.status = Device::DeviceErrorAsync;
        return result;
    }

    if (itemId.startsWith("album:")) {
        QString idString = itemId;
        idString.remove(QRegExp("^album:"));
        QVariantMap filter;
        filter.insert("albumid", idString.toInt());
        QVariantMap params;
        params.insert("filter", filter);
        QVariantList properties;
        properties.append("thumbnail");
        properties.append("albumid");
        params.insert("properties", properties);
        int id = m_jsonHandler->sendData("AudioLibrary.GetSongs", params);
        m_pendingBrowseRequests.insert(id, result);
        result.status = Device::DeviceErrorAsync;
        return result;
    }

    if (itemId.startsWith("tvshow:")) {
        QString idString = itemId;
        idString.remove(QRegExp("^tvshow:"));
        QVariantMap params;
        params.insert("tvshowid", idString.toInt());
        QVariantList properties;
        properties.append("tvshowid");
        properties.append("season");
        properties.append("thumbnail");
        params.insert("properties", properties);
        int id = m_jsonHandler->sendData("VideoLibrary.GetSeasons", params);
        m_pendingBrowseRequests.insert(id, result);
        result.status = Device::DeviceErrorAsync;
        return result;
    }

    if (itemId.startsWith("season:")) {
        QString idString = itemId;
        idString.remove(QRegExp("^season:"));
        int seasonId = idString.left(idString.indexOf(",")).toInt();
        idString.remove(QRegExp("^[0-9]*,tvshow:"));
        int tvShowId = idString.toInt();
        QVariantMap params;
        params.insert("tvshowid", tvShowId);
        params.insert("season", seasonId);
        params.insert("properties", properties);
        qCDebug(dcKodi()) << "getting episodes:" << params;
        int id = m_jsonHandler->sendData("VideoLibrary.GetEpisodes", params);
        m_pendingBrowseRequests.insert(id, result);
        result.status = Device::DeviceErrorAsync;
        return result;
    }

    if (itemId.startsWith("addon:")) {
        QString idString = itemId;
        idString.remove(QRegExp("^addon:"));
        QVariantMap params;
        params.insert("directory", "plugin://" + idString);
//        QVariantList properties;
//        properties.append("tvshowid");
//        properties.append("season");
//        params.insert("properties", properties);
        qCDebug(dcKodi()) << "Sending" << params;
        int id = m_jsonHandler->sendData("Files.GetDirectory", params);
        m_pendingBrowseRequests.insert(id, result);
        result.status = Device::DeviceErrorAsync;
        return result;
    }

    if (itemId.startsWith("file:")) {
        QString idString = itemId;
        idString.remove(QRegExp("^file:"));
        QVariantMap params;
        params.insert("directory", idString);
        params.insert("properties", properties);
        qCDebug(dcKodi()) << "Sending" << params;
        int id = m_jsonHandler->sendData("Files.GetDirectory", params);
        m_pendingBrowseRequests.insert(id, result);
        result.status = Device::DeviceErrorAsync;
        return result;
    }

    result.status = Device::DeviceErrorItemNotFound;
    return result;
}

Device::BrowserItemResult Kodi::browserItem(const QString &itemId, Device::BrowserItemResult &result)
{
    qCDebug(dcKodi()) << "Getting details for" << itemId;
    QString idString = itemId;
    QString method;
    QVariantMap params;
    if (idString.startsWith("song:")) {
        idString.remove(QRegExp("^song:"));
        params.insert("songid", idString.toInt());
        method = "AudioLibrary.GetSongDetails";
    } else if (idString.startsWith("movie:")) {
        idString.remove(QRegExp("^movie:"));
        params.insert("movieid", idString.toInt());
        method = "VideoLibrary.GetMovieDetails";
    } else if (idString.startsWith("episode:")) {
        idString.remove(QRegExp("^episode:"));
        params.insert("episodeid", idString.toInt());
        method = "VideoLibrary.GetEpisodeDetails";
    } else if (idString.startsWith("musicvideo:")) {
        idString.remove(QRegExp("^musicvideo:"));
        params.insert("musicvideoid", idString.toInt());
        method = "VideoLibrary.GetMusicVideoDetails";
    } else {
        qCWarning(dcKodi()) << "Unhandled browserItem request!" << itemId;
        result.status = Device::DeviceErrorUnsupportedFeature;
        return result;
    }
    int id = m_jsonHandler->sendData(method, params);
    m_pendingBrowserItemRequests.insert(id, result);
    result.status = Device::DeviceErrorAsync;
    return result;
}

Device::DeviceError Kodi::launchBrowserItem(const QString &itemId)
{
    qCDebug(dcKodi()) << "Launching" << itemId;
    QVariantMap playlistItem;

    QString idString = itemId;
    if (idString.startsWith("song:")) {
        idString.remove(QRegExp("^song:"));
        int idx = idString.indexOf(",album:");
        if (idx > 0) {
            int position = idString.left(idx).toInt();
            idString.remove(QRegExp("^[0-9]*,album:"));
            int albumId = idString.toInt();

            QVariantMap params;
            params.insert("playlistid", 0);
            m_jsonHandler->sendData("Playlist.Clear", params);

            QVariantMap item;
            item.insert("albumid", albumId);
            params.insert("item", item);
            m_jsonHandler->sendData("Playlist.Add", params);

            playlistItem.insert("playlistid", 0);
            playlistItem.insert("position", position);

        } else {
            playlistItem.insert("songid", idString.toInt());
        }
    } else if (idString.startsWith("movie:")) {
        idString.remove(QRegExp("^movie:"));
        playlistItem.insert("movieid", idString.toInt());
    } else if (idString.startsWith("episode:")) {
        idString.remove(QRegExp("^episode:"));
        playlistItem.insert("episodeid", idString.toInt());
    } else if (idString.startsWith("file:")) {
        idString.remove(QRegExp("^file:"));
        playlistItem.insert("file", idString);
    } else {
        qCWarning(dcKodi()) << "Unhandled launchBrowserItem request!" << itemId;
        return Device::DeviceErrorItemNotFound;
    }
    QVariantMap params;
    params.clear();
    params.insert("item", playlistItem);

    qCDebug(dcKodi()) << "Player.Open" << params;
    m_jsonHandler->sendData("Player.Open", params);
    return Device::DeviceErrorNoError;
}

void Kodi::onVolumeChanged(const int &volume, const bool &muted)
{
    if (m_volume != volume || m_muted != muted) {
        m_volume = volume;
        m_muted = muted;
        emit stateChanged();
    }
}

void Kodi::onUpdateFinished(const QVariantMap &data)
{
    qCDebug(dcKodi()) << "update finished:" << data;
    if (data.contains("volume")) {
        m_volume = data.value("volume").toInt();
    }
    if (data.contains("muted")) {
        m_muted = data.value("muted").toBool();
    }
    emit stateChanged();
}

void Kodi::activePlayersChanged(const QVariantList &data)
{
    qCDebug(dcKodi()) << "active players changed" << data.count() << data;
    m_activePlayerCount = data.count();
    if (m_activePlayerCount == 0) {
        onPlaybackStatusChanged("Stopped");
        return;
    }
    m_activePlayer = data.first().toMap().value("playerid").toInt();
    qCDebug(dcKodi) << "Active Player changed:" << m_activePlayer << data.first().toMap().value("type").toString();
    emit activePlayerChanged(data.first().toMap().value("type").toString());

    updatePlayerProperties();
}

void Kodi::playerPropertiesReceived(const QVariantMap &properties)
{
    qCDebug(dcKodi()) << "player props received" << properties;

    if (m_activePlayerCount > 0) {
        if (properties.value("speed").toDouble() > 0) {
            onPlaybackStatusChanged("Playing");
        } else {
            onPlaybackStatusChanged("Paused");
        }
    }

    emit shuffleChanged(properties.value("shuffled").toBool());
    emit repeatChanged(properties.value("repeat").toString());

}

void Kodi::mediaMetaDataReceived(const QVariantMap &data)
{
    QVariantMap item = data.value("item").toMap();

    QString title = item.value("title").toString();
    QString artist;
    QString collection;
    if (item.value("type").toString() == "song") {
        artist = !item.value("artist").toList().isEmpty() ? item.value("artist").toList().first().toString() : "";
        collection = item.value("album").toString();
    } else if (item.value("type").toString() == "episode") {
        collection = item.value("showtitle").toString();
    } else if (item.value("type").toString() == "unknown") {
        artist = item.value("channel").toString();
        collection = item.value("showtitle").toString();
    } else if (item.value("type").toString() == "channel") {
        artist = item.value("channel").toString();
        collection = item.value("showtitle").toString();
    } else if (item.value("type").toString() == "movie") {
        artist = item.value("director").toStringList().join(", ");
        collection = item.value("year").toString();
    }

    QString artwork = item.value("thumbnail").toString();
    if (artwork.isEmpty()) {
        artwork = item.value("fanart").toString();
    }
    qCDebug(dcKodi) << "title:" << title << artwork;
    emit mediaMetadataChanged(title, artist, collection, artwork);
}

void Kodi::onPlaybackStatusChanged(const QString &playbackState)
{
    if (playbackState != "Stopped") {
        updateMetadata();
    } else {
        emit mediaMetadataChanged(QString(), QString(), QString(), QString());
    }
    emit playbackStatusChanged(playbackState);
}

void Kodi::processNotification(const QString &method, const QVariantMap &params)
{
    qCDebug(dcKodi) << "got notification" << method << params;

    if (method == "Application.OnVolumeChanged") {
        QVariantMap data = params.value("data").toMap();
        onVolumeChanged(data.value("volume").toInt(), data.value("muted").toBool());
    } else if (method == "Player.OnPlay" || method == "Player.OnResume") {
        emit activePlayersChanged(QVariantList() << params.value("data").toMap().value("player"));
        onPlaybackStatusChanged("Playing");
    } else if (method == "Player.OnPause") {
        emit playbackStatusChanged("Paused");
    } else if (method == "Player.OnStop") {
        emit playbackStatusChanged("Stopped");
        emit activePlayersChanged(QVariantList());
    }
}

void Kodi::processResponse(int id, const QString &method, const QVariantMap &response)
{

    qCDebug(dcKodi) << "response received:" << method << response;

    if (response.contains("error")) {
        //qCDebug(dcKodi) << QJsonDocument::fromVariant(response).toJson();
        qCWarning(dcKodi) << "got error response for request " << method << ":" << response.value("error").toMap().value("message").toString();
        emit actionExecuted(id, false);
        return;
    }

    if (method == "Application.GetProperties") {
        //qCDebug(dcKodi) << "got update response" << reply.method();
        emit updateDataReceived(response.value("result").toMap());
        return;
    }

    if (method == "JSONRPC.Version") {
        qCDebug(dcKodi) << "got version response" << method;
        emit versionDataReceived(response.value("result").toMap());
        return;
    }

    if (method == "Player.GetActivePlayers") {
        qCDebug(dcKodi) << "Active players changed" << response;
        emit activePlayersChanged(response.value("result").toList());
        return;
    }

    if (method == "Player.GetProperties") {
        qCDebug(dcKodi) << "Player properties received" << response;
        playerPropertiesReceived(response.value("result").toMap());
        return;
    }

    if (method == "Player.GetItem") {
        qCDebug(dcKodi) << "Played item received" << response;
        emit mediaMetaDataReceived(response.value("result").toMap());
        return;
    }

    if (method == "Player.SetShuffle" || method == "Player.SetRepeat") {
        updatePlayerProperties();
        emit actionExecuted(id, true);
        return;
    }

    if (method == "AudioLibrary.GetArtists") {
        Device::BrowseResult result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &artistVariant, response.value("result").toMap().value("artists").toList()) {
            QVariantMap artist = artistVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << artist;
            BrowserItem item("artist:" + artist.value("artistid").toString(), artist.value("label").toString());
            item.setBrowsable(true);
            item.setIcon(BrowserItem::BrowserIconFolder);
            item.setThumbnail(prepareThumbnail(artist.value("thumbnail").toString()));
            qCDebug(dcKodi()) << "Thumbnail" << item.thumbnail();
            result.items.append(item);
        }
        emit browseResult(result);
        return;
    }

    if (method == "AudioLibrary.GetAlbums") {
        Device::BrowseResult result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &albumVariant, response.value("result").toMap().value("albums").toList()) {
            QVariantMap album = albumVariant.toMap();
            BrowserItem item("album:" + album.value("albumid").toString(), album.value("label").toString());
            item.setBrowsable(true);
            item.setIcon(BrowserItem::BrowserIconFolder);
            item.setThumbnail(prepareThumbnail(album.value("thumbnail").toString()));
            result.items.append(item);
        }
        emit browseResult(result);
        return;
    }

    if (method == "AudioLibrary.GetSongs") {
        Device::BrowseResult result = m_pendingBrowseRequests.take(id);
        int i = 0;
        foreach (const QVariant &songVariant, response.value("result").toMap().value("songs").toList()) {
            QVariantMap song = songVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << song;
            QString newId = "song:";
            if (song.contains("albumid")) {
                newId += QString::number(i);
                newId += ",album:" + song.value("albumid").toString();
            } else {
                newId += song.value("songid").toString();
            }
            BrowserItem item(newId, song.value("label").toString());
            item.setExecutable(true);
            item.setIcon(BrowserItem::BrowserIconMusic);
            item.setThumbnail(prepareThumbnail(song.value("thumbnail").toString()));
            result.items.append(item);
            i++;
        }
        emit browseResult(result);
        return;
    }


    if (method == "VideoLibrary.GetMovies") {
        Device::BrowseResult result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &movieVariant, response.value("result").toMap().value("movies").toList()) {
            QVariantMap movie = movieVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << movie;
            BrowserItem item("movie:" + movie.value("movieid").toString(), movie.value("label").toString());
            item.setExecutable(true);
            item.setIcon(BrowserItem::BrowserIconVideo);
            item.setThumbnail(prepareThumbnail(movie.value("thumbnail").toString()));
            result.items.append(item);
        }
        emit browseResult(result);
        return;
    }

    if (method == "VideoLibrary.GetTVShows") {
        Device::BrowseResult result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &tvShowVariant, response.value("result").toMap().value("tvshows").toList()) {
            QVariantMap tvShow = tvShowVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << tvShow;
            BrowserItem item("tvshow:" + tvShow.value("tvshowid").toString(), tvShow.value("label").toString());
            item.setBrowsable(true);
            item.setIcon(BrowserItem::BrowserIconFolder);
            item.setThumbnail(prepareThumbnail(tvShow.value("thumbnail").toString()));
            result.items.append(item);
        }
        emit browseResult(result);
        return;
    }

    if (method == "VideoLibrary.GetSeasons") {
        Device::BrowseResult result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &seasonVariant, response.value("result").toMap().value("seasons").toList()) {
            QVariantMap season = seasonVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << season;
            BrowserItem item("season:" + season.value("season").toString() + ",tvshow:" + season.value("tvshowid").toString(), season.value("label").toString());
            item.setBrowsable(true);
            item.setIcon(BrowserItem::BrowserIconFolder);
            item.setThumbnail(prepareThumbnail(season.value("thumbnail").toString()));
            result.items.append(item);
        }
        emit browseResult(result);
        return;
    }

    if (method == "VideoLibrary.GetEpisodes") {
        Device::BrowseResult result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &episodeVariant, response.value("result").toMap().value("episodes").toList()) {
            QVariantMap episode = episodeVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << episode;
            BrowserItem item("episode:" + episode.value("episodeid").toString(), episode.value("label").toString());
            item.setExecutable(true);
            item.setIcon(BrowserItem::BrowserIconVideo);
            item.setThumbnail(prepareThumbnail(episode.value("thumbnail").toString()));
            result.items.append(item);
        }
        emit browseResult(result);
        return;
    }

    if (method == "VideoLibrary.GetMusicVideos") {
        Device::BrowseResult result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &musicVideoVariant, response.value("result").toMap().value("musicvideos").toList()) {
            QVariantMap musicVideo = musicVideoVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << musicVideo;
            BrowserItem item("musicvideo:" + musicVideo.value("musicvideoid").toString(), musicVideo.value("label").toString());
            item.setExecutable(true);
            item.setIcon(BrowserItem::BrowserIconVideo);
            item.setThumbnail(prepareThumbnail(musicVideo.value("thumbnail").toString()));
            result.items.append(item);
        }
        emit browseResult(result);
        return;
    }

    if (method == "Addons.GetAddons") {
        Device::BrowseResult result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &addonVariant, response.value("result").toMap().value("addons").toList()) {
            QVariantMap addon = addonVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << addon;
            BrowserItem item("addon:" + addon.value("addonid").toString(), addon.value("name").toString());
            item.setBrowsable(true);
            item.setIcon(BrowserItem::BrowserIconApplication);
            result.items.append(item);
        }
        emit browseResult(result);
        return;
    }

    if (method == "Files.GetDirectory") {
        Device::BrowseResult result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &fileVariant, response.value("result").toMap().value("files").toList()) {
            QVariantMap file = fileVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << file;
            BrowserItem item("file:" + file.value("file").toString(), file.value("label").toString());
            if (file.value("type").toString() == "directory" || file.value("type").toString() == "unknown") {
                item.setBrowsable(true);
                item.setIcon(BrowserItem::BrowserIconFolder);
            } else if (file.value("type").toString() == "episode" || file.value("type").toString() == "movie") {
                item.setExecutable(true);
                item.setIcon(BrowserItem::BrowserIconVideo);
            } else if (file.value("type").toString() == "song") {
                item.setExecutable(true);
                item.setIcon(BrowserItem::BrowserIconMusic);
            }
            result.items.append(item);
        }
        emit browseResult(result);
        return;
    }

    if (method == "AudioLibrary.GetSongDetails") {
        Device::BrowserItemResult result = m_pendingBrowserItemRequests.take(id);
        result.item.setId("song:" + response.value("result").toMap().value("songdetails").toMap().value("songid").toString());
        result.item.setDisplayName(response.value("result").toMap().value("songdetails").toMap().value("label").toString());
        qCDebug(dcKodi()) << "Song details:" << result.item.displayName();
        emit browserItemResult(result);
        return;
    }

    if (method == "VideoLibrary.GetMovieDetails") {
        Device::BrowserItemResult result = m_pendingBrowserItemRequests.take(id);
        result.item.setId("movie:" + response.value("result").toMap().value("moviedetails").toMap().value("movieid").toString());
        result.item.setDisplayName(response.value("result").toMap().value("moviedetails").toMap().value("label").toString());
        qCDebug(dcKodi()) << "Movie details:" << result.item.displayName();
        emit browserItemResult(result);
        return;
    }

    if (method == "VideoLibrary.GetEpisodeDetails") {
        Device::BrowserItemResult result = m_pendingBrowserItemRequests.take(id);
        result.item.setId("movie:" + response.value("result").toMap().value("episodedetails").toMap().value("episodeid").toString());
        result.item.setDisplayName(response.value("result").toMap().value("episodedetails").toMap().value("label").toString());
        qCDebug(dcKodi()) << "Episode details:" << result.item.displayName();
        emit browserItemResult(result);
        return;
    }

    if (method == "VideoLibrary.GetMusicVideoDetails") {
        Device::BrowserItemResult result = m_pendingBrowserItemRequests.take(id);
        result.item.setId("movie:" + response.value("result").toMap().value("musicvideodetails").toMap().value("musicvideoid").toString());
        result.item.setDisplayName(response.value("result").toMap().value("musicvideodetails").toMap().value("label").toString());
        qCDebug(dcKodi()) << "Episode details:" << result.item.displayName();
        emit browserItemResult(result);
        return;
    }

    qCWarning(dcKodi()) << "unhandled reply" << method << response;
}

void Kodi::updatePlayerProperties()
{
    QVariantMap params;
    params.insert("playerid", m_activePlayer);
    QVariantList properties;
    properties << "speed" << "shuffled" << "repeat";
    params.insert("properties", properties);
    m_jsonHandler->sendData("Player.GetProperties", params);
}

void Kodi::updateMetadata()
{
    QVariantMap params;
    params.insert("playerid", m_activePlayer);
    QVariantList fields;
    fields << "title" << "artist" << "album" << "director" << "thumbnail" << "showtitle" << "fanart" << "channel" << "year";
    params.insert("properties", fields);
    m_jsonHandler->sendData("Player.GetItem", params);
}

QString Kodi::prepareThumbnail(const QString &thumbnail)
{
    if (thumbnail.isEmpty()) {
        return QString();
    }

    return QString("http://%1:%2/image/%3")
                .arg(m_connection->hostAddress().toString())
                .arg(m_httpPort)
                .arg(QString(thumbnail.toUtf8().toPercentEncoding()));
}
