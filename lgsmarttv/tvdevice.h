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

#ifndef TVDEVICE_H
#define TVDEVICE_H

#include <QObject>
#include <QUrl>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QXmlStreamReader>
#include <QXmlReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttributes>

#include "integrations/integrationplugin.h"
#include "tveventhandler.h"

class TvDevice : public QObject
{
    Q_OBJECT
public:
    explicit TvDevice(const QHostAddress &hostAddress, quint16 port, quint16 eventHandlerPort, QObject *parent = 0);

    enum RemoteKey{
        Power           = 1,
        Num0            = 2,
        Num1            = 3,
        Num2            = 4,
        Num3            = 5,
        Num4            = 6,
        Num5            = 7,
        Num6            = 8,
        Num7            = 9,
        Num8            = 10,
        Num9            = 11,
        Up              = 12,
        Down            = 13,
        Left            = 14,
        Right           = 15,
        Ok              = 20,
        Home            = 21,
        Menu            = 22,
        Back            = 23,
        VolUp           = 24,
        VolDown         = 25,
        Mute            = 26,
        ChannelUp       = 27,
        ChannelDown     = 28,
        Blue            = 29,
        Green           = 30,
        Red             = 31,
        Yellow          = 32,
        Play            = 33,
        Pause           = 34,
        Stop            = 35,
        FastForward     = 36,
        Rewind          = 37,
        SkipForward     = 38,
        SkipBackward    = 39,
        Record          = 40,
        RecrodList      = 41,
        Repeat          = 42,
        LiveTv          = 43,
        EPG             = 44,
        Info            = 45,
        AspectRatio     = 46,
        ExternalInput   = 47,
        PIP             = 48,
        ChangeSub       = 49,
        ProgramList     = 50,
        TeleText        = 51,
        Mark            = 52,
        Video3D         = 400,
        LR_3D           = 401,
        Dash            = 402,
        PrevoiusChannel = 403,
        FavouritChannel = 404,
        QuickMenu       = 405,
        TextOption      = 406,
        AudioDescription= 407,
        NetCastKey      = 408,
        EnergySaving    = 409,
        AV_Mode         = 410,
        SimpLink        = 411,
        Exit            = 412,
        ReservationList = 413,
        PipUp           = 414,
        PipDown         = 415,
        SwitchVideo     = 416,
        MyApps          = 417
    };

    // propertys
    void setKey(const QString &key);
    QString key() const;

    QHostAddress hostAddress() const;
    quint16 port() const;
    quint16 eventHandlerPort() const;

    void setUuid(const QString &uuid);
    QString uuid() const;

    // States
    void setPaired(const bool &paired);
    bool paired() const;

    void setReachable(const bool &reachable);
    bool reachable() const;

    bool is3DMode() const;
    int volumeLevel() const;
    bool mute() const;
    QString channelType() const;
    QString channelName() const;
    int channelNumber() const;
    QString programName() const;
    int inputSourceIndex() const;
    QString inputSourceLabelName() const;

    // other methods
    static QPair<QNetworkRequest, QByteArray> createDisplayKeyRequest(const QHostAddress &host, quint16 port);
    static QPair<QNetworkRequest, QByteArray> createPairingRequest(const QHostAddress &host, quint16 port, quint16 eventHandlerPort, const QString &key);
    static QPair<QNetworkRequest, QByteArray> createEndPairingRequest(const QHostAddress &host, quint16 port, quint16 eventHandlerPort);
    static QPair<QNetworkRequest, QByteArray> createEventRequest(const QHostAddress &host, quint16 port, quint16 eventHandlerPort);

    QPair<QNetworkRequest, QByteArray> createPressButtonRequest(const TvDevice::RemoteKey &key);

    QUrl buildDisplayImageUrl() const;

    QNetworkRequest createVolumeInformationRequest();
    QNetworkRequest createChannelInformationRequest();
    QNetworkRequest createChannelListRequest();
    QNetworkRequest createAppListCountRequest(int type);
    QNetworkRequest createAppListRequest(int type, int appCount);

    void onVolumeInformationUpdate(const QByteArray &data);
    void onChannelInformationUpdate(const QByteArray &data);
    void onAppListUpdated(const QByteArray &appList);

    static QString printXmlData(const QByteArray &data);

    void browse(BrowseResult *result);
    void browserItem(BrowserItemResult *result);
    int launchBrowserItem(const QString &itemId);

    void clearAppList();
    void addAppList(const QVariantList &appList);

private:
    TvEventHandler *m_eventHandler;

    QHostAddress m_hostAddress;
    quint16 m_port = 0;
    quint16 m_eventHandlerPort = 0;
    QString m_uuid;
    QString m_key;

    QVariantList m_appList;

    // States
    bool m_paired = false;
    bool m_reachable = false;
    bool m_is3DMode = false;
    bool m_mute = false;
    int m_volumeLevel = 20;
    int m_inputSourceIndex = -1;
    int m_channelNumber = -1;
    QString m_channelType;
    QString m_channelName;
    QString m_programName;
    QString m_inputSourceLabel;

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


signals:
    void stateChanged();

private slots:
    void eventOccured(const QByteArray &data);

};

#endif // TVDEVICE_H
