/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "knxtunnel.h"

#include "extern-plugininfo.h"

#include <QDateTime>
#include <QNetworkInterface>

#include <QKnxTpdu>
#include <QKnx1Bit>
#include <QKnxDatapointType>
#include <QKnxLinkLayerFrame>
#include <QKnx8BitUnsignedValue>
#include <QKnxLinkLayerFrameBuilder>

Q_LOGGING_CATEGORY(dcKnxTunnelTraffic, "KnxTunnelTraffic")

KnxTunnel::KnxTunnel(const QHostAddress &remoteAddress, QObject *parent) :
    QObject(parent),
    m_remoteAddress(remoteAddress)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(false);
    m_timer->setInterval(5000);
    connect(m_timer, &QTimer::timeout, this, &KnxTunnel::onTimeout);

    m_queueTimer = new QTimer(this);
    m_queueTimer->setSingleShot(false);
    m_queueTimer->setInterval(500);
    connect(m_queueTimer, &QTimer::timeout, this, &KnxTunnel::onQueueTimeout);

    m_tunnel = new QKnxNetIpTunnel(this);
    m_tunnel->setLocalPort(0);
    m_tunnel->setHeartbeatTimeout(1000);

    connect(m_tunnel, &QKnxNetIpTunnel::frameReceived, this, &KnxTunnel::onTunnelFrameReceived);
    connect(m_tunnel, &QKnxNetIpTunnel::connected, this, &KnxTunnel::onTunnelConnected);
    connect(m_tunnel, &QKnxNetIpTunnel::disconnected, this, &KnxTunnel::onTunnelDisconnected);
    connect(m_tunnel, &QKnxNetIpTunnel::stateChanged, this, &KnxTunnel::onTunnelStateChanged);
}

QHostAddress KnxTunnel::remoteAddress() const
{
    return m_remoteAddress;
}

void KnxTunnel::setRemoteAddress(const QHostAddress &remoteAddress)
{
    m_remoteAddress = remoteAddress;
}

bool KnxTunnel::connected() const
{
    return m_tunnel->state() == QKnxNetIpTunnel::State::Connected;
}

bool KnxTunnel::connectTunnel()
{
    // Get the local address for this tunnel remote address
    QHostAddress localAddress = getLocalAddress(m_remoteAddress);
    if (localAddress.isNull()) {
        qCWarning(dcKnx()) << "Could connect to" << m_remoteAddress.toString() << ". There is no local interface for this server address. Make sure this device is connected to the correct network.";
        return false;
    }

    m_tunnel->setLocalAddress(localAddress);
    qCDebug(dcKnx()) << "Connecting tunnel to" << m_remoteAddress.toString() << "using" << m_tunnel->localAddress();
    m_tunnel->connectToHost(m_remoteAddress, m_port);
    return true;
}

void KnxTunnel::disconnectTunnel()
{
    qCDebug(dcKnx()) << "Disonnecting tunnel from" << m_remoteAddress.toString();
    m_tunnel->disconnectFromHost();
}

void KnxTunnel::sendKnxDpdSwitchFrame(const QKnxAddress &knxAddress, bool power)
{
    qCDebug(dcKnx()) << "Send DpdSwitch" << knxAddress.toString() << (power ? "ON" : "OFF");

    QKnxTpdu tpdu;
    tpdu.setTransportControlField(QKnxTpdu::TransportControlField::DataGroup);
    tpdu.setApplicationControlField(QKnxTpdu::ApplicationControlField::GroupValueWrite);
    tpdu.setData(QKnxSwitch(power ? QKnxSwitch::State::On : QKnxSwitch::State::Off).bytes());

    QKnxLinkLayerFrame frame = QKnxLinkLayerFrame::builder()
            .setMedium(QKnx::MediumType::NetIP)
            .setDestinationAddress(knxAddress)
            .setTpdu(tpdu)
            .createFrame();

    requestSendFrame(frame);
}

void KnxTunnel::sendKnxDpdUpDownFrame(const QKnxAddress &knxAddress, bool status)
{
    qCDebug(dcKnx()) << "Send DpdUpDown" << knxAddress.toString() << (status ? "UP" : "DOWN");

    QKnxTpdu tpdu;
    tpdu.setTransportControlField(QKnxTpdu::TransportControlField::DataGroup);
    tpdu.setApplicationControlField(QKnxTpdu::ApplicationControlField::GroupValueWrite);
    tpdu.setData(QKnxUpDown(status ? QKnxUpDown::State::Up : QKnxUpDown::State::Down).bytes());

    QKnxLinkLayerFrame frame = QKnxLinkLayerFrame::builder()
            .setMedium(QKnx::MediumType::NetIP)
            .setDestinationAddress(knxAddress)
            .setTpdu(tpdu)
            .createFrame();

    requestSendFrame(frame);
}

