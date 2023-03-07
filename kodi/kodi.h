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

#ifndef KODI_H
#define KODI_H

#include <QObject>
#include <QHostAddress>

#include "kodiconnection.h"
#include "kodijsonhandler.h"

#include "types/browseritem.h"
#include "types/browseritemaction.h"
#include "integrations/thing.h"
#include "integrations/browseresult.h"
#include "integrations/browseritemresult.h"

class Kodi : public QObject
{
    Q_OBJECT
public:

    explicit Kodi(const QHostAddress &hostAddress, int port = 9090, int httpPort = 8080, QObject *parent = nullptr);
    ~Kodi();

    QHostAddress hostAddress() const;
    void setHostAddress(const QHostAddress &address);

    uint port() const;
    void setPort(uint port);

    uint httpPort() const;
    void setHttpPort(uint httpPort);

    bool connected() const;

    // propertys
    int setMuted(const bool &muted);
    bool muted() const;

    int setVolume(const int &volume);
    int volume() const;

    int setShuffle(bool shuffle);
    int setRepeat(const QString &repeat);

    // actions
    int showNotification(const QString &title, const QString &message, const int &displayTime, const QString &image);
    int navigate(const QString &to);
    int systemCommand(const QString &command);

    void update();
    void checkVersion();

    void connectKodi();
    void disconnectKodi();

    void browse(BrowseResult *result);
    void browserItem(BrowserItemResult *result);
    int launchBrowserItem(const QString &itemId);
    int executeBrowserItemAction(const QString &itemId, const ActionTypeId &actionTypeId);

signals:
    void connectionStatusChanged(bool connected);
    void stateChanged();
    void activePlayerChanged(const QString &playerType);
    void actionExecuted(int actionId, bool success);
    void browserItemExecuted(int actionId, bool success);
    void browserItemActionExecuted(int actionId, bool success);
    void updateDataReceived(const QVariantMap &data);
    void playbackStatusChanged(const QString &playbackState);
    void mediaMetadataChanged(const QString &title, const QString &artist, const QString &collection, const QString &artwork);
    void shuffleChanged(bool shuffle);
    void repeatChanged(const QString &repeat);

private slots:
    void onConnectionStatusChanged();
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
        ~VirtualFsNode() { qDeleteAll(childs); }
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

    QHash<int, BrowseResult*> m_pendingBrowseRequests;
    QHash<int, BrowserItemResult*> m_pendingBrowserItemRequests;

};

#endif // KODI_H
