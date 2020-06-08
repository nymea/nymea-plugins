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

#ifndef AVRCONNECTION_H
#define AVRCONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QUuid>

class AvrConnection : public QObject
{
    Q_OBJECT
public:
    enum RepeatMode {
        RepeatModeRepeatAll,
        RepeatModeRepeatOne,
        RepeatModeRepeatNone
    };

    enum PlayBackMode {
        PlayBackModePlaying,
        PlayBackModeStopped,
        PlayBackModePaused
    };

    explicit AvrConnection(const QHostAddress &hostAddress, const int &port = 23, QObject *parent = nullptr);
    ~AvrConnection();

    void connectDevice();
    void disconnectDevice();

    QHostAddress hostAddress() const;
    int port() const;
    bool connected();

    QUuid getChannel();
    QUuid getVolume();
    QUuid getMute();
    QUuid getPower();
    QUuid getSurroundMode();
    QUuid getPlayBackInfo();

    QUuid setChannel(const QByteArray &channel);
    QUuid setVolume(int volume);
    QUuid setMute(bool mute);
    QUuid setPower(bool power);
    QUuid setSurroundMode(const QByteArray &surroundMode);
    QUuid enableToneControl(bool enabled);
    QUuid setBassLevel(int level); //-6 to +6
    QUuid setTrebleLevel(int level); //-6 to +6

    QUuid getBassLevel();
    QUuid getTrebleLevel();
    QUuid getToneControl();

    QUuid play();
    QUuid pause();
    QUuid stop();
    QUuid skipNext();
    QUuid skipBack();
    QUuid setRandom(bool on);
    QUuid setRepeat(RepeatMode mode);

    QUuid increaseVolume();
    QUuid decreaseVolume();
private:
    QTimer *m_commandTimer = nullptr;
    QTcpSocket *m_socket = nullptr;
    QHostAddress m_hostAddress;
    int m_port;
    QList<QPair<QUuid, QByteArray>> m_commandBuffer;

    QUuid sendCommand(const QByteArray &message);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void readData();

signals:
    void socketErrorOccured(QAbstractSocket::SocketError socketError);
    void connectionStatusChanged(bool status);
    void commandExecuted(const QUuid &commandId, bool success);
    void volumeChanged(int volume);
    void muteChanged(bool mute);
    void channelChanged(const QString &channel);
    void powerChanged(bool power);
    void surroundModeChanged(const QString &surroundMode);
    void songChanged(const QString &song);
    void artistChanged(const QString &artist);
    void albumChanged(const QString &album);
    void playBackModeChanged(AvrConnection::PlayBackMode);
    void bassLevelChanged(int level);
    void trebleLevelChanged(int level);
    void toneControlEnabledChanged(bool enabled);
};

#endif // AVRCONNECTION_H
