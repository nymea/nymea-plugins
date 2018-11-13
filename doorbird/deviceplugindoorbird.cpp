/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "deviceplugindoorbird.h"
#include "plugininfo.h"

#include <network/upnp/upnpdiscovery.h>

#include <QNetworkAccessManager>
#include <QAuthenticator>

DevicePluginDoorbird::DevicePluginDoorbird()
{
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::authenticationRequired, this, [this](QNetworkReply *reply, QAuthenticator *authenticator) {
        Device *dev = m_networkRequests.value(reply);
        if (!myDevices().contains(dev)) {
            qCWarning(dcDoorBird) << "Credentials requested for a device which doesn't exist any more";
            return;
        }
        qCDebug(dcDoorBird) << "Credentials requested for device:" << dev->name();
        authenticator->setUser(dev->paramValue(doorBirdDeviceUsernameParamTypeId).toString());
        authenticator->setPassword(dev->paramValue(doorBirdDevicePasswordParamTypeId).toString());
    });
}

DevicePluginDoorbird::~DevicePluginDoorbird()
{

}

DeviceManager::DeviceError DevicePluginDoorbird::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(deviceClassId)
    Q_UNUSED(params)

    UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices();

    connect(reply, &UpnpDiscoveryReply::finished, this, [reply]() {
        reply->deleteLater();
        qCDebug(dcDoorBird) << "UPnP discovery reply:" << reply->error();
    });

    return DeviceManager::DeviceErrorAsync;
}

void DevicePluginDoorbird::init()
{

}

DeviceManager::DeviceSetupStatus DevicePluginDoorbird::setupDevice(Device *device)
{
    QNetworkRequest request(QString("http://%1/bha-api/monitor.cgi?ring=doorbell,motionsensor").arg(device->paramValue(doorBirdDeviceAddressParamTypeId).toString()));
    qCDebug(dcDoorBird) << "Starting monitoring" << device->name();
    QNetworkReply *reply = m_nam->get(request);
    m_networkRequests.insert(reply, device);
    connect(reply, &QNetworkReply::downloadProgress, this, [this, device, reply](qint64 bytesReceived, qint64 bytesTotal){
        if (!myDevices().contains(device)) {
            qCWarning(dcDoorBird) << "Device disappeared for monitor stream." << bytesReceived << bytesTotal;
            reply->abort();
            return;
        }
        m_readBuffers[device].append(reply->readAll());
        qCDebug(dcDoorBird) << "Monitor data for" << device->name();
        qCDebug(dcDoorBird) << m_readBuffers[device];

        // Input data looks like:
        // "--ioboundary\r\nContent-Type: text/plain\r\n\r\ndoorbell:H\r\n\r\n"

        while (!m_readBuffers[device].isEmpty()) {
            // find next --ioboundary
            QString boundary = QStringLiteral("--ioboundary");
            int startIndex = m_readBuffers[device].indexOf(boundary);
            if (startIndex == -1) {
                qCWarning(dcDoorBird) << "No meaningful data in buffer:" << m_readBuffers[device];
                if (m_readBuffers[device].size() > 1024) {
                    qCWarning(dcDoorBird) << "Buffer size > 1KB and still no meaningful data. Discarding buffer...";
                    m_readBuffers[device].clear();
                }
                // Assuming we don't have enough data yet...
                return;
            }

            QByteArray contentType = QByteArrayLiteral("Content-Type: text/plain");
            int contentTypeIndex = m_readBuffers[device].indexOf(contentType);
            if (contentTypeIndex == -1) {
                qCWarning(dcDoorBird) << "Cannot find Content-Type in buffer:" << m_readBuffers[device];
                if (m_readBuffers[device].size() > startIndex + 50) {
                    qCWarning(dcDoorBird) << boundary << "found but unexpected data follows. Skipping this element...";
                    m_readBuffers[device].remove(0, startIndex + boundary.length());
                    continue;
                }
                // Assuming we don't have enough data yet...
                return;
            }

            // At this point we have the boundary and Content-Type. Remove all of that and take the entire string to either end or next boundary
            m_readBuffers[device].remove(0, contentTypeIndex + contentType.length());
            int nextStartIndex = m_readBuffers[device].indexOf(boundary);
            QByteArray data;
            if (nextStartIndex == -1) {
                data = m_readBuffers[device];
                m_readBuffers[device].clear();
            } else {
                data = m_readBuffers[device].left(nextStartIndex);
                m_readBuffers[device].remove(0, nextStartIndex);
            }

            QString message = data.trimmed();
            QStringList parts = message.split(":");
            if (parts.count() != 2) {
                qCWarning(dcDoorBird) << "Message has invalid format:" << message << "Expected device:state";
                continue;
            }
            if (parts.first() == "doorbell") {
                if (parts.at(1) == "H") {
                    qCDebug(dcDoorBird) << "Doorbell ringing!";
                    // TODO: emit event
                }
            } else if (parts.first() == "motionsensor") {
                if (parts.at(1) == "H") {
                    qCDebug(dcDoorBird) << "Motion sensor detected a person";
                    // TODO: emit event
                }
            } else {
                qCWarning(dcDoorBird) << "Unhandled DoorBird data:" << message;
            }
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, device, reply](){
        reply->deleteLater();
        m_networkRequests.remove(reply);
        qCDebug(dcDoorBird) << "Monitor request finished:" << reply->error();
    });
    return DeviceManager::DeviceSetupStatusSuccess;
}

DeviceManager::DeviceError DevicePluginDoorbird::executeAction(Device *device, const Action &action)
{
    if (action.actionTypeId() == doorBirdUnlatchActionTypeId) {
        QNetworkRequest request(QString("http://%1/bha-api/open-door.cgi?r=1").arg(device->paramValue(doorBirdDeviceAddressParamTypeId).toString()));
        qCDebug(dcDoorBird) << "Sending request:" << request.url();
        QNetworkReply *reply = m_nam->get(request);
        m_networkRequests.insert(reply, device);
        connect(reply, &QNetworkReply::finished, this, [this, reply, device, action](){
            reply->deleteLater();
            m_networkRequests.remove(reply);
            if (!myDevices().contains(device)) {
                // Device must have been removed in the meantime
                return;
            }
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcDoorBird) << "Error unlatching DoorBird device" << device->name();
                emit actionExecutionFinished(action.id(), DeviceManager::DeviceErrorHardwareFailure);
                return;
            }
            qCDebug(dcDoorBird) << "DoorBird unlatched:" << reply->error() << reply->errorString();
            emit actionExecutionFinished(action.id(), DeviceManager::DeviceErrorNoError);
        });
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}
