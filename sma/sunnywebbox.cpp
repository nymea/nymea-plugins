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
#include "QJsonArray"

SunnyWebBox::SunnyWebBox(NetworkAccessManager *networkAccessManager, const QHostAddress &hostAddress,  QObject *parrent) :
    QObject(parrent),
    m_hostAddresss(hostAddress),
    m_networkManager(networkAccessManager)
{
    qCDebug(dcSma()) << "SunnyWebBox: Creating Sunny Web Box connection";
}

SunnyWebBox::~SunnyWebBox()
{
    qCDebug(dcSma()) << "SunnyWebBox: Deleting Sunny Web Box connection";
}

QString SunnyWebBox::getPlantOverview()
{
    return sendMessage(m_hostAddresss, "GetPlantOverview");
}

QString SunnyWebBox::getDevices()
{
    return sendMessage(m_hostAddresss, "GetDevices");
}

QString SunnyWebBox::getProcessDataChannels(const QString &deviceId)
{
    QJsonObject params;
    params["device"] = deviceId;
    return sendMessage(m_hostAddresss, "GetProcessDataChannels", params);
}

QString SunnyWebBox::getProcessData(const QStringList &deviceKeys)
{
    QJsonObject params;
    params["device"] = deviceKeys.first();
    return sendMessage(m_hostAddresss, "GetProcessData", params);
}

QString SunnyWebBox::getParameterChannels(const QString &deviceKey)
{
    QJsonObject paramsObj;
    QJsonArray devicesArray;
    QJsonObject deviceObj;
    deviceObj["key"] = deviceKey;
    devicesArray.append(deviceObj);
    paramsObj["devices"] = devicesArray;
    return sendMessage(m_hostAddresss, "GetParameterChannels", paramsObj);
}

QString SunnyWebBox::getParameters(const QStringList &deviceKeys)
{
    QJsonObject paramsObj;
    QJsonArray devicesArray;
    QJsonObject deviceObj;
    deviceObj["key"] = deviceKeys.first(); //TODO
    devicesArray.append(deviceObj);
    paramsObj["devices"] = devicesArray;
    return sendMessage(m_hostAddresss, "GetParameter", paramsObj);
}

QString SunnyWebBox::setParameters(const QString &deviceKey, const QHash<QString, QVariant> &channels)
{
    QJsonObject paramsObj;
    QJsonArray devicesArray;
    QJsonObject deviceObj;
    deviceObj["key"] = deviceKey;
    QJsonArray channelsArray;
    Q_FOREACH(QString key, channels.keys()) {
        QJsonObject channelObj;
        channelObj["meta"] = key;
        channelObj["value"] = channels.value(key).toString();
        channelsArray.append(channelObj);
    }
    deviceObj["channels"] = channelsArray;
    devicesArray.append(deviceObj);
    paramsObj["devices"] = devicesArray;
    return sendMessage(m_hostAddresss, "SetParameter", paramsObj);
}

void SunnyWebBox::setHostAddress(const QHostAddress &address)
{
    qCDebug(dcSma()) << "SunnyWebBox: Setting host address to" << address.toString();
    m_hostAddresss = address;
}

QHostAddress SunnyWebBox::hostAddress()
{
    return m_hostAddresss;
}

