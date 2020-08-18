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

#include "sunnywebboxcommunication.h"
#include "extern-plugininfo.h"

#include "QJsonDocument"
#include "QJsonObject"

SunnyWebBoxCommunication::SunnyWebBoxCommunication(QObject *parent) : QObject(parent)
{
    m_udpSocket = new QUdpSocket(this);
    m_udpSocket->bind(QHostAddress::LocalHost, m_port);

    connect(m_udpSocket, &QUdpSocket::stateChanged, this, [this](QAbstractSocket::SocketState state) {
        emit socketConnected(state == QAbstractSocket::SocketState::ConnectedState);
    });

    connect(m_udpSocket, &QUdpSocket::readyRead, this, [this] {

        QHostAddress address;
        quint16 port;
        QByteArray data;
        while (m_udpSocket->hasPendingDatagrams()) {
            qCDebug(dcSma()) << "Received datagram";
            int receivedBytes = m_udpSocket->readDatagram(data.data(), 1000, &address, &port);
            if (receivedBytes == -1) {
                qCWarning(dcSma()) << "Error reading pending datagram";
            }
        }
        datagramReceived(address, data);
    });
}

int SunnyWebBoxCommunication::sendMessage(const QHostAddress &address, const QString &procedure)
{
    int requestId = qrand();

    QJsonDocument doc;
    QJsonObject obj;
    obj["version"] = "1.0";
    obj["proc"] = procedure;
    obj["id"] = requestId;
    obj["format"] = "JSON";
    qCDebug(dcSma()) << "Send message" << doc.toJson() << address << m_port;
    m_udpSocket->writeDatagram(doc.toJson(), address, m_port);
    return requestId;
}

int SunnyWebBoxCommunication::sendMessage(const QHostAddress &address, const QString &procedure, const QJsonObject &params)
{
    int requestId = qrand();

    QJsonDocument doc;
    QJsonObject obj;
    obj["version"] = "1.0";
    obj["proc"] = procedure;
    obj["id"] = requestId;
    obj["format"] = "JSON";
    if (!params.isEmpty()) {
        obj.insert("params", params);
    }
    qCDebug(dcSma()) << "Send message" << doc.toJson() << address << m_port;
    m_udpSocket->writeDatagram(doc.toJson(), address, m_port);
    return requestId;
}

void SunnyWebBoxCommunication::datagramReceived(const QHostAddress &address, const QByteArray &data)
{
    qCDebug(dcSma()) << "Datagram received" << data;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcSma()) << "Could not parse JSON" << error.errorString();
        return;
    }
    if (!doc.isObject()) {
        qCWarning(dcSma()) << "JSON is not an Object";
        return;
    }
    QVariantMap map = doc.toVariant().toMap();
    if (map["version"] != "1.0") {
        qCWarning(dcSma()) << "API version not supported" << map["version"];
        return;
    }

    if (map.contains("proc") && map.contains("result")) {
        QString requestType = map["proc"].toString();
        int requestId = map["id"].toInt();
        QVariantMap result = map.value("result").toMap();
        emit messageReceived(address, requestId, requestType, result);
    } else {
        qCWarning(dcSma()) << "Missing proc or result value";
    }
}
