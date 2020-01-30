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
    // Note: error signal will be interpreted as function, not as signal in C++11
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
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

int AvrConnection::port() const
{
    return m_port;
}

bool AvrConnection::connected()
{
    return m_socket->isOpen();
}

void AvrConnection::getAllStatus()
{
    sendCommand("PW?\rSI?\rMV?\rMS?\rMU?\r");
}

void AvrConnection::getChannel()
{
    sendCommand("SI?\r");
}

void AvrConnection::getVolume()
{
    sendCommand("MV?\r");
}

void AvrConnection::getMute()
{
    sendCommand("MU?\r");
}

void AvrConnection::getPower()
{
    sendCommand("PW?\r");
}

void AvrConnection::getSurroundMode()
{
    sendCommand("MS?\r");
}

void AvrConnection::sendCommand(const QByteArray &message)
{
    m_socket->write(message);
}

void AvrConnection::setChannel(const QByteArray &channel)
{
    QByteArray cmd = "SI" + channel + "\r";
    qCDebug(dcDenon) << "Change to channel:" << cmd;
    sendCommand(cmd);
}

void AvrConnection::setVolume(int volume)
{
    qCDebug(dcDenon) << "Set volume" << volume;
    QByteArray cmd = "MV" + QByteArray::number(volume) + "\r";
    sendCommand(cmd);
}

void AvrConnection::setMute(bool mute)
{
    qCDebug(dcDenon) << "Set mute" << mute;
    QByteArray cmd;
    if (mute) {
        cmd = "MUON\r";
    } else {
        cmd = "MUOFF\r";
    }
    sendCommand(cmd);
}

void AvrConnection::setPower(bool power)
{
    qCDebug(dcDenon) << "Set power" << power;
    QByteArray cmd;
    if (power) {
        cmd = "PWON\r";
    } else {
        cmd = "PWSTANDBY\r";
    }
    sendCommand(cmd);
}

void AvrConnection::setSurroundMode(const QByteArray &surroundMode)
{
    qCDebug(dcDenon) << "Set surround mode" << surroundMode;
    QByteArray cmd = "MS" + surroundMode + "\r";
    sendCommand(cmd);
}

void AvrConnection::increaseVolume()
{
    qCDebug(dcDenon) << "Execute volume increase";
    QByteArray cmd = "MVUP\r";
    sendCommand(cmd);
}

void AvrConnection::decreaseVolume()
{
    qCDebug(dcDenon) << "Execute volume decrease";
    QByteArray cmd = "MVDOWN\r";
    sendCommand(cmd);
}

void AvrConnection::onConnected()
{
    qCDebug(dcDenon) << "connected successfully to" << hostAddress().toString() << port();
    emit connectionStatusChanged(true);
}

void AvrConnection::onDisconnected()
{
    qCDebug(dcDenon) << "disconnected from" << hostAddress().toString() << port();
    emit connectionStatusChanged(false);
}

void AvrConnection::onError(QAbstractSocket::SocketError socketError)
{
    qCWarning(dcDenon) << "socket error:" << socketError << m_socket->errorString();
    emit socketErrorOccured(socketError);
}

void AvrConnection::readData()
{
    QByteArray data = m_socket->readAll();
    qCDebug(dcDenon) << "Data received" << data;

    if (data.contains("MV") && !data.contains("MAX")){
        int index = data.indexOf("MV");
        int volume = data.mid(index+2, 2).toInt();
        emit volumeChanged(volume);
    }

    if (data.left(2).contains("SI")) {
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
    }

    if (data.contains("PWON")) {
        emit powerChanged(true);
    }
    if (data.contains("PWSTANDBY")) {
        emit powerChanged(false);
    }
    if (data.contains("MUON")) {
        emit muteChanged(false);
    }
    if (data.contains("MUOFF")) {
        emit muteChanged(false);
    }

    if (data.left(2).contains("MS")) {
        data.remove(0, 2);
        QByteArray cmd = data;
        emit surroundModeChanged(cmd);
    }
}
