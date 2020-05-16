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

#include "streamunlimiteddevice.h"
#include "streamunlimitedrequest.h"
#include "extern-plugininfo.h"

#include <types/mediabrowseritem.h>
#include <network/networkaccessmanager.h>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QTimer>


StreamUnlimitedDevice::StreamUnlimitedDevice(NetworkAccessManager *nam, QObject *parent):
    QObject(parent),
    m_nam(nam)
{

}

void StreamUnlimitedDevice::setHost(const QHostAddress &address, int port)
{
    m_adddress = address;
    m_port = port;

    if (m_pollReply) {
        m_pollReply->abort();
    }

    qCDebug(dcStreamUnlimited()) << "Connecting to StreamUnlimited device at" << m_adddress;
    m_connectionStatus = ConnectionStatusConnecting;
    emit connectionStatusChanged(m_connectionStatus);

    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());
    url.setPort(port);
    url.setPath("/api/event/modifyQueue");
    QUrlQuery query;
    query.addQueryItem("queueId", "");
    QVariantList subscriptionList;
    QVariantMap subscriptionItem;
    subscriptionItem.insert("type", "item");
    subscriptionItem.insert("path", "settings:/mediaPlayer/playMode");
    subscriptionList.append(subscriptionItem);
    subscriptionItem.insert("path", "settings:/mediaPlayer/mute");
    subscriptionList.append(subscriptionItem);
    subscriptionItem.insert("path", "player:player/control");
    subscriptionList.append(subscriptionItem);
    subscriptionItem.insert("path", "player:player/data");
    subscriptionList.append(subscriptionItem);
    subscriptionItem.insert("path", "settings:/ui/language");
    subscriptionList.append(subscriptionItem);
    // Not needed
//    subscriptionItem.insert("path", "player:player/data/playTime");
//    subscriptionList.append(subscriptionItem);
    subscriptionItem.insert("path", "player:volume");
    subscriptionList.append(subscriptionItem);


    query.addQueryItem("subscribe", QJsonDocument::fromVariant(subscriptionList).toJson(QJsonDocument::Compact).toPercentEncoding());
    query.addQueryItem("unsubscribe", "[]");
    url.setQuery(query);

//    qCDebug(dcStreamUnlimited()) << "Setting up poll queue:" << url;
    QNetworkRequest request(url);
    request.setRawHeader("Connection", "keep-alive");
    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcStreamUnlimited()) << "Error connecting to SUE device:" << reply->errorString();
            m_connectionStatus = ConnectionStatusError;
            emit connectionStatusChanged(m_connectionStatus);

            QTimer::singleShot(10000, this, [this](){
                if (connectionStatus() != ConnectionStatusConnecting && connectionStatus() != ConnectionStatusConnected) {
                    setHost(m_adddress, m_port);
                }
            });
            return;
        }

        QByteArray data = reply->readAll();
        QByteArray idString = data.trimmed();
        idString.replace("\"", "");
        m_pollQueueId = QUuid(idString);
        qCDebug(dcStreamUnlimited()) << "Poll queue id:" << m_pollQueueId;
        if (m_pollQueueId.isNull()) {
            qCWarning(dcStreamUnlimited()) << "Error fetching poll queue id:" << data;
            m_connectionStatus = ConnectionStatusError;
            emit connectionStatusChanged(m_connectionStatus);
            return;
        }

        m_connectionStatus = ConnectionStatusConnected;
        emit connectionStatusChanged(m_connectionStatus);

        refreshMute();
        refreshVolume();
        refreshPlayerData();
        refreshPlayMode();
        refreshLanguage();

        pollQueue();
    });
}

QHostAddress StreamUnlimitedDevice::address() const
{
    return m_adddress;
}

int StreamUnlimitedDevice::port() const
{
    return m_port;
}

StreamUnlimitedDevice::ConnectionStatus StreamUnlimitedDevice::connectionStatus()
{
    return m_connectionStatus;
}

QLocale StreamUnlimitedDevice::language() const
{
    return m_language;
}

