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

#include "avrconnection.h"
#include "extern-plugininfo.h"

AvrConnection::AvrConnection(const QHostAddress &hostAddress, const int &port, QObject *parent) :
    QObject(parent),
    m_hostAddress(hostAddress),
    m_port(port)
{
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &AvrConnection::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &AvrConnection::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &AvrConnection::readData);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    connect(m_socket, &QTcpSocket::errorOccurred, this, &AvrConnection::onError);
#else
    // Note: error signal will be interpreted as function, not as signal in C++11
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
#endif


   m_commandTimer = new QTimer(this);
   m_commandTimer->start(50); // 50ms is the minimum request interval specified

   connect(m_commandTimer, &QTimer::timeout, this, [this] {
       if (!m_commandBuffer.isEmpty()) {
           QPair<QUuid, QByteArray> command =  m_commandBuffer.takeFirst();
           if (m_socket->write(command.second) == -1) {
               emit commandExecuted(command.first, false);
               qCWarning(dcDenon()) << "Could not execute command" << command.second;
           } else {
               emit commandExecuted(command.first, true);
           }
       } else {
           m_commandTimer->stop();
       }
   });
}

AvrConnection::~AvrConnection()
{
    m_socket->close();
}

void AvrConnection::connectDevice()
{
    if (m_socket->state() == QAbstractSocket::ConnectingState) {
        return;
    }
    m_socket->connectToHost(m_hostAddress, m_port);
}

void AvrConnection::disconnectDevice()
{
    m_socket->close();
}

QHostAddress AvrConnection::hostAddress() const
{
    return m_hostAddress;
}

void AvrConnection::setHostAddress(const QHostAddress &hostAddress)
{
    if (m_hostAddress != hostAddress) {
        disconnectDevice();
        m_hostAddress = hostAddress;
        connectDevice();
    }
}

int AvrConnection::port() const
{
    return m_port;
}

void AvrConnection::setPort(int port)
{
    if (m_port != port) {
        disconnectDevice();
        m_port = port;
        connectDevice();
    }
}

bool AvrConnection::connected()
{
    return m_socket->isOpen();
}

QUuid AvrConnection::getChannel()
{
    return sendCommand("SI?\r");
}

QUuid AvrConnection::getVolume()
{
    return sendCommand("MV?\r");
}

QUuid AvrConnection::getMute()
{
    return sendCommand("MU?\r");
}

QUuid AvrConnection::getPower()
{
    return sendCommand("PW?\r");
}

QUuid AvrConnection::getSurroundMode()
{
    return sendCommand("MS?\r");
}

QUuid AvrConnection::getPlayBackInfo()
{
    return sendCommand("NSE\r");
}

QUuid AvrConnection::sendCommand(const QByteArray &message)
{
   QUuid commandId = QUuid::createUuid();

   if (!m_commandTimer->isActive())
       m_commandTimer->start(50);

   m_commandBuffer.append(QPair<QUuid, QByteArray>(commandId, message));
   return commandId;
}

QUuid AvrConnection::setChannel(const QByteArray &channel)
{
    QByteArray cmd = "SI" + channel + "\r";
    qCDebug(dcDenon()) << "Change to channel:" << channel;
    return sendCommand(cmd);
}

QUuid AvrConnection::setVolume(uint volume)
{
    qCDebug(dcDenon()) << "Set volume" << volume;
    QByteArray cmd = "MV" + QByteArray::number(volume) + "\r";
    return sendCommand(cmd);
}

QUuid AvrConnection::setMute(bool mute)
{
    qCDebug(dcDenon()) << "Set mute" << mute;
    QByteArray cmd;
    if (mute) {
        cmd = "MUON\r";
    } else {
        cmd = "MUOFF\r";
    }
    return sendCommand(cmd);
}

QUuid AvrConnection::setPower(bool power)
{
    qCDebug(dcDenon()) << "Set power" << power;
    QByteArray cmd;
    if (power) {
        cmd = "PWON\r";
    } else {
        cmd = "PWSTANDBY\r";
    }
    return sendCommand(cmd);
}

