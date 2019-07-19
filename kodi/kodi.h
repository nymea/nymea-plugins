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

#ifndef KODI_H
#define KODI_H

#include <QObject>
#include <QHostAddress>

#include "kodiconnection.h"
#include "kodijsonhandler.h"

#include "types/browseritem.h"
#include "types/browseritemaction.h"
#include "devices/device.h"

class Kodi : public QObject
{
    Q_OBJECT
public:

    explicit Kodi(const QHostAddress &hostAddress, int port = 9090, int httpPort = 8080, QObject *parent = nullptr);

    QHostAddress hostAddress() const;
    int port() const;

    bool connected() const;

    // propertys
    int setMuted(const bool &muted);
    bool muted() const;

    int setVolume(const int &volume);
    int volume() const;

    int setShuffle(bool shuffle);
    int setRepeat(const QString &repeat);

    // actions
    int showNotification(const QString &title, const QString &message, const int &displayTime, const QString &notificationType);
    int pressButton(const QString &button);
    int systemCommand(const QString &command);

    void update();
    void checkVersion();

    void connectKodi();
    void disconnectKodi();

    Device::BrowseResult browse(const QString &itemId, Device::BrowseResult &result);
    Device::BrowserItemResult browserItem(const QString &itemId, Device::BrowserItemResult &result);
    Device::DeviceError launchBrowserItem(const QString &itemId);
    int executeBrowserItemAction(const QString &itemId, const ActionTypeId &actionTypeId);

signals:
    void connectionStatusChanged();
    void stateChanged();
    void activePlayerChanged(const QString &playerType);
    void actionExecuted(int actionId, bool success);
    void updateDataReceived(const QVariantMap &data);
    void versionDataReceived(const QVariantMap &data);
    void playbackStatusChanged(const QString &playbackState);
    void mediaMetadataChanged(const QString &title, const QString &artist, const QString &collection, const QString &artwork);
    void shuffleChanged(bool shuffle);
    void repeatChanged(const QString &repeat);
    void browseResult(const Device::BrowseResult &result);
    void browserItemResult(const Device::BrowserItemResult &result);
    void browserItemActionExecuted(int actionId, bool success);

private slots:
    void onVolumeChanged(const int &volume, const bool &muted);
    void onUpdateFinished(const QVariantMap &data);
    void activePlayersChanged(const QVariantList &data);
    void playerPropertiesReceived(const QVariantMap &properties);
    void mediaMetaDataReceived(const QVariantMap &data);
    void onPlaybackStatusChanged(const QString &plabackState);

    void processNotification(const QString &method, const QVariantMap &params);
    void processResponse(int id, const QString &method, const QVariantMap &response);

    void updatePlayerProperties();
    void updateMetadata();

private:
    QString prepareThumbnail(const QString &thumbnail);

private:
    KodiConnection *m_connection;
    int m_httpPort;
    KodiJsonHandler *m_jsonHandler;
    bool m_muted;
    int m_volume;
    int m_activePlayerCount = 0; // if it's > 0, there is something playing (either music or video or slideshow)
    int m_activePlayer = -1;

    class VirtualFsNode {
    public:
        VirtualFsNode(const BrowserItem &item):item(item) {}
        BrowserItem item;
        QList<VirtualFsNode*> childs;
        QString getMethod;
        QVariantMap getParams;
        void addChild(VirtualFsNode* child) {childs.append(child); }
        VirtualFsNode *findNode(const QString &id) {
            if (item.id() == id) return this;
            foreach (VirtualFsNode *child, childs) {
                VirtualFsNode *node = child->findNode(id);
                if (node) return node;
            }
            return nullptr;
        }
    };
    VirtualFsNode* m_virtualFs = nullptr;

    QHash<int, Device::BrowseResult> m_pendingBrowseRequests;
    QHash<int, Device::BrowserItemResult> m_pendingBrowserItemRequests;

};

#endif // KODI_H