StreamUnlimitedDevice::PlayStatus StreamUnlimitedDevice::playbackStatus() const
{
    return m_playbackStatus;
}

uint StreamUnlimitedDevice::volume() const
{
    return m_volume;
}

int StreamUnlimitedDevice::setVolume(uint volume)
{
    int commandId = m_commandId++;

    QVariantMap params;
    params.insert("type", "i32_");
    params.insert("i32_", volume);

    StreamUnlimitedSetRequest *request = new StreamUnlimitedSetRequest(m_nam, m_adddress, m_port, "player:volume", "value", params, this);
    connect(request, &StreamUnlimitedSetRequest::error, this, [=](){
        emit commandCompleted(commandId, false);
    });
    connect(request, &StreamUnlimitedSetRequest::finished, this, [=](const QByteArray &data){
        emit commandCompleted(commandId, data == "true");
    });

    return commandId;
}

bool StreamUnlimitedDevice::mute() const
{
    return m_mute;
}

int StreamUnlimitedDevice::setMute(bool mute)
{
    int commandId = m_commandId++;

    QVariantMap params;
    params.insert("type", "bool_");
    params.insert("bool_", mute);

    StreamUnlimitedSetRequest *request = new StreamUnlimitedSetRequest(m_nam, m_adddress, m_port, "settings:/mediaPlayer/mute", "value", params, this);
    connect(request, &StreamUnlimitedSetRequest::error, this, [=](){
        emit commandCompleted(commandId, false);
    });
    connect(request, &StreamUnlimitedSetRequest::finished, this, [=](const QByteArray &data){
        emit commandCompleted(commandId, data == "true");
    });

    return commandId;
}

bool StreamUnlimitedDevice::shuffle() const
{
    return m_shuffle;
}

int StreamUnlimitedDevice::setShuffle(bool shuffle)
{
    int commandId = m_commandId++;

    StreamUnlimitedSetRequest *request = setPlayMode(shuffle, m_repeat);
    connect(request, &StreamUnlimitedSetRequest::error, this, [=](){
        emit commandCompleted(commandId, false);
    });
    connect(request, &StreamUnlimitedSetRequest::finished, this, [=](const QByteArray &data){
        emit commandCompleted(commandId, data == "true");
    });

    return commandId;
}

StreamUnlimitedDevice::Repeat StreamUnlimitedDevice::repeat() const
{
    return m_repeat;
}

int StreamUnlimitedDevice::setRepeat(StreamUnlimitedDevice::Repeat repeat)
{
    int commandId = m_commandId++;

    StreamUnlimitedSetRequest *request = setPlayMode(m_shuffle, repeat);
    connect(request, &StreamUnlimitedSetRequest::error, this, [=](){
        emit commandCompleted(commandId, false);
    });
    connect(request, &StreamUnlimitedSetRequest::finished, this, [=](const QByteArray &data){
        emit commandCompleted(commandId, data == "true");
    });

    return commandId;
}

QString StreamUnlimitedDevice::title() const
{
    return m_title;
}

QString StreamUnlimitedDevice::artist() const
{
    return m_artist;
}

QString StreamUnlimitedDevice::album() const
{
    return m_album;
}

QString StreamUnlimitedDevice::artwork() const
{
    return m_artwork;
}

int StreamUnlimitedDevice::play()
{
    if (m_playbackStatus == PlayStatusPaused) {
        return executeControlCommand("pause");
    }
    return executeControlCommand("play");
}

int StreamUnlimitedDevice::pause()
{
    return executeControlCommand("pause");
}

int StreamUnlimitedDevice::stop()
{
    return executeControlCommand("stop");
}

int StreamUnlimitedDevice::skipBack()
{
    return executeControlCommand("previous");
}

int StreamUnlimitedDevice::skipNext()
{
    return executeControlCommand("next");
}

int StreamUnlimitedDevice::browseDevice(const QString &itemId)
{
    return browseInternal(itemId);
}

