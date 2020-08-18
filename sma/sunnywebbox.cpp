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

SunnyWebBox::SunnyWebBox(SunnyWebBoxCommunication *communication, const QHostAddress &hostAddress,  QObject *parrent) :
    QObject(parrent),
    m_hostAddresss(hostAddress),
    m_communication(communication)
{
    //TODO connect communication with socket state;
    connect(m_communication, &SunnyWebBoxCommunication::messageReceived, this, &SunnyWebBox::onMessageReceived);
}

int SunnyWebBox::getPlantOverview()
{
    return m_communication->sendMessage(m_hostAddresss, "GetPlantOverview");
}

int SunnyWebBox::getDevices()
{
    return m_communication->sendMessage(m_hostAddresss, "GetDevices");
}

int SunnyWebBox::getProcessDataChannels(const QString &deviceId)
{
    QJsonObject params;
    params["device"] = deviceId;
    return m_communication->sendMessage(m_hostAddresss, "GetProcessDataChannels", params);
}

int SunnyWebBox::getProcessData(const QStringList &deviceKeys)
{
    QJsonObject params;
    params["device"] = deviceKeys.first(); //TODO
    return m_communication->sendMessage(m_hostAddresss, "GetProcessData", params);
}

int SunnyWebBox::getParameterChannels(const QString &deviceKey)
{
    QJsonObject paramsObj;
    QJsonArray devicesArray;
    QJsonObject deviceObj;
    deviceObj["key"] = deviceKey;
    devicesArray.append(deviceObj);
    paramsObj["devices"] = devicesArray;
    return m_communication->sendMessage(m_hostAddresss, "GetParameterChannels", paramsObj);
}

int SunnyWebBox::getParameters(const QStringList &deviceKeys)
{
    QJsonObject paramsObj;
    QJsonArray devicesArray;
    QJsonObject deviceObj;
    deviceObj["key"] = deviceKeys.first(); //TODO
    devicesArray.append(deviceObj);
    paramsObj["devices"] = devicesArray;
    return m_communication->sendMessage(m_hostAddresss, "GetParameter", paramsObj);
}

int SunnyWebBox::setParameters(const QString &deviceKey, const QHash<QString, QVariant> &channels)
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
    return m_communication->sendMessage(m_hostAddresss, "SetParameter", paramsObj);
}

void SunnyWebBox::setHostAddress(const QHostAddress &address)
{
    m_hostAddresss = address;
}

QHostAddress SunnyWebBox::hostAddress()
{
    return m_hostAddresss;
}

void SunnyWebBox::onMessageReceived(const QHostAddress &address, int messageId, const QString &messageType, const QVariantMap &result)
{
    if (address != m_hostAddresss) {
        return;
    }

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
        qCWarning(dcSma()) << "Unknown message type" << messageType;
    }
}
