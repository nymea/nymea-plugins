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

#include "integrationpluginhomeconnect.h"
#include "integrations/integrationplugin.h"
#include "network/networkaccessmanager.h"
#include "plugininfo.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>

IntegrationPluginHomeConnect::IntegrationPluginHomeConnect()
{
}


IntegrationPluginHomeConnect::~IntegrationPluginHomeConnect()
{
    if (m_pluginTimer5sec)
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5sec);
    if (m_pluginTimer60sec)
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer60sec);
}

void IntegrationPluginHomeConnect::discoverThings(ThingDiscoveryInfo *info)
{
    Q_UNUSED(info)
}

void IntegrationPluginHomeConnect::startPairing(ThingPairingInfo *info)
{
    if (info->thingClassId() == homeConnectConnectionThingClassId) {

        HomeConnect *homeConnect = new HomeConnect(hardwareManager()->networkManager(), "TODO", "TODO", this);
        QUrl url = homeConnect->getLoginUrl(QUrl("https://127.0.0.1:8888"), "TODO Scope");
        qCDebug(dcHomeConnect()) << "HomeConnect url:" << url;
        info->setOAuthUrl(url);
        info->finish(Thing::ThingErrorNoError);
        m_setupHomeConnectConnections.insert(info->thingId(), homeConnect);
    } else {

        qCWarning(dcHomeConnect()) << "Unhandled pairing metod!";
        info->finish(Thing::ThingErrorCreationMethodNotSupported);
    }
}

void IntegrationPluginHomeConnect::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username);

    if (info->thingClassId() == homeConnectConnectionThingClassId) {
        qCDebug(dcHomeConnect()) << "Redirect url is" << secret;
        QUrl url(secret);
        QUrlQuery query(url);
        QByteArray authorizationCode = query.queryItemValue("code").toLocal8Bit();
        QByteArray state = query.queryItemValue("state").toLocal8Bit();
        //TODO evaluate state if it equals the given state

        HomeConnect *HomeConnect = m_setupHomeConnectConnections.value(info->thingId());

        if (!HomeConnect) {
            qWarning(dcHomeConnect()) << "No HomeConnect connection found for device:"  << info->thingName();
            m_setupHomeConnectConnections.remove(info->thingId());
            HomeConnect->deleteLater();
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }
        HomeConnect->getAccessTokenFromAuthorizationCode(authorizationCode);
        connect(HomeConnect, &HomeConnect::authenticationStatusChanged, this, [info, this](bool authenticated){
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

    } else {
        qCWarning(dcHomeConnect()) << "Invalid thingClassId -> no pairing possible with this device";
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}


void IntegrationPluginHomeConnect::setupThing(ThingSetupInfo *info)
{
    if (!m_pluginTimer5sec) {
        m_pluginTimer5sec = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_pluginTimer5sec, &PluginTimer::timeout, this, [this]() {

            foreach (Thing *connectionThing, myThings().filterByThingClassId(homeConnectConnectionThingClassId)) {
                HomeConnect *HomeConnect = m_homeConnectConnections.value(connectionThing);
                if (!HomeConnect) {
                    qWarning(dcHomeConnect()) << "No HomeConnect account found for" << connectionThing->name();
                    continue;
                }
            }
        });
    }

    if (!m_pluginTimer60sec) {
        m_pluginTimer60sec = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer60sec, &PluginTimer::timeout, this, [this]() {
            foreach (Thing *thing, myThings().filterByThingClassId(homeConnectConnectionThingClassId)) {
                HomeConnect *homeConnect = m_homeConnectConnections.value(thing);
                if (!homeConnect) {
                    qWarning(dcHomeConnect()) << "No HomeConnect account found for" << thing->name();
                    continue;
                }

            }
        });
    }

    if (info->thing()->thingClassId() == homeConnectConnectionThingClassId) {
        HomeConnect *homeConnect;
        if (m_setupHomeConnectConnections.keys().contains(thing->id())) {
            //Fresh device setup, has already a fresh access token
            qCDebug(dcHomeConnect()) << "HomeConnect OAuth setup complete";
            HomeConnect = m_setupHomeConnectConnections.take(device->id());
            connect(homeConnect, &HomeConnect::connectionChanged, this, &IntegrationPluginHomeConnect::onConnectionChanged);
            connect(homeConnect, &HomeConnect::actionExecuted, this, &IntegrationPluginHomeConnect::onActionExecuted);
            connect(homeConnect, &HomeConnect::authenticationStatusChanged, this, &IntegrationPluginHomeConnect::onAuthenticationStatusChanged);
            m_homeConnectConnections.insert(thing, homeConnect);
            return;
        } else {
            //device loaded from the device database, needs a new access token;
            pluginStorage()->beginGroup(thing->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();

            homeConnect = new HomeConnect(hardwareManager()->networkManager(), "TODO", "TODO", this);
            connect(homeConnect, &HomeConnect::connectionChanged, this, &IntegrationPluginHomeConnect::onConnectionChanged);
            connect(homeConnect, &HomeConnect::actionExecuted, this, &IntegrationPluginHomeConnect::onActionExecuted);
            connect(homeConnect, &HomeConnect::authenticationStatusChanged, this, &IntegrationPluginHomeConnect::onAuthenticationStatusChanged);
            homeConnect->getAccessTokenFromRefreshToken(refreshToken);
            m_homeConnectConnections.insert(thing, homeConnect);
            info->finish(Thing::ThingErrorNoError);
        }
    } else {
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginHomeConnect::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == homeConnectConnectionThingClassId) {
        HomeConnect *homeConnect = m_homeConnectConnections.value(thing);
        Q_UNUSED(homeConnect)
    }
}

void IntegrationPluginHomeConnect::executeAction(ThingActionInfo *info)
{

}

void IntegrationPluginHomeConnect::thingRemoved(Thing *thing)
{
    qCDebug(dcHomeConnect) << "Delete " << thing->name();
    if (myThings().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5sec);
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer60sec);
        m_pluginTimer5sec = nullptr;
        m_pluginTimer60sec = nullptr;
    }
}
Device *device