QUuid AvrConnection::setSurroundMode(const QByteArray &surroundMode)
{
    qCDebug(dcDenon()) << "Set surround mode" << surroundMode;
    QByteArray cmd = "MS" + surroundMode + "\r";
    return sendCommand(cmd);
}

QUuid AvrConnection::enableToneControl(bool enabled)
{
    QByteArray cmd;
    if (enabled) {
        cmd = "PSTONE CTRL ON\r";
    } else {
        cmd = "PSTONE CTRL OFF\r";
    }
    return sendCommand(cmd);
}

QUuid AvrConnection::setBassLevel(int level)
{
    QByteArray cmd;
    cmd = "PSBAS ";
    cmd.append(50 + level);
    cmd.append("\r");
    return sendCommand(cmd);
}

QUuid AvrConnection::setTrebleLevel(int level)
{
    QByteArray cmd;
    cmd = "PSTRE ";
    cmd.append(50 + level);
    cmd.append("\r");
    return sendCommand(cmd);
}

QUuid AvrConnection::getBassLevel()
{
    return sendCommand("PSBAS ?\r");
}

QUuid AvrConnection::getTrebleLevel()
{
    return sendCommand("PSTRE ?\r");
}

QUuid AvrConnection::getToneControl()
{
    return sendCommand("PSTONE CTRL ?\r");
}

QUuid AvrConnection::play()
{
    return sendCommand("NS9A\r");
}

QUuid AvrConnection::pause()
{
    return sendCommand("NS9B\r");
}

QUuid AvrConnection::stop()
{
    return sendCommand("NS9C\r");
}

QUuid AvrConnection::skipNext()
{
    return sendCommand("NS9D\r");
}

QUuid AvrConnection::skipBack()
{
    return sendCommand("NS9E\r");
}

QUuid AvrConnection::setRandom(bool on)
{
    QByteArray cmd;
    if (on) {
        cmd = "NS9K\r";
    } else {
        cmd = "NS9M\r";
    }
    return sendCommand(cmd);
}

QUuid AvrConnection::setRepeat(AvrConnection::RepeatMode mode)
{
    QByteArray cmd;
    switch (mode) {
    case RepeatModeRepeatAll:
        cmd = "NS9I\r";
    break;
    case RepeatModeRepeatOne:
        cmd = "NS9H\r";
        break;
    case RepeatModeRepeatNone:
        cmd = "NS9J\r";
        break;
    }
    return sendCommand(cmd);
}

QUuid AvrConnection::increaseVolume()
{
    qCDebug(dcDenon()) << "Execute volume increase";
    QByteArray cmd = "MVUP\r";
    return sendCommand(cmd);
}

QUuid AvrConnection::decreaseVolume()
{
    qCDebug(dcDenon()) << "Execute volume decrease";
    QByteArray cmd = "MVDOWN\r";
    return sendCommand(cmd);
}

void AvrConnection::onConnected()
{
    qCDebug(dcDenon()) << "connected successfully to" << hostAddress().toString() << port();
    emit connectionStatusChanged(true);
}

void AvrConnection::onDisconnected()
{
    qCDebug(dcDenon()) << "disconnected from" << hostAddress().toString() << port();
    emit connectionStatusChanged(false);
}

void AvrConnection::onError(QAbstractSocket::SocketError socketError)
{
    qCWarning(dcDenon()) << "socket error:" << socketError << m_socket->errorString();
    emit socketErrorOccured(socketError);
}