int StreamUnlimitedDevice::playBrowserItem(const QString &itemId)
{
    QString path;
    QString value;

    if (itemId.startsWith("audio:")) {
        path = "player:player/control";
        value = itemId;
        value.remove(QRegExp("^audio:"));
    } else if (itemId.startsWith("action:")) {
        path = itemId;
        path.remove(QRegExp("^action:"));
        value = "true";
    }

    int commandId = m_commandId++;

    StreamUnlimitedSetRequest *request = new StreamUnlimitedSetRequest(m_nam, m_adddress, m_port, path, "activate", QJsonDocument::fromJson(value.toUtf8()).toVariant().toMap(), this);
    connect(request, &StreamUnlimitedSetRequest::error, this, [=](){
        emit commandCompleted(commandId, false);
    });
    connect(request, &StreamUnlimitedSetRequest::finished, this, [=](const QByteArray &data){
        qCDebug(dcStreamUnlimited()) << "Play browser item result:" << data;
        emit commandCompleted(commandId, data == "null");
    });


    return commandId;
}

int StreamUnlimitedDevice::browserItem(const QString &itemId)
{
    QString node = itemId;
    bool browsable = false;
    bool executable = false;

    if (node.startsWith("action:")) {
        node.remove(QRegExp("^action:"));
        executable = true;
    }

    int commandId = m_commandId++;

    StreamUnlimitedGetRequest *request = new StreamUnlimitedGetRequest(m_nam, m_adddress, m_port, itemId, {"title", "icon", "type", "description", "containerPlayable", "audioType", "context", "mediaData", "flags", "timestamp", "value"}, this);
    connect(request, &StreamUnlimitedGetRequest::error, this, [=](){
        emit browserItemResult(commandId, false);
    });
    connect(request, &StreamUnlimitedGetRequest::finished, this, [=](const QVariantMap &result){

        QString pathPrefix = "container:";
        QString title = result.value("title").toString();
        QString icon = result.value("icon").toString();
        QString type = result.value("type").toString();
        QString description = result.value("description").toString();
        QString containerPlayable = result.value("containerPlayable").toString();
        QString audioType = result.value("audioType").toString();
        QVariantMap context = result.value("context").toMap();
        QVariantMap mediaData = result.value("mediaData").toMap();
        QVariantMap flags = result.value("flags").toMap();

        BrowserItem item(itemId);
        item.setDisplayName(title);
        item.setDescription(description);
        item.setBrowsable(browsable);
        item.setExecutable(executable);

        emit browserItemResult(commandId, true, item);
    });

    return commandId;
}

int StreamUnlimitedDevice::setLocaleOnBoard(const QLocale &locale)
{
    int commandId = m_commandId++;

    QVariantMap localeParams;
    localeParams.insert("type", "string_");
    localeParams.insert("string_", locale.name());
    StreamUnlimitedSetRequest* request = new StreamUnlimitedSetRequest(m_nam, m_adddress, m_port, "settings:/ui/language", "value", localeParams, this);
    connect(request, &StreamUnlimitedSetRequest::error, this, [=]() {
        emit commandCompleted(commandId, false);
    });
    connect(request, &StreamUnlimitedSetRequest::finished, this, [=]() {
        emit commandCompleted(commandId, true);
    });
    return commandId;
}

int StreamUnlimitedDevice::storePreset(uint presetId)
{
    int commandId = m_commandId++;

    QVariantMap params;
    params.insert("type", "string_");
    params.insert("string_", QString::number(presetId));
    StreamUnlimitedSetRequest* request = new StreamUnlimitedSetRequest(m_nam, m_adddress, m_port, "googlecast:setPresetAction", "activate", params, this);
    connect(request, &StreamUnlimitedSetRequest::error, this, [=]() {
        emit commandCompleted(commandId, false);
    });
    connect(request, &StreamUnlimitedSetRequest::finished, this, [=](const QByteArray &response) {
        qCDebug(dcStreamUnlimited()) << "Store preset response" << response;
        emit commandCompleted(commandId, response == "null");
    });
    return commandId;
}

