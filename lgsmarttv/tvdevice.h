/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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
#include <QXmlStreamReader>
#include <QXmlReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttributes>

#include "tveventhandler.h"

class TvDevice : public QObject
{
    Q_OBJECT
public:
    explicit TvDevice(const QHostAddress &hostAddress, const int &port, QObject *parent = 0);

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

    void setHostAddress(const QHostAddress &hostAddress);
    QHostAddress hostAddress() const;

    void setPort(int port);
    int port() const;

    void setUuid(const QString &uuid);
    QString uuid() const;

    // States
    void setPaired(bool paired);
    bool paired() const;

    void setReachable(bool reachable);
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
    static QPair<QNetworkRequest, QByteArray> createDisplayKeyRequest(const QHostAddress &host, int port);
    static QPair<QNetworkRequest, QByteArray> createPairingRequest(const QHostAddress &host, int port, const QString &key);
    static QPair<QNetworkRequest, QByteArray> createEndPairingRequest(const QUrl &url);
    static QPair<QNetworkRequest, QByteArray> createEndPairingRequest(const QHostAddress &host, int port);
    static QPair<QNetworkRequest, QByteArray> createEventRequest(const QHostAddress &host, int port);

    QPair<QNetworkRequest, QByteArray> createPressButtonRequest(TvDevice::RemoteKey key);

    QNetworkRequest createVolumeInformationRequest();
    QNetworkRequest createChannelInformationRequest();
    void onVolumeInformationUpdate(const QByteArray &data);
    void onChannelInformationUpdate(const QByteArray &data);

private:
    TvEventHandler *m_eventHandler = nullptr;

    QHostAddress m_hostAddress;
    int m_port;
    QString m_uuid;
    QString m_key;

    // States
    bool m_paired;
    bool m_reachable;
    bool m_is3DMode;
    bool m_mute;
    int m_volumeLevel;
    int m_inputSourceIndex;
    int m_channelNumber;
    QString m_channelType;
    QString m_channelName;
    QString m_programName;
    QString m_inputSourceLabel;

    QString printXmlData(const QByteArray &data);

signals:
    void stateChanged();

private slots:
    void eventOccured(const QByteArray &data);

};

#endif // TVDEVICE_H
