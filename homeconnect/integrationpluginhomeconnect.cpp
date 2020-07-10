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

        HomeConnect *homeConnect = new HomeConnect(hardwareManager()->networkManager(), "423713AB3EDA5B44BCE6E7B3546C43DADCB27A156C681E30455250637B2213DB", "AE182EA9F1CB99416DFD62CE61BF6DCDB3BB7D4697B58D4499D3792EC9F7412D", this);
        QUrl url = homeConnect->getLoginUrl(QUrl("https://127.0.0.1:8888"), "Monitor");
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
        //QByteArray state = query.queryItemValue("state").toLocal8Bit();
        //TODO evaluate state if it equals the given state

        HomeConnect *homeConnect = m_setupHomeConnectConnections.value(info->thingId());
        if (!homeConnect) {
            qWarning(dcHomeConnect()) << "No HomeConnect connection found for device:"  << info->thingName();
            m_setupHomeConnectConnections.remove(info->thingId());
            homeConnect->deleteLater();
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }
        qCDebug(dcHomeConnect()) << "Authorization code" << authorizationCode;
        homeConnect->getAccessTokenFromAuthorizationCode(authorizationCode);
        connect(homeConnect, &HomeConnect::authenticationStatusChanged, this, [info, this](bool authenticated){
            HomeConnect *homeConnect = static_cast<HomeConnect *>(sender());

            if(!authenticated) {
                qWarning(dcHomeConnect()) << "Authentication process failed";
                m_setupHomeConnectConnections.remove(info->thingId());
                homeConnect->deleteLater();
                info->finish(Thing::ThingErrorSetupFailed);
                return;
            }
            QByteArray accessToken = homeConnect->accessToken();
            QByteArray refreshToken = homeConnect->refreshToken();
            qCDebug(dcHomeConnect()) << "Token:" << accessToken << refreshToken;

            pluginStorage()->beginGroup(info->thingId().toString());
            pluginStorage()->setValue("refresh_token", refreshToken);
            pluginStorage()->endGroup();

            info->finish(Thing::ThingErrorNoError);
        });
    } else {
        qCWarning(dcHomeConnect()) << "Invalid thingClassId -> no pairing possible with this device";
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}


void IntegrationPluginHomeConnect::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (info->thing()->thingClassId() == homeConnectConnectionThingClassId) {
        HomeConnect *homeConnect;
        if (m_setupHomeConnectConnections.keys().contains(thing->id())) {
            //Fresh device setup, has already a fresh access token
            qCDebug(dcHomeConnect()) << "HomeConnect OAuth setup complete";
            homeConnect = m_setupHomeConnectConnections.take(thing->id());
            connect(homeConnect, &HomeConnect::connectionChanged, this, &IntegrationPluginHomeConnect::onConnectionChanged);
            connect(homeConnect, &HomeConnect::actionExecuted, this, &IntegrationPluginHomeConnect::onRequestExecuted);
            connect(homeConnect, &HomeConnect::authenticationStatusChanged, this, &IntegrationPluginHomeConnect::onAuthenticationStatusChanged);
            m_homeConnectConnections.insert(thing, homeConnect);
            return;
        } else {
            //device loaded from the device database, needs a new access token;
            pluginStorage()->beginGroup(thing->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();

            homeConnect = new HomeConnect(hardwareManager()->networkManager(), "423713AB3EDA5B44BCE6E7B3546C43DADCB27A156C681E30455250637B2213DB", "AE182EA9F1CB99416DFD62CE61BF6DCDB3BB7D4697B58D4499D3792EC9F7412D", this);
            connect(homeConnect, &HomeConnect::connectionChanged, this, &IntegrationPluginHomeConnect::onConnectionChanged);
            connect(homeConnect, &HomeConnect::actionExecuted, this, &IntegrationPluginHomeConnect::onRequestExecuted);
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

    if (thing->thingClassId() == homeConnectConnectionThingClassId) {
        HomeConnect *homeConnect = m_homeConnectConnections.value(thing);
        Q_UNUSED(homeConnect)
    }
}

void IntegrationPluginHomeConnect::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();
    if (thing->thingClassId() == homeConnectConnectionThingClassId) {
        if (action.actionTypeId() == ActionTypeId("asdf")) { //TODO

        } else {
            Q_ASSERT_X(false, "executeAction", QString("Unhandled actionTypeId: %1").arg(action.actionTypeId().toString()).toUtf8());
        }
    } else {
        Q_ASSERT_X(false, "executeAction", QString("Unhandled deviceClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginHomeConnect::thingRemoved(Thing *thing)
{
    qCDebug(dcHomeConnect) << "Delete " << thing->name();
    if (thing->thingClassId() == homeConnectConnectionThingClassId) {
        m_homeConnectConnections.take(thing)->deleteLater();
    }

    if (myThings().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5sec);
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer60sec);
        m_pluginTimer5sec = nullptr;
        m_pluginTimer60sec = nullptr;
    }
}

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
    HomeConnect *homeConnectConnection = static_cast<HomeConnect *>(sender());
    if (m_asyncSetup.contains(homeConnectConnection)) {
        ThingSetupInfo *info = m_asyncSetup.take(homeConnectConnection);
        if (authenticated) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    } else {
        Thing *thing = m_homeConnectConnections.key(homeConnectConnection);
        if (!thing)
            return;

        thing->setStateValue(homeConnectConnectionLoggedInStateTypeId, authenticated);
        if (!authenticated) {
            //refresh access token needs to be refreshed
            pluginStorage()->beginGroup(thing->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();
            homeConnectConnection->getAccessTokenFromRefreshToken(refreshToken);
        }
    }
}

void IntegrationPluginHomeConnect::onRequestExecuted(QUuid requestId, bool success)
{
    if (m_pendingActions.contains(requestId)) {
        ThingActionInfo *info = m_pendingActions.value(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
    }
}