int StreamUnlimitedDevice::loadPreset(uint presetId)
{
    int commandId = m_commandId++;

    QVariantMap params;
    params.insert("type", "string_");
    params.insert("string_", QString::number(presetId));
    StreamUnlimitedSetRequest* request = new StreamUnlimitedSetRequest(m_nam, m_adddress, m_port, "googlecast:invokePresetAction", "activate", params, this);
    connect(request, &StreamUnlimitedSetRequest::error, this, [=]() {
        emit commandCompleted(commandId, false);
    });
    connect(request, &StreamUnlimitedSetRequest::finished, this, [=](const QByteArray &response) {
        qCDebug(dcStreamUnlimited()) << "Invoke preset response" << response;
        emit commandCompleted(commandId, response == "null");
    });
    return commandId;
}

int StreamUnlimitedDevice::browseInternal(const QString &itemId, int commandIdOverride)
{
    int commandId = commandIdOverride;
    if (commandIdOverride == -1) {
        commandId = m_commandId++;
    }

    QStringList roles = {"path", "title", "icon", "type", "description", "containerPlayable", "audioType", "context", "mediaData", "flags", "timestamp", "value", "disabled"};

    QVariantMap containerInfo;
    QString node = itemId;

    if (itemId.isEmpty()) {
        // FIXME: Newer versions of nSDK use ui: instead of the old path of /ui.
        // However, new nSDK is backwards compatible, so let's use /ui for now
        // to be compatible with older models too
//        node = "ui:";
        node = "/ui";
    } else {
        node.remove(QRegExp("^container:"));
        QJsonDocument jsonDoc = QJsonDocument::fromJson(node.toUtf8());
        containerInfo = jsonDoc.toVariant().toMap();
        node = containerInfo.value("path").toString();
    }

    StreamUnlimitedBrowseRequest *request = new StreamUnlimitedBrowseRequest(m_nam, m_adddress, m_port, node, roles, this);
    connect(request, &StreamUnlimitedBrowseRequest::error, this, [=](){
        qCWarning(dcStreamUnlimited()) << "Browse error";
        emit browseResults(commandId, false);
    });
    connect(request, &StreamUnlimitedBrowseRequest::finished, this, [=](const QVariantMap &result){
//        qCDebug(dcStreamUnlimited()) << "Browseresult:" << qUtf8Printable(QJsonDocument::fromVariant(result).toJson());

        if (result.contains("rowsRedirect")) {
            QVariantMap redirect;
            redirect.insert("path", result.value("rowsRedirect").toString());
            QJsonDocument jsonDoc = QJsonDocument::fromVariant(redirect);
            browseInternal("container:" + jsonDoc.toJson(QJsonDocument::Compact), commandId);
            return;
        }

        if (!result.contains("rows")) {
            qCWarning(dcStreamUnlimited()) << "Response from SU device doesn't have rows:" << qUtf8Printable(QJsonDocument::fromVariant(result).toJson());
            emit browseResults(commandId, false);
            return;
        }


        BrowserItems *items = new BrowserItems();
        QList<StreamUnlimitedBrowseRequest*> *pendingContextRequests = new QList<StreamUnlimitedBrowseRequest*>();

        QVariantList rows = result.value("rows").toList();
        qCDebug(dcStreamUnlimited()) << "Browsing returned" << rows.count() << "rows";
        for (int i = 0; i < rows.count(); i++) {
            QVariantList entry = rows.at(i).toList();
            if (entry.length() < 12) {
                qCWarning(dcStreamUnlimited()) << "Received invalid reply from SU device:" << qUtf8Printable(QJsonDocument::fromVariant(result).toJson());
                continue;
            }

            QString pathPrefix;
            QString path = entry.takeFirst().toString();
            QString title = entry.takeFirst().toString();
            QString icon = entry.takeFirst().toString();
            QString type = entry.takeFirst().toString();
            QString description = entry.takeFirst().toString();
            QString containerPlayable = entry.takeFirst().toString();
            QString audioType = entry.takeFirst().toString();
            QVariantMap context = entry.takeFirst().toMap();
            QVariantMap mediaData = entry.takeFirst().toMap();
            QVariantMap flags = entry.takeFirst().toMap();
            QString timestamp = entry.takeFirst().toString();
            QVariantMap value = entry.takeFirst().toMap();
            bool disabled = entry.takeFirst().toBool();

            bool browsable = false;
            bool executable = false;

            BrowserItem::BrowserIcon browserIcon = BrowserItem::BrowserIconNone;

            if (type == "header") {
                // Generate random id as ids need to be unique in nymea
                path = QUuid::createUuid().toString();
            }
            if (type == "value") {
                description = value.value(value.value("type").toString()).toString();
            }

            if (type == "audio") {
                executable = true;
                browserIcon = BrowserItem::BrowserIconMusic;

                pathPrefix = "audio:";

                QVariantMap playbackInfo;
                playbackInfo.insert("control", "play");
                if (audioType == "audioBroadcast") {
                    QVariantMap mediaRoles;
                    mediaRoles.insert("title", title);
                    mediaRoles.insert("icon", icon);
                    mediaRoles.insert("type", type);
                    mediaRoles.insert("audioType", audioType);
                    mediaRoles.insert("path", path);
                    mediaRoles.insert("mediaData", mediaData);
                    mediaRoles.insert("context", context);
                    mediaRoles.insert("description", description);
                    playbackInfo.insert("mediaRoles", mediaRoles);
                } else {
                    playbackInfo.insert("type", "itemInContainer");
                    playbackInfo.insert("index", QString::number(i));
                    playbackInfo.insert("mediaRoles", containerInfo);
                    playbackInfo.insert("version", "9");
                    QVariantMap trackRoles;
                    trackRoles.insert("title", title);
                    trackRoles.insert("type", type);
                    trackRoles.insert("flags", flags);
                    trackRoles.insert("path", path);
                    trackRoles.insert("mediaData", mediaData);
                    trackRoles.insert("timestamp", timestamp);
                    trackRoles.insert("context", context);
                    trackRoles.insert("containerPlayable", false);
                    playbackInfo.insert("trackRoles", trackRoles);
                }

                QJsonDocument jsonDoc = QJsonDocument::fromVariant(playbackInfo);

                path = jsonDoc.toJson(QJsonDocument::Compact);

            }

            if (type == "container") {
                browsable = true;
                browserIcon = BrowserItem::BrowserIconFolder;
                pathPrefix = "container:";

                QVariantMap containerInfo;
                containerInfo.insert("title", title);
                containerInfo.insert("type", type);
                containerInfo.insert("flags", flags);
                containerInfo.insert("path", path);
                containerInfo.insert("mediaData", mediaData);
                containerInfo.insert("timestamp", timestamp);
                containerInfo.insert("context", context);
                containerInfo.insert("containerPlayable", containerPlayable);

                QJsonDocument jsonDoc = QJsonDocument::fromVariant(containerInfo);
                path = jsonDoc.toJson(QJsonDocument::Compact);
            }


            if (type == "action") {
                executable = true;
                pathPrefix = "action:";
                browserIcon = BrowserItem::BrowserIconApplication;
            }


            MediaBrowserItem browserItem(pathPrefix + path, title);
            browserItem.setDescription(description);
            browserItem.setBrowsable(browsable);
            browserItem.setExecutable(executable);
            browserItem.setIcon(browserIcon);
            browserItem.setDisabled(disabled);

            if (icon.startsWith("skin:")) {
                if (icon == "skin:iconMusicLibrary") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconMusicLibrary);
                } else if (icon == "skin:iconRecentlyPlayed") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconPlaylist);
                } else if (icon == "skin:iconPlaylists") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconPlaylist);
                } else if (icon == "skin:iconFavorites") {
                    browserItem.setIcon(BrowserItem::BrowserIconFavorites);
                } else if (icon == "skin:iconSpotify") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconSpotify);
                } else if (icon == "skin:iconAirable") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconAirable);
                } else if (icon == "skin:iconTidal") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconTidal);
                } else if (icon == "skin:iconvTuner") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconVTuner);
                } else if (icon == "skin:iconSirius") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconSiriusXM);
                } else if (icon == "skin:iconTuneIn") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconTuneIn);
                } else if (icon == "skin:iconAmazonMusic") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconAmazon);
                } else if (icon == "skin:iconCdrom") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconDisk);
                } else if (icon == "skin:iconUsb") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconUSB);
                } else if (icon == "skin:iconMediaLibrary") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconNetwork);
                } else if (icon == "skin:iconAUX") {
                    browserItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconAux);
                }
            } else {
                browserItem.setThumbnail(icon);
            }

            if (context.value("type").toString() == "container" && context.value("containerType").toString() == "context") {
                // Item has a context menu entry... Let's fetch that
                StreamUnlimitedBrowseRequest *contextRequest = new StreamUnlimitedBrowseRequest(m_nam, m_adddress, m_port, context.value("path").toString(), {"path", "title", "icon", "type", "description", "containerPlayable", "audioType", "context", "mediaData", "flags", "timestamp", "value"
                                                                                                                                                              //                                               "", "personType", "albumType", "imageType", "videoType", "epgType", "modifiable", "disabled", "valueOperation()", "edit", "query", "activate", "likeIt", "rowsOperation", "setR>
                                                                                                                                                                                                             "", "containerType"}, this);
                pendingContextRequests->append(contextRequest);
                connect(contextRequest, &StreamUnlimitedBrowseRequest::error, this, [=](){
                    pendingContextRequests->removeAll(contextRequest);
                    // We failed to fetch the context... Return the item nevertheless...
                    items->append(browserItem);
                    // If there are no more pending context requests, finish the entire request
                    if (pendingContextRequests->isEmpty()) {
                        emit browseResults(commandId, true, *items);
                        delete pendingContextRequests;
                        delete items;
                    }
                });
                connect(contextRequest, &StreamUnlimitedBrowseRequest::finished, this, [=](const QVariantMap &contextResult){

                    pendingContextRequests->removeAll(contextRequest);

                    QList<ActionTypeId> itemActionTypes;
                    QVariantList contextItems = contextResult.value("rows").toList();
                    foreach (const QVariant &contextRow, contextItems) {
                        QString contextItemPath = contextRow.toList().takeFirst().toString();
                        if (contextItemPath.startsWith("playlists:pl/selectaddmode")) {
                            qCDebug(dcStreamUnlimited()) << "Have add to play queue context action:" << contextItemPath;

                            itemActionTypes.append(streamSDKdevBoardAddToPlayQueueBrowserItemActionTypeId);

                        } else if (contextItemPath.startsWith("playlists:pl/addtoplaylist")) {
                            qCDebug(dcStreamUnlimited()) << "Have add to playlist context action:" << contextItemPath;
                        } else if (contextItemPath.startsWith("playlists:pq/contextmenu?action=clearPl")) {
                            qCDebug(dcStreamUnlimited()) << "Have clear playlist context action:" << contextItemPath;
                            itemActionTypes.append(streamSDKdevBoardClearPlaylistBrowserItemActionTypeId);
                        } else {
                            qCDebug(dcStreamUnlimited()) << "Have unknown context menu item:" << contextItemPath;
                        }
                    }

                    MediaBrowserItem copy(browserItem);
                    copy.setActionTypeIds(itemActionTypes);
                    items->append(copy);

                    // If there are no more pending context requests, finish the entire request
                    if (pendingContextRequests->isEmpty()) {
                        emit browseResults(commandId, true, *items);
                        delete pendingContextRequests;
                        delete items;
                    }
                });

                // Don't add this item to the result set just yet. We'll do that that when the context request finished
                continue;
            }

            // No context request, add this item to the result
            items->append(browserItem);
        }

        // If there are no pending context requests at all, finish the entire request right away
        if (pendingContextRequests->isEmpty()) {
            emit browseResults(commandId, true, *items);
            delete pendingContextRequests;
            delete items;
        }

    });
    return commandId;
}