void KnxTunnel::sendKnxDpdStepFrame(const QKnxAddress &knxAddress, bool status)
{
    qCDebug(dcKnx()) << "Send DpdStep" << knxAddress.toString() << (status ? "INCREASE" : "DECREASE");

    QKnxTpdu tpdu;
    tpdu.setTransportControlField(QKnxTpdu::TransportControlField::DataGroup);
    tpdu.setApplicationControlField(QKnxTpdu::ApplicationControlField::GroupValueWrite);
    tpdu.setData(QKnxStep(status ? QKnxStep::State::Increase : QKnxStep::State::Decrease).bytes());

    QKnxLinkLayerFrame frame = QKnxLinkLayerFrame::builder()
            .setMedium(QKnx::MediumType::NetIP)
            .setDestinationAddress(knxAddress)
            .setTpdu(tpdu)
            .createFrame();

    requestSendFrame(frame);
}

void KnxTunnel::sendKnxDpdScalingFrame(const QKnxAddress &knxAddress, int scale)
{
    qCDebug(dcKnx()) << "Send DpdScaling" << knxAddress.toString() << scale;

    QKnxTpdu tpdu;
    tpdu.setTransportControlField(QKnxTpdu::TransportControlField::DataGroup);
    tpdu.setApplicationControlField(QKnxTpdu::ApplicationControlField::GroupValueWrite);
    tpdu.setData(QKnxScaling(scale).bytes());

    QKnxLinkLayerFrame frame = QKnxLinkLayerFrame::builder()
            .setMedium(QKnx::MediumType::NetIP)
            .setDestinationAddress(knxAddress)
            .setTpdu(tpdu)
            .createFrame();

    requestSendFrame(frame);
}

void KnxTunnel::readKnxGroupValue(const QKnxAddress &knxAddress)
{
    qCDebug(dcKnx()) << "Read knx group value from group address" << knxAddress.toString();

    QKnxTpdu tpdu;
    tpdu.setTransportControlField(QKnxTpdu::TransportControlField::DataGroup);
    tpdu.setApplicationControlField(QKnxTpdu::ApplicationControlField::GroupValueRead);

    QKnxLinkLayerFrame frame = QKnxLinkLayerFrame::builder()
            .setMessageCode(QKnxLinkLayerFrame::MessageCode::DataRequest)
            .setMedium(QKnx::MediumType::NetIP)
            .setDestinationAddress(knxAddress)
            .setTpdu(tpdu)
            .createFrame();

    requestSendFrame(frame);
}

void KnxTunnel::readKnxDpdSwitchState(const QKnxAddress &knxAddress)
{
    qCDebug(dcKnx()) << "Read switch group value" << knxAddress.toString();

    QKnxTpdu tpdu;
    tpdu.setTransportControlField(QKnxTpdu::TransportControlField::DataGroup);
    tpdu.setApplicationControlField(QKnxTpdu::ApplicationControlField::GroupValueRead);

    QKnxLinkLayerFrame frame = QKnxLinkLayerFrame::builder()
            .setMessageCode(QKnxLinkLayerFrame::MessageCode::DataRequest)
            .setMedium(QKnx::MediumType::NetIP)
            .setDestinationAddress(knxAddress)
            .setTpdu(tpdu)
            .createFrame();

    requestSendFrame(frame);
}

void KnxTunnel::readKnxDpdScalingState(const QKnxAddress &knxAddress)
{
    qCDebug(dcKnx()) << "Read scaling group value" << knxAddress.toString();

    QKnxTpdu tpdu;
    tpdu.setTransportControlField(QKnxTpdu::TransportControlField::DataGroup);
    tpdu.setApplicationControlField(QKnxTpdu::ApplicationControlField::GroupValueRead);

    QKnxLinkLayerFrame frame = QKnxLinkLayerFrame::builder()
            .setMessageCode(QKnxLinkLayerFrame::MessageCode::DataRequest)
            .setMedium(QKnx::MediumType::NetIP)
            .setDestinationAddress(knxAddress)
            .setTpdu(tpdu)
            .createFrame();

    requestSendFrame(frame);
}

void KnxTunnel::readKnxDpdTemperatureSensor(const QKnxAddress &knxAddress)
{
    qCDebug(dcKnx()) << "Read temperature sensor group value" << knxAddress.toString();

    QKnxTpdu tpdu;
    tpdu.setTransportControlField(QKnxTpdu::TransportControlField::DataGroup);
    tpdu.setApplicationControlField(QKnxTpdu::ApplicationControlField::GroupValueRead);

    QKnxLinkLayerFrame frame = QKnxLinkLayerFrame::builder()
            .setMessageCode(QKnxLinkLayerFrame::MessageCode::DataRequest)
            .setMedium(QKnx::MediumType::NetIP)
            .setDestinationAddress(knxAddress)
            .setTpdu(tpdu)
            .createFrame();

    requestSendFrame(frame);
}