void SunnyWebBox::parseMessage(const QString &messageId, const QString &messageType, const QVariantMap &result)
{
    if (messageType == "GetPlantOverview") {
        Overview overview;
        QVariantList overviewList = result.value("overview").toList();
        qCDebug(dcSma()) << "SunnyWebBox: GetPlantOverview";
        Q_FOREACH(QVariant value, overviewList) {
            QVariantMap map = value.toMap();

            if (map["meta"].toString() == "GriPwr") {
                overview.power = map["value"].toString().toInt();
                QString unit = map["unit"].toString();
                qCDebug(dcSma()) << "SunnyWebBox:       - Power" << overview.power << unit;
            } else if (map["meta"].toString() == "GriEgyTdy") {
                overview.dailyYield = map["value"].toString().toDouble();
                QString unit = map["unit"].toString();
                qCDebug(dcSma()) << "SunnyWebBox:       - Daily yield" << overview.dailyYield << unit;
            } else if (map["meta"].toString() == "GriEgyTot") {
                overview.totalYield = map["value"].toString().toDouble();
                QString unit = map["unit"].toString();
                qCDebug(dcSma()) << "SunnyWebBox:       - Total yield" << overview.totalYield << unit;
            } else if (map["meta"].toString() == "OpStt") {
                overview.status = map["value"].toString();
                qCDebug(dcSma()) << "SunnyWebBox:       - Status" << overview.status;
            } else if (map["meta"].toString() == "Msg") {
                overview.error = map["value"].toString();
                qCDebug(dcSma()) << "SunnyWebBox:       - Error" << overview.error;
            }
        }
        emit plantOverviewReceived(messageId, overview);

    } else if (messageType == "GetDevices") {
        QList<Device> devices;
        QVariantList deviceList = result.value("devices").toList();
        qCDebug(dcSma()) << "SunnyWebBox: GetDevices" << result.value("totalDevicesReturned").toInt();
        Q_FOREACH(QVariant value, deviceList) {
            Device device;
            QVariantMap map = value.toMap();
            device.name = map["name"].toString();
            qCDebug(dcSma()) << "SunnyWebBox:       - Name" << device.name;
            device.key = map["key"].toString();
            qCDebug(dcSma()) << "SunnyWebBox:       - Key" << device.key;
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
    } else if (messageType == "GetProcessDataChannels" ||
               messageType == "GetProDataChannels") {
        Q_FOREACH(QString deviceKey, result.keys()) {
            QStringList processDataChannels = result.value(deviceKey).toStringList();
            if (!processDataChannels.isEmpty())
                emit processDataChannelsReceived(messageId, deviceKey, processDataChannels);
        }
    } else if (messageType == "GetProcessData") {
        QList<Device> devices;
        QVariantList devicesList = result.value("devices").toList();
        Q_FOREACH(QVariant value, devicesList) {

            QString key = value.toMap().value("key").toString();
            QVariantList channelsList =  value.toMap().value("channels").toList();
            QHash<QString, QVariant> channels;
            Q_FOREACH(QVariant channel, channelsList) {
                channels.insert(channel.toMap().value("meta").toString(), channel.toMap().value("value"));
            }
            emit processDataReceived(messageId, key, channels);
        }
    } else if (messageType == "GetParameterChannels") {
        Q_FOREACH(QString deviceKey, result.keys()) {
            QStringList parameterChannels = result.value(deviceKey).toStringList();
            if (!parameterChannels.isEmpty())
                emit parameterChannelsReceived(messageId, deviceKey, parameterChannels);
        }
    } else if (messageType == "GetParameter"|| messageType == "SetParameter") {
        QList<Device> devices;
        QVariantList devicesList = result.value("devices").toList();
        Q_FOREACH(QVariant value, devicesList) {

            QString key = value.toMap().value("key").toString();
            QVariantList channelsList =  value.toMap().value("channels").toList();
            QList<Parameter> parameters;
            Q_FOREACH(QVariant channel, channelsList) {
               Parameter parameter;
               parameter.meta = channel.toMap().value("meta").toString();
               parameter.name = channel.toMap().value("name").toString();
               parameter.unit = channel.toMap().value("unit").toString();
               parameter.min = channel.toMap().value("min").toDouble();
               parameter.max = channel.toMap().value("max").toDouble();
               parameter.value = channel.toMap().value("value").toDouble();
               parameters.append(parameter);
            }
            emit parametersReceived(messageId, key, parameters);
        }
    } else {
        qCWarning(dcSma()) << "SunnyWebBox: Unknown message type" << messageType;
    }
}

void SunnyWebBox::setConnectionStatus(bool connected)
{
    if (m_connected != connected) {
        qCDebug(dcSma()) << "SunnyWebBox: Connection status changed" << connected;
        m_connected = connected;
        emit connectedChanged(m_connected);
    }
}

QString SunnyWebBox::sendMessage(const QHostAddress &address, const QString &procedure)
{
    return sendMessage(address, procedure, QJsonObject());
}

QString SunnyWebBox::sendMessage(const QHostAddress &address, const QString &procedure, const QJsonObject &params)
{
    qCDebug(dcSma()) << "SunnyWebBox: Send message to" << address.toString() << "Procedure:" << procedure << "Params:" << params;
    QString requestId = QUuid::createUuid().toString().remove('{').remove('-').left(14);

    QJsonDocument doc;
    QJsonObject obj;
    obj["format"] = "JSON";
    obj["id"] = requestId;
    obj["proc"] = procedure;
    obj["version"] = "1.0";

    if (!params.isEmpty()) {
        obj.insert("params", params);
    }
    doc.setObject(obj);

    QUrl url;
    url.setHost(address.toString());
    url.setPath("/rpc");
    url.setPort(80);
    url.setScheme("http");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    QByteArray data = doc.toJson(QJsonDocument::JsonFormat::Compact);
    data.prepend("RPC=");
    QNetworkReply *reply = m_networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, address, requestId, reply]{

        if (reply->error() != QNetworkReply::NoError) {
            setConnectionStatus(false);
            return;
        }
        setConnectionStatus(true);

        QByteArray data = reply->readAll();
        qCDebug(dcSma()) << "SunnyWebBox: Received reply" << data;

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcSma()) << "SunnyWebBox: Could not parse JSON" << error.errorString();
            return;
        }
        if (!doc.isObject()) {
            qCWarning(dcSma()) << "SunnyWebBox: JSON is not an Object";
            return;
        }
        QVariantMap map = doc.toVariant().toMap();
        if (map["version"] != "1.0") {
            qCWarning(dcSma()) << "SunnyWebBox: API version not supported" << map["version"];
            return;
        }

        if (map.contains("proc") && map.contains("result")) {
            QString requestType = map["proc"].toString();
            QString requestId = map["id"].toString();
            QVariantMap result = map.value("result").toMap();
            parseMessage(requestId, requestType, result);
        } else {
            qCWarning(dcSma()) << "SunnyWebBox: Missing proc or result value";
        }
    });
    return requestId;
}