void StreamUnlimitedDevice::pollQueue()
{
    if (m_pollReply) {
        m_pollReply->abort();
        m_pollReply = nullptr;
    }

    QUrl url;
    url.setScheme("http");
    url.setHost(m_adddress.toString());
    url.setPort(m_port);
    url.setPath("/api/event/pollQueue");

    QUrlQuery query;
    query.addQueryItem("queueId", m_pollQueueId.toString());
    query.addQueryItem("timeout", "25"); // Timeout must be less than 30 secs as nymea will kill network requests after that time
    url.setQuery(query);

//    qCDebug(dcStreamUnlimited()) << "Polling" << url;
    QNetworkRequest request(url);
    request.setRawHeader("Connection", "keep-alive");

    QNetworkReply *reply = m_nam->get(request);
    m_pollReply = reply;
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        m_pollReply = nullptr;
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcStreamUnlimited()) << "Connection to StreamUnlimited device lost:" << reply->errorString();
            m_connectionStatus = ConnectionStatusError;
            emit connectionStatusChanged(ConnectionStatusDisconnected);

            QTimer::singleShot(10000, this, [this](){
                if (connectionStatus() != ConnectionStatusConnecting && connectionStatus() != ConnectionStatusConnected) {
                    setHost(m_adddress, m_port);
                }
            });

            return;
        }

        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcStreamUnlimited()) << "Error parsing json from StreamUnlimited device:" << error.errorString();
            m_connectionStatus = ConnectionStatusError;
            emit connectionStatusChanged(ConnectionStatusDisconnected);
            return;
        }

        QVariantList changes = jsonDoc.toVariant().toList();
        foreach (const QVariant &change, changes) {
            QVariantMap changeMap = change.toMap();
            if (changeMap.value("itemType").toString() == "update") {
                if (changeMap.value("path").toString() == "player:volume") {
                    refreshVolume();
                } else if (changeMap.value("path").toString() == "player:player/data") {
                    refreshPlayerData();
                } else if (changeMap.value("path").toString() == "settings:/mediaPlayer/mute") {
                    refreshMute();
                } else if (changeMap.value("path").toString() == "settings:/mediaPlayer/playMode") {
                    refreshPlayMode();
                } else if (changeMap.value("path").toString() == "settings:/ui/language") {
                    refreshLanguage();
                } else {
                    qCWarning(dcStreamUnlimited()) << "Unhandled change event" << change;
                }
            }
        }


        pollQueue();
    });
}

