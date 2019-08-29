/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io         *
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

#include "devicepluginsonos.h"
#include "devices/device.h"
#include "network/networkaccessmanager.h"
#include "plugininfo.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>

DevicePluginSonos::DevicePluginSonos()
{

}


DevicePluginSonos::~DevicePluginSonos()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}


Device::DeviceSetupStatus DevicePluginSonos::setupDevice(Device *device)
{
    if (!m_pluginTimer) {

    }
    if (device->deviceClassId() == sonosConnectionDeviceClassId) {
        qCDebug(dcSonos()) << "Sonos OAuth setup complete";
        Sonos *sonos = new Sonos(hardwareManager()->networkManager(), "0a8f6d44-d9d1-4474-bcfa-cfb41f8b66e8", this);
        pluginStorage()->beginGroup(device->id().toString());
        QString username = pluginStorage()->value("username").toString();
        QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();
        m_sonosConnections.insert(device, sonos);
    }

    if (device->deviceClassId() == sonosGroupDeviceClassId) {

        //set parent ID
    }
    return Device::DeviceSetupStatusSuccess;
}

DevicePairingInfo DevicePluginSonos::pairDevice(DevicePairingInfo &devicePairingInfo)
{

    if (devicePairingInfo.deviceClassId() == sonosConnectionDeviceClassId) {
        QString clientId = "b15cbf8c-a39c-47aa-bd93-635a96e9696c";
        QString clientSecret = "c086ba71-e562-430b-a52f-867c6482fd11";

        QUrl url("https://api.sonos.com/login/v3/oauth");
        QUrlQuery queryParams;
        queryParams.addQueryItem("client_id", clientId);
        queryParams.addQueryItem("redirect_uri", "https://127.0.0.1:8888");
        queryParams.addQueryItem("response_type", "code");
        queryParams.addQueryItem("scope", "playback-control-all");
        queryParams.addQueryItem("state", QUuid::createUuid().toString());
        url.setQuery(queryParams);

        qCDebug(dcSonos()) << "Sonos url:" << url;

        devicePairingInfo.setOAuthUrl(url);
        devicePairingInfo.setStatus(Device::DeviceErrorNoError);
        return devicePairingInfo;
    }

    qCWarning(dcSonos()) << "Unhandled pairing metod!";
    devicePairingInfo.setStatus(Device::DeviceErrorCreationMethodNotSupported);
    return devicePairingInfo;
}

DevicePairingInfo DevicePluginSonos::confirmPairing(DevicePairingInfo &devicePairingInfo, const QString &username, const QString &secret)
{
    Q_UNUSED(username);
    qCDebug(dcSonos()) << "Confirm pairing";

    if (devicePairingInfo.deviceClassId() == sonosConnectionDeviceClassId) {
         qCDebug(dcSonos()) << "Secret is" << secret;
         QUrl url(secret);
         QUrlQuery query(url);
         qCDebug(dcSonos()) << "Acess code is:" << query.queryItemValue("code");

         QString accessCode = query.queryItemValue("code");

         // Obtaining access token
         url = QUrl("https://api.sonos.com/login/v3/oauth/access");
         query.clear();
         query.addQueryItem("grant_type", "authorization_code");
         query.addQueryItem("code", accessCode);
         query.addQueryItem("redirect_uri", "https%3A%2F%2F127.0.0.1%3A8888");
         url.setQuery(query);

         QNetworkRequest request(url);
         request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded;charset=utf-8");

         QByteArray clientId = "b15cbf8c-a39c-47aa-bd93-635a96e9696c";
         QByteArray clientSecret = "c086ba71-e562-430b-a52f-867c6482fd11";

         QByteArray auth = QByteArray(clientId + ':' + clientSecret).toBase64(QByteArray::Base64Encoding | QByteArray::KeepTrailingEquals);
         request.setRawHeader("Authorization", QString("Basic %1").arg(QString(auth)).toUtf8());

         QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QByteArray());
         connect(reply, &QNetworkReply::finished, this, [this, reply, devicePairingInfo](){
             reply->deleteLater();

             QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
             qCDebug(dcSonos()) << "Sonos accessToken reply:" << this << reply->error() << reply->errorString() << jsonDoc.toJson();
             qCDebug(dcSonos()) << "Access token:" << jsonDoc.toVariant().toMap().value("access_token").toString();
             qCDebug(dcSonos()) << "expires at" << QDateTime::currentDateTime().addSecs(jsonDoc.toVariant().toMap().value("expires_in").toInt()).toString();
             qCDebug(dcSonos()) << "Refresh token:" << jsonDoc.toVariant().toMap().value("refresh_token").toString();
             DevicePairingInfo info(devicePairingInfo);
             info.setStatus(Device::DeviceErrorNoError);
             emit pairingFinished(info);
         });

         devicePairingInfo.setStatus(Device::DeviceErrorAsync);
         return devicePairingInfo;
     }


    qCWarning(dcSonos()) << "Invalid deviceclassId -> no pairing possible with this device";
    devicePairingInfo.setStatus(Device::DeviceErrorHardwareFailure);
    return devicePairingInfo;
}

