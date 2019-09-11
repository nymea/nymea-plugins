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

#include "devicepluginhomeconnect.h"
#include "devices/device.h"
#include "network/networkaccessmanager.h"
#include "plugininfo.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>

DevicePluginHomeConnect::DevicePluginHomeConnect()
{
}


DevicePluginHomeConnect::~DevicePluginHomeConnect()
{
    if (m_pluginTimer5sec)
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5sec);
    if (m_pluginTimer60sec)
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer60sec);
}


Device::DeviceSetupStatus DevicePluginHomeConnect::setupDevice(Device *device)
{
    if (!m_pluginTimer5sec) {
        m_pluginTimer5sec = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_pluginTimer5sec, &PluginTimer::timeout, this, [this]() {

            foreach (Device *connectionDevice, myDevices().filterByDeviceClassId(homeConnectConnectionDeviceClassId)) {
                HomeConnect *HomeConnect = m_homeConnectConnections.value(connectionDevice);
                if (!HomeConnect) {
                    qWarning(dcHomeConnect()) << "No HomeConnect connection found to device" << connectionDevice->name();
                    continue;
                }
            }
        });
    }

    if (!m_pluginTimer60sec) {
        m_pluginTimer60sec = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer60sec, &PluginTimer::timeout, this, [this]() {
            foreach (Device *device, myDevices().filterByDeviceClassId(homeConnectConnectionDeviceClassId)) {
                HomeConnect *homeConnect = m_homeConnectConnections.value(device);
                if (!homeConnect) {
                    qWarning(dcHomeConnect()) << "No HomeConnect connection found to device" << device->name();
                    continue;
                }

            }
        });
    }

    if (device->deviceClassId() == homeConnectConnectionDeviceClassId) {
        HomeConnect *homeConnect;
        if (m_setupHomeConnectConnections.keys().contains(device->id())) {
            //Fresh device setup, has already a fresh access token
            qCDebug(dcHomeConnect()) << "HomeConnect OAuth setup complete";
            HomeConnect = m_setupHomeConnectConnections.take(device->id());
            connect(homeConnect, &HomeConnect::connectionChanged, this, &DevicePluginHomeConnect::onConnectionChanged);
            connect(homeConnect, &HomeConnect::actionExecuted, this, &DevicePluginHomeConnect::onActionExecuted);
            connect(homeConnect, &HomeConnect::authenticationStatusChanged, this, &DevicePluginHomeConnect::onAuthenticationStatusChanged);
            m_homeConnectConnections.insert(device, homeConnect);
            return Device::DeviceSetupStatusSuccess;
        } else {
            //device loaded from the device database, needs a new access token;
            pluginStorage()->beginGroup(device->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();

            homeConnect = new HomeConnect(hardwareManager()->networkManager(), "TODO", "TODO", this);
            connect(homeConnect, &HomeConnect::connectionChanged, this, &DevicePluginHomeConnect::onConnectionChanged);
            connect(homeConnect, &HomeConnect::actionExecuted, this, &DevicePluginHomeConnect::onActionExecuted);
            connect(homeConnect, &HomeConnect::authenticationStatusChanged, this, &DevicePluginHomeConnect::onAuthenticationStatusChanged);
            HomeConnect->getAccessTokenFromRefreshToken(refreshToken);
            m_homeConnectConnections.insert(device, homeConnect);
            return Device::DeviceSetupStatusAsync;
        }
    }
    return Device::DeviceSetupStatusFailure;
}

DevicePairingInfo DevicePluginHomeConnect::pairDevice(DevicePairingInfo &devicePairingInfo)
{
    if (devicePairingInfo.deviceClassId() == homeConnectConnectionDeviceClassId) {

        HomeConnect *HomeConnect = new HomeConnect(hardwareManager()->networkManager(), "TODO", "TODO", this);
        QUrl url = homeConnect->getLoginUrl(QUrl("https://127.0.0.1:8888"), "TODO Scope");
        qCDebug(dcHomeConnect()) << "HomeConnect url:" << url;
        devicePairingInfo.setOAuthUrl(url);
        devicePairingInfo.setStatus(Device::DeviceErrorNoError);
        m_setupHomeConnectConnections.insert(devicePairingInfo.deviceId(), HomeConnect);
        return devicePairingInfo;
    }

    qCWarning(dcHomeConnect()) << "Unhandled pairing metod!";
    devicePairingInfo.setStatus(Device::DeviceErrorCreationMethodNotSupported);
    return devicePairingInfo;
}