void StreamUnlimitedDevice::refreshVolume()
{
    StreamUnlimitedGetRequest *request = new StreamUnlimitedGetRequest(m_nam, m_adddress, m_port, "player:volume", {"value"}, this);
    connect(request, &StreamUnlimitedGetRequest::finished, this, [=](const QVariantMap &result){
        QVariantMap volumeMap = result.value("value").toMap();
        m_volume = volumeMap.value(volumeMap.value("type").toString()).toUInt();
        emit volumeChanged(m_volume);
    });
}

void StreamUnlimitedDevice::refreshMute()
{
    StreamUnlimitedGetRequest *request = new StreamUnlimitedGetRequest(m_nam, m_adddress, m_port, "settings:/mediaPlayer/mute", {"value"}, this);
    connect(request, &StreamUnlimitedGetRequest::finished, this, [=](const QVariantMap &result){
        QVariantMap muteMap = result.value("value").toMap();
        m_mute = muteMap.value(muteMap.value("type").toString()).toBool();
        emit muteChanged(m_mute);
    });
}

void StreamUnlimitedDevice::refreshPlayMode()
{
    StreamUnlimitedGetRequest *request = new StreamUnlimitedGetRequest(m_nam, m_adddress, m_port, "settings:/mediaPlayer/playMode", {"value"}, this);
    connect(request, &StreamUnlimitedGetRequest::finished, this, [=](const QVariantMap &result){
        QVariantMap playModeMap = result.value("value").toMap();
        QString playModeString = playModeMap.value("playerPlayMode").toString();
        bool shuffle = false;
        Repeat repeat = RepeatNone;
        if (playModeString.contains("shuffle")) {
            shuffle = true;
        }
        if (playModeString.toLower().contains("repeatone")) {
            repeat = RepeatOne;
        } else if (playModeString.toLower().contains("repeatall")) {
            repeat = RepeatAll;
        }

        if (m_shuffle != shuffle) {
            m_shuffle = shuffle;
            emit shuffleChanged(shuffle);
        }
        if (m_repeat != repeat) {
            m_repeat = repeat;
            emit repeatChanged(repeat);
        }
    });
}