void KnxTunnel::printFrame(const QKnxLinkLayerFrame &frame)
{
    qCDebug(dcKnxTunnelTraffic()) << "Frame: (" << frame.sourceAddress().toString() << "-->" << frame.destinationAddress().toString() << ")" << frame;
    qCDebug(dcKnxTunnelTraffic()) << "    Message code:" << frame.messageCode();
    qCDebug(dcKnxTunnelTraffic()) << "    MediumType" << frame.mediumType();
    qCDebug(dcKnxTunnelTraffic()) << "    Control field:" << frame.controlField();
    qCDebug(dcKnxTunnelTraffic()) << "    Extended control field:" << frame.extendedControlField();
    qCDebug(dcKnxTunnelTraffic()) << "    Additional infos:" << frame.additionalInfos();
    qCDebug(dcKnxTunnelTraffic()) << "    Bytes:" << frame.bytes().toHex().toByteArray();
    qCDebug(dcKnxTunnelTraffic()) << "    TPDU:" << frame.tpdu() << "Size:" << frame.tpdu().size();
    qCDebug(dcKnxTunnelTraffic()) << "       " << frame.tpdu().transportControlField();
    qCDebug(dcKnxTunnelTraffic()) << "       " << frame.tpdu().applicationControlField();
    qCDebug(dcKnxTunnelTraffic()) << "       " << frame.tpdu().mediumType();
    qCDebug(dcKnxTunnelTraffic()) << "        Sequence number:" << frame.tpdu().sequenceNumber();
    qCDebug(dcKnxTunnelTraffic()) << "        Data:" << frame.tpdu().data().toHex().toByteArray();
}

void KnxTunnel::requestSendFrame(const QKnxLinkLayerFrame &frame)
{
    // If the timer is running, enqueue
    if (m_queueTimer->isActive()) {
        m_sendingQueue.enqueue(frame);
        return;
    }

    // No queue, timer not running, lets send the message directly
    sendFrame(frame);
    m_queueTimer->start();
}

void KnxTunnel::sendFrame(const QKnxLinkLayerFrame &frame)
{
    if (!connected()) {
        qCWarning(dcKnx()) << "Cannot send frame on tunnel" << m_remoteAddress.toString() << " because the tunnel is not connected.";
        return;
    }

    qCDebug(dcKnxTunnelTraffic()) << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss") << "--> Sending frame" << frame << frame.destinationAddress().toString() << frame.tpdu().data().toHex().toByteArray();
    printFrame(frame);
    m_tunnel->sendFrame(frame);
}

QHostAddress KnxTunnel::getLocalAddress(const QHostAddress &remoteAddress)
{
    QHostAddress localAddress;
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        qCDebug(dcKnx()) << "Network interface" << interface.name() << interface.type();
        foreach (const QNetworkAddressEntry &addressEntry, interface.addressEntries()) {
            if (addressEntry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                qCDebug(dcKnx()) << "    - " << addressEntry.ip().toString() << addressEntry.netmask().toString();
                if (remoteAddress.isInSubnet(addressEntry.ip(), addressEntry.prefixLength())) {
                    qCDebug(dcKnx()) << "Found local interface address for" << remoteAddress.toString() << "-->" << addressEntry.ip().toString() << interface.name();
                    localAddress = addressEntry.ip();
                }
            }
        }
    }

    return localAddress;
}

void KnxTunnel::onTimeout()
{
    qCDebug(dcKnx()) << "Tunnel reconnection timeout.";
    connectTunnel();
}

void KnxTunnel::onQueueTimeout()
{
    // If queue is empty, we are don for now.
    if (m_sendingQueue.isEmpty()) {
        m_queueTimer->stop();
        return;
    }

    // Send next message
    QKnxLinkLayerFrame frame = m_sendingQueue.dequeue();
    sendFrame(frame);
    m_queueTimer->start();
}

void KnxTunnel::onTunnelConnected()
{
    qCDebug(dcKnx()) << "Tunnel connected.";
    emit connectedChanged();

    // Stop the reconnect timer
    m_timer->stop();
}

void KnxTunnel::onTunnelDisconnected()
{
    qCDebug(dcKnx()) << "Tunnel disconnected.";
    emit connectedChanged();

    // Start the reconnect timer
    m_timer->start();
}

void KnxTunnel::onTunnelStateChanged(QKnxNetIpEndpointConnection::State state)
{
    qCDebug(dcKnx()) << "Tunnel state changed" << state;
}

void KnxTunnel::onTunnelFrameReceived(const QKnxLinkLayerFrame &frame)
{
    qCDebug(dcKnxTunnelTraffic()) << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss") << "<-- Tunnel frame received" << frame << frame.destinationAddress().toString() << frame.tpdu().data().toHex().toByteArray();
    printFrame(frame);
    emit frameReceived(frame);
}