DevicePairingInfo DevicePluginHomeConnect::confirmPairing(DevicePairingInfo &devicePairingInfo, const QString &username, const QString &secret)
{
    Q_UNUSED(username);

    if (devicePairingInfo.deviceClassId() == homeConnectConnectionDeviceClassId) {
        qCDebug(dcHomeConnect()) << "Redirect url is" << secret;
        QUrl url(secret);
        QUrlQuery query(url);
        QByteArray authorizationCode = query.queryItemValue("code").toLocal8Bit();
        QByteArray state = query.queryItemValue("state").toLocal8Bit();
        //TODO evaluate state if it equals the given state

        HomeConnect *HomeConnect = m_setupHomeConnectConnections.value(devicePairingInfo.deviceId());

        if (!HomeConnect) {
            qWarning(dcHomeConnect()) << "No HomeConnect connection found for device:"  << devicePairingInfo.deviceName();
            m_setupHomeConnectConnections.remove(devicePairingInfo.deviceId());
            HomeConnect->deleteLater();
            devicePairingInfo.setStatus(Device::DeviceErrorHardwareFailure);
            return devicePairingInfo;
        }
        HomeConnect->getAccessTokenFromAuthorizationCode(authorizationCode);
        connect(HomeConnect, &HomeConnect::authenticationStatusChanged, this, [devicePairingInfo, this](bool authenticated){
            HomeConnect *homeConnect = static_cast<HomeConnect *>(sender());
            DevicePairingInfo info(devicePairingInfo);
            if(!authenticated) {
                qWarning(dcHomeConnect()) << "Authentication process failed"  << devicePairingInfo.deviceName();
                m_setupHomeConnectConnections.remove(info.deviceId());
                homeConnect->deleteLater();
                info.setStatus(Device::DeviceErrorSetupFailed);
                emit pairingFinished(info);
                return;
            }
            QByteArray accessToken = homeConnect->accessToken();
            QByteArray refreshToken = homeConnect->refreshToken();
            qCDebug(dcHomeConnect()) << "Token:" << accessToken << refreshToken;

            pluginStorage()->beginGroup(info.deviceId().toString());
            pluginStorage()->setValue("refresh_token", refreshToken);
            pluginStorage()->endGroup();

            info.setStatus(Device::DeviceErrorNoError);
            emit pairingFinished(info);
        });
        devicePairingInfo.setStatus(Device::DeviceErrorAsync);
        return devicePairingInfo;
    }
    qCWarning(dcHomeConnect()) << "Invalid deviceclassId -> no pairing possible with this device";
    devicePairingInfo.setStatus(Device::DeviceErrorHardwareFailure);
    return devicePairingInfo;
}

void DevicePluginHomeConnect::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == homeConnectConnectionDeviceClassId) {
        HomeConnect *homeConnect = m_HomeConnectConnections.value(device);
        Q_UNUSED(homeConnect)
    }
}


void DevicePluginHomeConnect::startMonitoringAutoDevices()
{

}

void DevicePluginHomeConnect::deviceRemoved(Device *device)
{
    qCDebug(dcHomeConnect) << "Delete " << device->name();
    if (myDevices().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5sec);
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer60sec);
        m_pluginTimer5sec = nullptr;
        m_pluginTimer60sec = nullptr;
    }
}


Device::DeviceError DevicePluginHomeConnect::executeAction(Device *device, const Action &action)
{
    Q_UNUSED(device)
    Q_UNUSED(action)
    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginHomeConnect::onConnectionChanged(bool connected)
{
    HomeConnect *HomeConnect = static_cast<HomeConnect *>(sender());
    Device *device = m_homeConnectConnections.key(HomeConnect);
    if (!device)
        return;
    device->setStateValue(homeConnectConnectionConnectedStateTypeId, connected);
}

void DevicePluginHomeConnect::onAuthenticationStatusChanged(bool authenticated)
{
    HomeConnect *HomeConnectConnection = static_cast<HomeConnect *>(sender());
    Device *device = m_homeConnectConnections.key(HomeConnectConnection);
    if (!device)
        return;

    if (!device->setupComplete()) {
        if (authenticated) {
            emit deviceSetupFinished(device, Device::DeviceSetupStatusSuccess);
        } else {
            emit deviceSetupFinished(device, Device::DeviceSetupStatusFailure);
        }
    } else {
        device->setStateValue(homeConnectConnectionLoggedInStateTypeId, authenticated);
        if (!authenticated) {
            //refresh access token needs to be refreshed
            pluginStorage()->beginGroup(device->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();
            HomeConnectConnection->getAccessTokenFromRefreshToken(refreshToken);
        }
    }
}

void DevicePluginHomeConnect::onActionExecuted(QUuid HomeConnectActionId, bool success)
{
    if (m_pendingActions.contains(HomeConnectActionId)) {
        ActionId nymeaActionId = m_pendingActions.value(HomeConnectActionId);
        if (success) {
            emit actionExecutionFinished(nymeaActionId, Device::DeviceErrorNoError);
        } else {
            emit actionExecutionFinished(nymeaActionId, Device::DeviceErrorHardwareFailure);
        }
    }
}