void AvrConnection::readData()
{
    QString data = QString(m_socket->readAll());

    QStringList lines = data.split('\r');
    foreach (QString line, lines) {
        if(line.isEmpty())
            continue;

        qCDebug(dcDenon()) << "Data received" << line;
        if (line.contains("MV") && !data.contains("MAX")){
            int index = data.indexOf("MV");
            int volume = data.mid(index+2, 2).toInt();
            emit volumeChanged(volume);

        } else if (line.left(2).contains("SI")) {
            QByteArray cmd;
            if (data.contains("TUNER")) {
                cmd = "TUNER";
            } else if (data.contains("DVD")) {
                cmd = "DVD";
            } else if (data.contains("BD")) {
                cmd = "BD";
            } else if (data.contains("TV")) {
                cmd = "TV";
            } else if (data.contains("SAT/CBL")) {
                cmd = "SAT/CBL";
            } else if (data.contains("MPLAY")) {
                cmd = "MPLAY";
            } else if (data.contains("GAME")) {
                cmd = "GAME";
            } else if (data.contains("AUX1")) {
                cmd = "AUX1";
            } else if (data.contains("NET")) {
                cmd = "NET";
            } else if (data.contains("PANDORA")) {
                cmd = "PANDORA";
            } else if (data.contains("SIRIUSXM")) {
                cmd = "SIRIUSXM";
            } else if (data.contains("SPOTIFY")) {
                cmd = "SPOTIFY";
            } else if (data.contains("FLICKR")) {
                cmd = "FLICKR";
            } else if (data.contains("FAVORITES")) {
                cmd = "FAVORITES";
            } else if (data.contains("IRADIO")) {
                cmd = "IRADIO";
            } else if (data.contains("SERVER")) {
                cmd = "SERVER";
            } else if (data.contains("USB/IPOD")) {
                cmd = "USB/IPOD";
            } else if (data.contains("IPD")) {
                cmd = "IPD";
            } else if (data.contains("IRP")) {
                cmd = "IRP";
            } else if (data.contains("FVP")) {
                cmd = "FVP";
            }
            emit channelChanged(cmd);
        } else if (data.contains("PWON")) {
            emit powerChanged(true);
        } else if (data.contains("PWSTANDBY")) {
            emit powerChanged(false);
        } else if (data.contains("MUON")) {
            emit muteChanged(true);
        } else if (data.contains("MUOFF")) {
            emit muteChanged(false);
        } else if (data.left(2).contains("MS")) {
            QString surroundMode = data.remove(0, 2).trimmed();
            qCDebug(dcDenon()) << "Surround mode changed" << surroundMode;
            emit surroundModeChanged(surroundMode);

        } else if (data.left(4).contains("NSE0")) {
            QString nowPlaying = QString(data).remove(0, 4).trimmed();
            qCDebug(dcDenon()) << "Playbackstatus" << nowPlaying;
            if (nowPlaying.contains("Now Playing")) {
                emit playBackModeChanged(PlayBackMode::PlayBackModePlaying);
            } else {
                emit playBackModeChanged(PlayBackMode::PlayBackModeStopped);
            }
        } else if (data.left(4).contains("NSE1")) {
            QString song = QString(data).remove(0, 5).trimmed();
            qCDebug(dcDenon()) << "Song" << song;
            emit songChanged(song);
        } else if (data.left(4).contains("NSE2")) {
            QString artist = QString(data).remove(0, 5).trimmed();
            qCDebug(dcDenon()) << "Artist" << artist;
            emit artistChanged(artist);
        } else if (data.left(4).contains("NSE4")) {
            QString album = QString(data).remove(0, 5).trimmed();
            qCDebug(dcDenon()) << "Album" << album;
            emit albumChanged(album);
        } else if (data.contains("PSTONE CTRL ON")) {
            qCDebug(dcDenon()) << "Tone control is on";
            emit toneControlEnabledChanged(true);
        } else if (data.contains("PSTONE CTRL OFF")) {
            qCDebug(dcDenon()) << "Tone control is off";
            emit toneControlEnabledChanged(false);
        } else if (data.contains("PSBAS")) {
            int index = data.indexOf("PSBAS");
            int bass = data.mid(index+6, 2).toInt() - 50;
            qCDebug(dcDenon()) << "Bass level" << bass;
            emit bassLevelChanged(bass);
        } else if (data.contains("PSTRE")) {
            int index = data.indexOf("PSTRE");
            int treble = data.mid(index+6, 2).toInt() - 50;
            qCDebug(dcDenon()) << "Treble level" << treble;
            emit trebleLevelChanged(treble);
        }
    }
}
