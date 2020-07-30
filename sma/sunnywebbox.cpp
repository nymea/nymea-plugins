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
    } else if (messageType == "GetProcessDataChannels") {
    } else if (messageType == "GetProcessData") {
    } else if (messageType == "GetParameterChannels") {
    } else if (messageType == "GetParameter") {
    } else if (messageType == "SetParameter") {
    } else {
        qCWarning(dcSma()) << "Unknown message type" << messageType;
    }
}