void DevicePluginSonos::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == sonosConnectionDeviceClassId) {

    }

    if (device->deviceClassId() == sonosGroupDeviceClassId) {

    }
}


void DevicePluginSonos::startMonitoringAutoDevices()
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == sonosGroupDeviceClassId) {
            return; // We already have a Auto Mock device... do nothing.
        }
    }
}

void DevicePluginSonos::deviceRemoved(Device *device)
{
    qCDebug(dcSonos) << "Delete " << device->name();
    if (myDevices().empty()) {
    }
}


Device::DeviceError DevicePluginSonos::executeAction(Device *device, const Action &action)
{
    Q_UNUSED(action)
    if (device->deviceClassId() == sonosGroupDeviceClassId) {
        Sonos *sonos = m_sonosConnections.value(device);
        //int groupId = device->paramValue(sonosGroupDe)
        if (!sonos)
            return Device::DeviceErrorInvalidParameter;

        if (action.actionTypeId()  == sonosGroupPlayActionTypeId) {
            sonos->play();
            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId()  == sonosGroupShuffleActionTypeId) {

            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId()  == sonosGroupRepeatActionTypeId) {

            if (action.param(sonosGroupRepeatActionRepeatParamTypeId).value().toString() == "None") {

            } else if (action.param(sonosGroupShuffleActionShuffleParamTypeId).value().toString() == "One") {

            } else if (action.param(sonosGroupShuffleActionShuffleParamTypeId).value().toString() == "All") {

            } else {
                return Device::DeviceErrorHardwareFailure;
            }


            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId() == sonosGroupPauseActionTypeId) {
            sonos->pause();
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == sonosGroupStopActionTypeId) {
            //sonos->stop();
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == sonosGroupMuteActionTypeId) {
            //bool mute = action.param(sonosGroupMuteActionMuteParamTypeId).value().toBool();

           //sonos->setGroupMute();
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == sonosGroupSkipNextActionTypeId) {
            /*if (!m_sonosSystem->GetPlayer()->Next()) {
                return Device::DeviceErrorHardwareFailure;
            }*/
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == sonosGroupSkipBackActionTypeId) {
           /* if(!m_sonosSystem->GetPlayer()->Previous()) {
                return Device::DeviceErrorHardwareFailure;
            }*/
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == sonosGroupSkipBackActionTypeId) {
            //int volume = action.param(sonosVolumeActionVolumeParamTypeId).value().toInt();
            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginSonos::onPluginTimer()
{
}

void DevicePluginSonos::onConnectionChanged()
{
    Sonos *sonos = static_cast<Sonos *>(sender());
    Device *device = m_sonosConnections.key(sonos);
    device->setStateValue(sonosConnectionConnectedStateTypeId, false); //TODO

    //TODO set all groups
}