void StreamUnlimitedDevice::refreshLanguage()
{
    StreamUnlimitedGetRequest *request = new StreamUnlimitedGetRequest(m_nam, m_adddress, m_port, "settings:/ui/language", {"value"}, this);
    connect(request, &StreamUnlimitedGetRequest::finished, this, [=](const QVariantMap &result){
        QVariantMap languageMap = result.value("value").toMap();
        m_language = QLocale(languageMap.value(languageMap.value("type").toString()).toString());
        emit volumeChanged(m_volume);
    });
}

StreamUnlimitedSetRequest *StreamUnlimitedDevice::setPlayMode(bool shuffle, StreamUnlimitedDevice::Repeat repeat)
{
    QString shuffleRepeatString;

    if (shuffle) {
        if (repeat == RepeatOne) {
            shuffleRepeatString = "shuffleRepeatOne";
        } else if (repeat == RepeatAll) {
            shuffleRepeatString = "shuffleRepeatAll";
        } else {
            shuffleRepeatString = "shuffle";
        }
    } else {
        if (repeat == RepeatOne) {
            shuffleRepeatString = "repeatOne";
        } else if (repeat == RepeatAll) {
            shuffleRepeatString = "repeatAll";
        } else {
            shuffleRepeatString = "normal";
        }
    }
    QVariantMap params;
    params.insert("type", "playerPlayMode");
    params.insert("playerPlayMode", shuffleRepeatString);

    return new StreamUnlimitedSetRequest(m_nam, m_adddress, m_port, "settings:/mediaPlayer/playMode", "value", params, this);
}

