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

#include "sunnywebbox.h"
#include "extern-plugininfo.h"

#include "QJsonDocument"
#include "QJsonObject"

SunnyWebBox::SunnyWebBox(QUdpSocket *udpSocket, QObject *parrent) :
    QObject(parrent),
    m_udpSocket(udpSocket)
{
    connect(m_udpSocket, &QUdpSocket::stateChanged, this, [this](QAbstractSocket::SocketState state) {
        emit connectedChanged((state == QAbstractSocket::SocketState::ConnectedState));
    });

    connect(m_udpSocket, &QUdpSocket::readyRead, this, [this] {
        //m_udpSocket->readDatagram(QByteArray())
        qCDebug(dcSma()) << "Received datagram" << m_udpSocket->readAll();
    });
}

int SunnyWebBox::getPlantOverview()
{
    return sendMessage("GetPlantOverview");
}

int SunnyWebBox::getDevices()
{
    return sendMessage("GetDevices");
}

int SunnyWebBox::getProcessDataChannels(const QString &deviceId)
{
    QJsonObject params;
    params["device"] = deviceId;
    return sendMessage("GetProcessDataChannels", params);
}

int SunnyWebBox::sendMessage(const QString &procedure)
{
    int requestId = qrand();

    QJsonDocument doc;
    QJsonObject obj;
    obj["version"] = "1.0";
    obj["proc"] = procedure;
    obj["id"] = requestId;
    obj["format"] = "JSON";
    m_udpSocket->writeDatagram(doc.toJson(), m_hostAddresss, m_port);
    return requestId;
}

int SunnyWebBox::sendMessage(const QString &procedure, const QJsonObject &params)
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
    m_udpSocket->writeDatagram(doc.toJson(), m_hostAddresss, m_port);
    return requestId;
}

void SunnyWebBox::onDatagramReceived(const QByteArray &data)
{
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
        emit messageResponseReceived(requestId, requestType, result);
    } else {
        qCWarning(dcSma()) << "Missing proc or result value";
    }
}

void SunnyWebBox::parseMessageReponse(int messageId, const QString &messageType, const QVariantMap &result)
{
    if (messageType == "GetPlantOverview") {
        Overview overview;
        QVariantList overviewList = result.value("overview").toList();
        Q_FOREACH(QVariant value, overviewList) {
            QVariantMap map = value.toMap();
            if (map["meta"].toString() == "GriPwr") {
                overview.power = map["value"].toInt();
            } else if (map["meta"].toString() == "GriEgyTdy") {
                overview.dailyYield = map["value"].toInt();
            } else if (map["meta"].toString() == "GriEgyTot") {
                overview.totalYield = map["value"].toInt();
            } else if (map["meta"].toString() == "OpStt") {
                overview.status = map["value"].toString();
            } else if (map["meta"].toString() == "Msg") {
                overview.error = map["value"].toString();
            }
        }
        emit plantOverviewReceived(messageId, overview);

    } else if (messageType == "GetDevices") {
        QList<Device> devices;
        QVariantList deviceList = result.value("devices").toList();
        Q_FOREACH(QVariant value, deviceList) {
            Device device;
            QVariantMap map = value.toMap();
            device.name = map["name"].toString();
            device.key = map["key"].toString();

            QVariantList childrenList = map["children"].toList();
            Q_FOREACH(QVariant childValue, childrenList) {
                Device child;
                QVariantMap childMap = childValue.toMap();
                device.name = childMap["name"].toString();
                device.key = childMap["key"].toString();
                device.childrens.append(child);
            }
            devices.append(device);
        }
        if (!devices.isEmpty())
            emit devicesReceived(messageId, devices);
    } else if (messageType == "GetProcessDataChannels") {
    } else if (messageType == "GetProcessData") {
    } else if (messageType == "GetParameterChannels") {
    } else if (messageType == "GetParameter") {
    } else if (messageType == "SetParameter") {
    } else {
        qCWarning(dcSma()) << "Unknown message type" << messageType;
    }
}