void IntegrationPluginHomeConnect::onConnectionChanged(bool connected)
{
    HomeConnect *homeConnect = static_cast<HomeConnect *>(sender());
    Thing *thing = m_homeConnectConnections.key(homeConnect);
    if (!thing)
        return;
    thing->setStateValue(homeConnectConnectionConnectedStateTypeId, connected);
}

void IntegrationPluginHomeConnect::onAuthenticationStatusChanged(bool authenticated)
{
    HomeConnect *HomeConnectConnection = static_cast<HomeConnect *>(sender());
    Thing *thing = m_homeConnectConnections.key(HomeConnectConnection);
    if (!thing)
        return;

    if (!thing->setupComplete()) {
        if (authenticated) {
            //emit deviceSetupFinished(device, Device::DeviceSetupStatusSuccess);
        } else {
            //emit deviceSetupFinished(device, Device::DeviceSetupStatusFailure);
        }
    } else {
        thing->setStateValue(homeConnectConnectionLoggedInStateTypeId, authenticated);
        if (!authenticated) {
            //refresh access token needs to be refreshed
            pluginStorage()->beginGroup(thing->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();
            HomeConnectConnection->getAccessTokenFromRefreshToken(refreshToken);
        }
    }
}

void IntegrationPluginHomeConnect::onActionExecuted(QUuid HomeConnectActionId, bool success)
{
    if (m_pendingActions.contains(HomeConnectActionId)) {
        ActionId nymeaActionId = m_pendingActions.value(HomeConnectActionId);
        if (success) {
            //emit actionExecutionFinished(nymeaActionId, Device::DeviceErrorNoError);
        } else {
            //emit actionExecutionFinished(nymeaActionId, Device::DeviceErrorHardwareFailure);
        }
    }
}