int StreamUnlimitedDevice::executeControlCommand(const QString &command)
{
    int commandId = m_commandId++;

    QVariantMap params;
    params.insert("control", command);
    StreamUnlimitedSetRequest *request = new StreamUnlimitedSetRequest(m_nam, m_adddress, m_port, "player:player/control", "activate", params, this);

    connect(request, &StreamUnlimitedSetRequest::error, this, [=](){
        emit commandCompleted(commandId, false);
    });
    connect(request, &StreamUnlimitedSetRequest::finished, this, [=](const QByteArray &data){
        emit commandCompleted(commandId, data == "true");
    });

    return commandId;
}

void StreamUnlimitedDevice::refreshPlayerData()
{
    StreamUnlimitedGetRequest *request = new StreamUnlimitedGetRequest(m_nam, m_adddress, m_port, "player:player/data", {"value"}, this);
    connect(request, &StreamUnlimitedGetRequest::finished, this, [=](const QVariantMap &result){
        QString playStatusString = result.value("value").toMap().value("state").toString();
        PlayStatus playStatus;
        if (playStatusString == "playing") {
            playStatus = PlayStatusPlaying;
        } else if (playStatusString == "paused") {
            playStatus = PlayStatusPaused;
        } else {
            playStatus = PlayStatusStopped;
        }
        if (m_playbackStatus != playStatus) {
            m_playbackStatus = playStatus;
            emit playbackStatusChanged(m_playbackStatus);
        }

        QString title = result.value("value").toMap().value("trackRoles").toMap().value("title").toString();
        if (title != m_title) {
            m_title = title;
            emit titleChanged(title);
        }

        QString artist = result.value("value").toMap().value("trackRoles").toMap().value("mediaData").toMap().value("metaData").toMap().value("artist").toString();
        if (artist != m_artist) {
            m_artist = artist;
            emit artistChanged(artist);
        }

        QString album = result.value("value").toMap().value("trackRoles").toMap().value("mediaData").toMap().value("metaData").toMap().value("album").toString();
        if (album != m_album) {
            m_album = album;
            emit albumChanged(album);
        }

        QString artwork = result.value("value").toMap().value("trackRoles").toMap().value("icon").toString();
        if (artwork != m_artwork) {
            m_artwork = artwork;
            emit artworkChanged(artwork);
        }
    });
}
