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

#include "kodi.h"
#include <QDebug>
#include "extern-plugininfo.h"
#include <QUrl>
#include <QTime>

Kodi::Kodi(const QHostAddress &hostAddress, int port, int httpPort, QObject *parent) :
    QObject(parent),
    m_httpPort(httpPort),
    m_muted(false),
    m_volume(-1)
{
    m_connection = new KodiConnection(hostAddress, port, this);
    connect (m_connection, &KodiConnection::connectionStatusChanged, this, &Kodi::onConnectionStatusChanged);

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

    // Video
    BrowserItem item = BrowserItem("videolibrary", tr("Video library"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    item.setActionTypeIds({kodiUpdateLibraryBrowserItemActionTypeId, kodiCleanLibraryBrowserItemActionTypeId});
    VirtualFsNode *videoLibrary = new VirtualFsNode(item);
    m_virtualFs->addChild(videoLibrary);

    item = BrowserItem("movies", tr("Movies"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *movies = new VirtualFsNode(item);
    movies->getMethod = "VideoLibrary.GetMovies";
    movies->getParams.insert("sort", sort);
    QVariantList movieProperties = properties;
    movieProperties.append("year");
    movieProperties.append("rating");
    movieProperties.append("runtime");
    movies->getParams.insert("properties", movieProperties);
    videoLibrary->addChild(movies);

    item = BrowserItem("tvshows", tr("TV Shows"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *tvShows = new VirtualFsNode(item);
    tvShows->getMethod = "VideoLibrary.GetTVShows";
    tvShows->getParams.insert("sort", sort);
    QVariantList tvShowProperties = properties;
    tvShowProperties.append("year");
    tvShowProperties.append("rating");
    tvShowProperties.append("season");
    tvShows->getParams.insert("properties", tvShowProperties);
    videoLibrary->addChild(tvShows);

    item = BrowserItem("musicvideos", tr("Music Videos"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *musicVideos = new VirtualFsNode(item);
    musicVideos->getMethod = "VideoLibrary.GetMusicVideos";
    musicVideos->getParams.insert("sort", sort);
    musicVideos->getParams.insert("properties", properties);
    videoLibrary->addChild(musicVideos);

    // Music
    item = BrowserItem("audiolibrary", tr("Music library"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    item.setActionTypeIds({kodiUpdateLibraryBrowserItemActionTypeId, kodiCleanLibraryBrowserItemActionTypeId});
    VirtualFsNode *audioLibrary = new VirtualFsNode(item);
    m_virtualFs->addChild(audioLibrary);

    item = BrowserItem("artists", tr("Artists"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *artists = new VirtualFsNode(item);
    artists->getMethod = "AudioLibrary.GetArtists";
    artists->getParams.insert("sort", sort);
    QVariantList artistProperties = properties;
    artistProperties.append("formed");
    artistProperties.append("genre");
    artists->getParams.insert("properties", artistProperties);
    audioLibrary->addChild(artists);

    item = BrowserItem("albums", tr("Albums"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *albums = new VirtualFsNode(item);
    albums->getMethod = "AudioLibrary.GetAlbums";
    albums->getParams.insert("sort", sort);
    QVariantList albumProperties = properties;
    albumProperties.append("artist");
    albumProperties.append("year");
    albums->getParams.insert("properties", albumProperties);
    audioLibrary->addChild(albums);

    item = BrowserItem("songs", tr("Songs"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconFolder);
    VirtualFsNode *songs = new VirtualFsNode(item);
    songs->getMethod = "AudioLibrary.GetSongs";
    songs->getParams.insert("sort", sort);
    QVariantList songProperties = properties;
    songProperties.append("artist");
    songProperties.append("album");
    songProperties.append("year");
    songs->getParams.insert("properties", songProperties);
    audioLibrary->addChild(songs);

    // Add-ons
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

Kodi::~Kodi()
{
    delete m_virtualFs;
}

QHostAddress Kodi::hostAddress() const
{
    return m_connection->hostAddress();
}

void Kodi::setHostAddress(const QHostAddress &address)
{
    m_connection->setHostAddress(address);
}

uint Kodi::port() const
{
    return m_connection->port();
}

void Kodi::setPort(uint port)
{
    m_connection->setPort(port);
}

uint Kodi::httpPort() const
{
    return m_httpPort;
}

void Kodi::setHttpPort(uint httpPort)
{
    m_httpPort = httpPort;
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

int Kodi::showNotification(const QString &title, const QString &message, const int &displayTime, const QString &image)
{
    QVariantMap params;
    params.insert("title", title);
    params.insert("message", message);
    params.insert("displaytime", displayTime);
    params.insert("image", image);

    return m_jsonHandler->sendData("GUI.ShowNotification", params);
}

int Kodi::navigate(const QString &to)
{
    qCDebug(dcKodi()) << "Navigate:" << to;
    if (to == "home") {
        return m_jsonHandler->sendData("Input.Home", QVariantMap());
    }

    QVariantMap params;
    QString mappedTo = to;
    if (to == "enter") {
        mappedTo = "select";
    }
    params.insert("action", mappedTo);
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

void Kodi::browse(BrowseResult *result)
{
//    m_jsonHandler->sendData()
    VirtualFsNode *node = m_virtualFs->findNode(result->itemId());

    if (node) {
        if (node->getMethod.isEmpty()) {
            foreach (VirtualFsNode *child, node->childs) {
                result->addItem(child->item);
            }
            result->finish(Thing::ThingErrorNoError);
            return;
        }

        qCDebug(dcKodi()) << "Sending:" << node->getMethod << node->getParams;
        int id = m_jsonHandler->sendData(node->getMethod, node->getParams);
        m_pendingBrowseRequests.insert(id, result);
        return;
    }

    QVariantMap sort;
    sort.insert("method", "label");
    sort.insert("ignorearticle", true);
    QVariantList properties;
    properties.append("thumbnail");

    if (result->itemId().startsWith("artist:")) {
        QString idString = result->itemId();
        idString.remove(QRegExp("^artist:"));
        QVariantMap filter;
        filter.insert("artistid", idString.toInt());
        QVariantMap params;
        params.insert("filter", filter);
        QVariantList albumProperties = properties;
        albumProperties.append("artist");
        albumProperties.append("year");
        params.insert("properties", albumProperties);
        int id = m_jsonHandler->sendData("AudioLibrary.GetAlbums", params);
        m_pendingBrowseRequests.insert(id, result);
        return;
    }

    if (result->itemId().startsWith("album:")) {
        QString idString = result->itemId();
        idString.remove(QRegExp("^album:"));
        QVariantMap filter;
        filter.insert("albumid", idString.toInt());
        QVariantMap params;
        params.insert("filter", filter);
        QVariantList songProperties = properties;
        songProperties.append("albumid");
        songProperties.append("artist");
        songProperties.append("album");
        songProperties.append("year");
        params.insert("properties", songProperties);
        int id = m_jsonHandler->sendData("AudioLibrary.GetSongs", params);
        m_pendingBrowseRequests.insert(id, result);
        return;
    }

    if (result->itemId().startsWith("tvshow:")) {
        QString idString = result->itemId();
        idString.remove(QRegExp("^tvshow:"));
        QVariantMap params;
        params.insert("tvshowid", idString.toInt());
        QVariantList properties;
        properties.append("tvshowid");
        properties.append("season");
        properties.append("thumbnail");
        properties.append("showtitle");
        params.insert("properties", properties);
        int id = m_jsonHandler->sendData("VideoLibrary.GetSeasons", params);
        m_pendingBrowseRequests.insert(id, result);
        return;
    }

    if (result->itemId().startsWith("season:")) {
        QString idString = result->itemId();
        idString.remove(QRegExp("^season:"));
        int seasonId = idString.left(idString.indexOf(",")).toInt();
        idString.remove(QRegExp("^[0-9]*,tvshow:"));
        int tvShowId = idString.toInt();
        QVariantMap params;
        params.insert("tvshowid", tvShowId);
        params.insert("season", seasonId);
        QVariantList properties;
        properties.append("thumbnail");
        properties.append("showtitle");
        properties.append("season");
        params.insert("properties", properties);
        qCDebug(dcKodi()) << "getting episodes:" << params;
        int id = m_jsonHandler->sendData("VideoLibrary.GetEpisodes", params);
        m_pendingBrowseRequests.insert(id, result);
        return;
    }

    if (result->itemId().startsWith("addon:")) {
        QString idString = result->itemId();
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
        return;
    }

    if (result->itemId().startsWith("file:")) {
        QString idString = result->itemId();
        idString.remove(QRegExp("^file:"));
        QVariantMap params;
        params.insert("directory", idString);
        params.insert("properties", properties);
        qCDebug(dcKodi()) << "Sending" << params;
        int id = m_jsonHandler->sendData("Files.GetDirectory", params);
        m_pendingBrowseRequests.insert(id, result);
        return;
    }

    result->finish(Thing::ThingErrorItemNotFound);
}

void Kodi::browserItem(BrowserItemResult *result)
{
    QString itemId = result->itemId();
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
        result->finish(Thing::ThingErrorItemNotFound);
        return;
    }
    int id = m_jsonHandler->sendData(method, params);
    m_pendingBrowserItemRequests.insert(id, result);
}

int Kodi::launchBrowserItem(const QString &itemId)
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
        return -1;
    }
    QVariantMap params;
    params.clear();
    params.insert("item", playlistItem);

    qCDebug(dcKodi()) << "Player.Open" << params;
    return m_jsonHandler->sendData("Player.Open", params);
}

int Kodi::executeBrowserItemAction(const QString &itemId, const ActionTypeId &actionTypeId)
{
    QString scope;
    QString method;
    if (actionTypeId == kodiUpdateLibraryBrowserItemActionTypeId) {
        method = "Scan";
    } else if (actionTypeId == kodiCleanLibraryBrowserItemActionTypeId) {
        method = "Clean";
    } else {
        return -1;
    }

    if (itemId == "audiolibrary") {
        scope = "AudioLibrary";
    } else if (itemId == "videolibrary") {
        scope = "VideoLibrary";
    } else {
        return -1;
    }

    return m_jsonHandler->sendData(scope + "." + method, QVariantMap());
}

void Kodi::onConnectionStatusChanged()
{
    if (m_connection->connected()) {
        checkVersion();
    } else {
        emit connectionStatusChanged(false);
    }
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

    updateMetadata();
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
    if (title.isEmpty()) { // Fall back to label if not title present
        title = item.value("label").toString();
    }
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
        return;
    }

    if (method == "Player.OnPlay" ||
            method == "Player.OnResume" ||
            method == "Player.OnPause" ||
            method == "Player.OnStop" ||
            method == "Player.OnAVChange") {
        update();
    }
}

void Kodi::processResponse(int id, const QString &method, const QVariantMap &response)
{

    qCDebug(dcKodi) << "response received:" << method << response;

    if (response.contains("error")) {
        qCWarning(dcKodi) << "got error response for request " << method << ":" << response.value("error").toMap().value("message").toString();
    }

    if (method == "JSONRPC.Version") {
        qCDebug(dcKodi) << "got version response" << method;
        QVariantMap data = response.value("result").toMap();
        QVariantMap version = data.value("version").toMap();
        QString apiVersion = QString("%1.%2.%3").arg(version.value("major").toString()).arg(version.value("minor").toString()).arg(version.value("patch").toString());
        qCDebug(dcKodi) << "API Version:" << apiVersion;

        if (version.value("major").toInt() < 6) {
            qCWarning(dcKodi) << "incompatible api version:" << apiVersion;
            m_connection->disconnectKodi();
            emit connectionStatusChanged(false);
            return;
        }
        emit connectionStatusChanged(true);

        update();
        return;
    }

    if (method == "Application.GetProperties") {
        //qCDebug(dcKodi) << "got update response" << reply.method();
        emit updateDataReceived(response.value("result").toMap());
        return;
    }


    if (method == "Player.GetActivePlayers") {
        qCDebug(dcKodi) << "Active players changed" << response;
        activePlayersChanged(response.value("result").toList());
        updatePlayerProperties();
        return;
    }

    if (method == "Player.GetProperties") {
        qCDebug(dcKodi) << "Player properties received" << response;
        playerPropertiesReceived(response.value("result").toMap());
        updateMetadata();
        return;
    }

    if (method == "Player.GetItem") {
        qCDebug(dcKodi) << "Played item received" << response;
        mediaMetaDataReceived(response.value("result").toMap());
        return;
    }

    if (method == "Player.SetShuffle" || method == "Player.SetRepeat") {
        updatePlayerProperties();
        emit actionExecuted(id, !response.contains("error"));
        return;
    }

    if (method == "AudioLibrary.GetArtists") {
        BrowseResult *result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &artistVariant, response.value("result").toMap().value("artists").toList()) {
            QVariantMap artist = artistVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << artist;
            BrowserItem item("artist:" + artist.value("artistid").toString(), artist.value("label").toString());
            item.setBrowsable(true);
            item.setIcon(BrowserItem::BrowserIconFolder);
            item.setThumbnail(prepareThumbnail(artist.value("thumbnail").toString()));
            QStringList description;
            if (!artist.value("formed").toString().isEmpty()) {
                description.append(artist.value("formed").toString());
            }
            if (!artist.value("genre").toStringList().isEmpty()) {
                description.append(artist.value("genre").toStringList().join(", "));
            }
            item.setDescription(description.join(" - "));
            qCDebug(dcKodi()) << "Thumbnail" << item.thumbnail();
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
        return;
    }

    if (method == "AudioLibrary.GetAlbums") {
        BrowseResult *result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &albumVariant, response.value("result").toMap().value("albums").toList()) {
            QVariantMap album = albumVariant.toMap();
            BrowserItem item("album:" + album.value("albumid").toString(), album.value("label").toString());
            item.setBrowsable(true);
            item.setIcon(BrowserItem::BrowserIconFolder);
            item.setThumbnail(prepareThumbnail(album.value("thumbnail").toString()));
            QStringList description;
            if (!album.value("artist").toStringList().isEmpty()) {
                description.append(album.value("artist").toStringList().join(", "));
            }
            if (album.value("year").toInt() != 0) {
                description.append(album.value("year").toString());
            }
            item.setDescription(description.join(" - "));
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
        return;
    }

    if (method == "AudioLibrary.GetSongs") {
        BrowseResult *result = m_pendingBrowseRequests.take(id);
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
            QStringList description;
            if (!song.value("artist").toStringList().isEmpty()) {
                description.append(song.value("artist").toStringList().join(","));
            }
            if (!song.value("album").toString().isEmpty()) {
                description.append(song.value("album").toString());
            } else if (!song.value("year").toString().isEmpty()) {
                description.append(song.value("year").toString());
            }
            item.setDescription(description.join(" - "));
            result->addItem(item);
            i++;
        }
        result->finish(Thing::ThingErrorNoError);
        return;
    }


    if (method == "VideoLibrary.GetMovies") {
        BrowseResult *result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &movieVariant, response.value("result").toMap().value("movies").toList()) {
            QVariantMap movie = movieVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << movie;
            BrowserItem item("movie:" + movie.value("movieid").toString(), movie.value("label").toString());
            item.setExecutable(true);
            item.setIcon(BrowserItem::BrowserIconVideo);
            item.setThumbnail(prepareThumbnail(movie.value("thumbnail").toString()));
            QString rating;
            for (int i = 0; i < 5; i++) {
                if (qRound(movie.value("rating").toDouble() / 2) >= i) {
                    rating += "★";
                } else {
                    rating += "☆";
                }
            }

            int runtime = movie.value("runtime").toInt();
            int hours = runtime / 60 / 60;
            int minutes = (runtime / 60) % 60;
            QString duration;
            duration = QString("%1:%2").arg(hours).arg(minutes, 2, 10, QChar('0'));
            item.setDescription(movie.value("year").toString() + " - " + duration + " - " + rating);
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
        return;
    }

    if (method == "VideoLibrary.GetTVShows") {
        BrowseResult *result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &tvShowVariant, response.value("result").toMap().value("tvshows").toList()) {
            QVariantMap tvShow = tvShowVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << tvShow;
            BrowserItem item("tvshow:" + tvShow.value("tvshowid").toString(), tvShow.value("label").toString());
            item.setBrowsable(true);
            item.setIcon(BrowserItem::BrowserIconFolder);
            item.setThumbnail(prepareThumbnail(tvShow.value("thumbnail").toString()));
            QString rating;
            for (int i = 0; i < 5; i++) {
                if (qRound(tvShow.value("rating").toDouble() / 2) >= i) {
                    rating += "★";
                } else {
                    rating += "☆";
                }
            }
            item.setDescription(tvShow.value("year").toString() + " - " + tr("%1 seasons").arg(tvShow.value("season").toInt()) + " - " + rating);
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
        return;
    }

    if (method == "VideoLibrary.GetSeasons") {
        BrowseResult *result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &seasonVariant, response.value("result").toMap().value("seasons").toList()) {
            QVariantMap season = seasonVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << season;
            BrowserItem item("season:" + season.value("season").toString() + ",tvshow:" + season.value("tvshowid").toString(), season.value("label").toString());
            item.setBrowsable(true);
            item.setIcon(BrowserItem::BrowserIconFolder);
            item.setThumbnail(prepareThumbnail(season.value("thumbnail").toString()));
            item.setDescription(season.value("showtitle").toString());
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
        return;
    }

    if (method == "VideoLibrary.GetEpisodes") {
        BrowseResult *result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &episodeVariant, response.value("result").toMap().value("episodes").toList()) {
            QVariantMap episode = episodeVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << episode;
            BrowserItem item("episode:" + episode.value("episodeid").toString(), episode.value("label").toString());
            item.setExecutable(true);
            item.setIcon(BrowserItem::BrowserIconVideo);
            item.setThumbnail(prepareThumbnail(episode.value("thumbnail").toString()));
            if (!episode.value("season").toString().isEmpty()) {
                item.setDescription(episode.value("showtitle").toString() + " - " + tr("Season %1").arg(episode.value("season").toString()));
            } else {
                item.setDescription(episode.value("showtitle").toString());
            }
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
        return;
    }

    if (method == "VideoLibrary.GetMusicVideos") {
        BrowseResult *result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &musicVideoVariant, response.value("result").toMap().value("musicvideos").toList()) {
            QVariantMap musicVideo = musicVideoVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << musicVideo;
            BrowserItem item("musicvideo:" + musicVideo.value("musicvideoid").toString(), musicVideo.value("label").toString());
            item.setExecutable(true);
            item.setIcon(BrowserItem::BrowserIconVideo);
            item.setThumbnail(prepareThumbnail(musicVideo.value("thumbnail").toString()));
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
        return;
    }

    if (method == "Addons.GetAddons") {
        BrowseResult *result = m_pendingBrowseRequests.take(id);
        foreach (const QVariant &addonVariant, response.value("result").toMap().value("addons").toList()) {
            QVariantMap addon = addonVariant.toMap();
            qCDebug(dcKodi()) << "Entry:" << addon;
            BrowserItem item("addon:" + addon.value("addonid").toString(), addon.value("name").toString());
            item.setBrowsable(true);
            item.setIcon(BrowserItem::BrowserIconApplication);
            item.setThumbnail(prepareThumbnail(addon.value("thumbnail").toString()));
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
        return;
    }

    if (method == "Files.GetDirectory") {
        BrowseResult *result = m_pendingBrowseRequests.take(id);
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
            item.setThumbnail(prepareThumbnail(file.value("thumbnail").toString()));
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
        return;
    }

    if (method == "AudioLibrary.GetSongDetails") {
        BrowserItemResult *result = m_pendingBrowserItemRequests.take(id);
        BrowserItem item("song:" + response.value("result").toMap().value("songdetails").toMap().value("songid").toString());
        item.setDisplayName(response.value("result").toMap().value("songdetails").toMap().value("label").toString());
        qCDebug(dcKodi()) << "Song details:" << item.displayName();
        result->finish(item);
        return;
    }

    if (method == "VideoLibrary.GetMovieDetails") {
        BrowserItemResult *result = m_pendingBrowserItemRequests.take(id);
        BrowserItem item("movie:" + response.value("result").toMap().value("moviedetails").toMap().value("movieid").toString());
        item.setDisplayName(response.value("result").toMap().value("moviedetails").toMap().value("label").toString());
        qCDebug(dcKodi()) << "Movie details:" << item.displayName();
        result->finish(item);
        return;
    }

    if (method == "VideoLibrary.GetEpisodeDetails") {
        BrowserItemResult *result = m_pendingBrowserItemRequests.take(id);
        BrowserItem item("movie:" + response.value("result").toMap().value("episodedetails").toMap().value("episodeid").toString());
        item.setDisplayName(response.value("result").toMap().value("episodedetails").toMap().value("label").toString());
        qCDebug(dcKodi()) << "Episode details:" << item.displayName();
        result->finish(item);
        return;
    }

    if (method == "VideoLibrary.GetMusicVideoDetails") {
        BrowserItemResult *result = m_pendingBrowserItemRequests.take(id);
        BrowserItem item("movie:" + response.value("result").toMap().value("musicvideodetails").toMap().value("musicvideoid").toString());
        item.setDisplayName(response.value("result").toMap().value("musicvideodetails").toMap().value("label").toString());
        qCDebug(dcKodi()) << "Episode details:" << item.displayName();
        result->finish(item);
        return;
    }

    if (method == "VideoLibrary.Scan" || method == "VideoLibrary.Clean" || method == "AudioLibrary.Scan" || method == "AudioLibrary.Clean") {
        emit browserItemActionExecuted(id, !response.contains("error"));
        return;
    }

    if (method == "Player.Open") {
        emit browserItemExecuted(id, !response.contains("error"));
        return;
    }

    // Default
    emit actionExecuted(id, !response.contains("error"));
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

    QString addr = m_connection->hostAddress().toString();
    if (m_connection->hostAddress().protocol() == QAbstractSocket::IPv6Protocol) {
        addr = '[' + addr + ']';
    }
    return QString("http://%1:%2/image/%3")
                .arg(addr)
                .arg(m_httpPort)
                .arg(QString(thumbnail.toUtf8().toPercentEncoding()));
}
