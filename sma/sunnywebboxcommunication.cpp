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
#include <QUuid>
#include <QTimer>
#include <QNetworkReply>
#include <QNetworkRequest>

SunnyWebBoxCommunication::SunnyWebBoxCommunication(NetworkAccessManager *networkAccessManager, QObject *parent) :
    QObject(parent),
    m_networkManager(networkAccessManager)
{
    qCDebug(dcSma()) << "Creating SunnyWebBoxCommunictaion";
    m_udpSocket = new QUdpSocket(this);
    m_udpSocket->bind(QHostAddress::Any, m_port);

    connect(m_udpSocket, &QUdpSocket::stateChanged, this, [this](QAbstractSocket::SocketState state) {
        emit socketConnected(state == QAbstractSocket::SocketState::ConnectedState);
    });

    connect(m_udpSocket, &QUdpSocket::readyRead, this, [this] {

        QHostAddress address;
        quint16 port;
        QByteArray data;
        data.resize(m_udpSocket->pendingDatagramSize());
        int receivedBytes = m_udpSocket->readDatagram(data.data(), data.size(), &address, &port);
        if (receivedBytes == -1) {
            qCWarning(dcSma()) << "Error reading pending datagram";
        }
        datagramReceived(address, data);
    });
}



QString SunnyWebBoxCommunication::sendMessage(const QHostAddress &address, const QString &procedure, const QJsonObject &params)
{
    QString requestId = QUuid::createUuid().toString().remove('{').remove('-').left(14);

    QJsonDocument doc;
    QJsonObject obj;

    obj.insert("params", params);
    obj["format"] = "JSON";
    obj["id"] = requestId;
    obj["proc"] = procedure;
    obj["version"] = "1.0";
    doc.setObject(obj);
    QByteArray data = doc.toJson(QJsonDocument::JsonFormat::Compact);
    if(!m_messageResponsePending) {
        qCDebug(dcSma()) << "Send message" << data << address << m_port;
        m_udpSocket->writeDatagram(data, address, m_port);
        m_messageResponsePending = true;
    } else {
        if (m_messageQueue[address].length() < 40) {
            qCDebug(dcSma()) << "Adding message to queue" << data << address << m_port;
            m_messageQueue[address].append(data);
        } else {
            qCDebug(dcSma()) << "Message queue overflow";
            return "";
        }
    }
    return requestId;
}

void SunnyWebBoxCommunication::datagramReceived(const QHostAddress &address, const QByteArray &data)
{
    if(!m_messageQueue.value(address).isEmpty()) {
        QByteArray data = m_messageQueue[address].takeFirst();
        qCDebug(dcSma()) << "Send message from queue" << data << address << m_port;
        //The interval between two queries should not be less than 30 seconds.
        QTimer::singleShot(3000, this, [this, data, address]{m_udpSocket->writeDatagram(data, address, m_port);});
    } else {
        m_messageResponsePending = false;
    }
    QList<QByteArray> arrayList = data.split('\x00');
    QByteArray cleanData;
    Q_FOREACH(QByteArray i, arrayList) {
        //Removing all '\0' characters
        cleanData.append(i);
    }
    qCDebug(dcSma()) << "Datagram received" << cleanData;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(cleanData, &error);
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
        QString requestId = map["id"].toString();
        QVariantMap result = map.value("result").toMap();
        emit messageReceived(address, requestId, requestType, result);
    } else {
        qCWarning(dcSma()) << "Missing proc or result value";
    }
}
