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

#include "integrationpluginmiele.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QUrl>
#include <QUrlQuery>

IntegrationPluginMiele::IntegrationPluginMiele()
{
    m_idParamTypeIds.insert(ovenThingClassId, ovenThingIdParamTypeId);
    m_idParamTypeIds.insert(fridgeThingClassId, fridgeThingIdParamTypeId);
    m_idParamTypeIds.insert(dryerThingClassId, dryerThingIdParamTypeId);
    m_idParamTypeIds.insert(coffeeMakerThingClassId, coffeeMakerThingIdParamTypeId);
    m_idParamTypeIds.insert(dishwasherThingClassId, dishwasherThingIdParamTypeId);
    m_idParamTypeIds.insert(washerThingClassId, washerThingIdParamTypeId);
    m_idParamTypeIds.insert(cookTopThingClassId, cookTopThingIdParamTypeId);
    m_idParamTypeIds.insert(hoodThingClassId, hoodThingIdParamTypeId);
    m_idParamTypeIds.insert(cleaningRobotThingClassId, cleaningRobotThingIdParamTypeId);
}

void IntegrationPluginMiele::startPairing(ThingPairingInfo *info)
{
    if (info->thingClassId() == mieleAccountThingClassId) {

        QByteArray clientId = configValue(mielePluginCustomClientIdParamTypeId).toByteArray();
        if (clientId.isEmpty()) {
            clientId = apiKeyStorage()->requestKey("miele").data("clientKey");
        }
        if (clientId.isEmpty()) {
            info->finish(Thing::ThingErrorAuthenticationFailure, tr("Client key and/or seceret is not available."));
            return;
        }
        Miele *miele = new Miele(hardwareManager()->networkManager(), clientId, this);
        QUrl url = miele->getLoginUrl(QUrl("https://127.0.0.1:8888"));
        qCDebug(dcMiele()) << "HomeConnect url:" << url;
        m_setupMieleConnections.insert(info->thingId(), miele);
        info->setOAuthUrl(url);
        info->finish(Thing::ThingErrorNoError);
    } else {
        qCWarning(dcMiele()) << "Unhandled pairing metod!";
        info->finish(Thing::ThingErrorCreationMethodNotSupported);
    }
}

void IntegrationPluginMiele::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username);

    if (info->thingClassId() == mieleAccountThingClassId) {
        qCDebug(dcMiele()) << "Redirect url is" << secret;
        QUrl url(secret);
        QUrlQuery query(url);
        QByteArray authorizationCode = query.queryItemValue("code").toLocal8Bit();

        Miele *miele = m_setupMieleConnections.value(info->thingId());
        if (!miele) {
            qWarning(dcMiele()) << "No Miele connection found for device:"  << info->thingName();
            m_setupMieleConnections.remove(info->thingId());
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }
        qCDebug(dcMiele()) << "Authorization code" << authorizationCode;
        miele->getAccessTokenFromAuthorizationCode(authorizationCode);
        connect(miele, &Miele::receivedRefreshToken, info, [info, this](const QByteArray &refreshToken){
            qCDebug(dcMiele()) << "Token:" << refreshToken;

            pluginStorage()->beginGroup(info->thingId().toString());
            pluginStorage()->setValue("refresh_token", refreshToken);
            pluginStorage()->endGroup();

            info->finish(Thing::ThingErrorNoError);
        });
    } else {
        Q_ASSERT_X(false, "confirmPairing", QString("Unhandled thingClassId: %1").arg(info->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginMiele::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == mieleAccountThingClassId) {
        Miele *miele;
        if (m_setupMieleConnections.keys().contains(thing->id())) {
            //Fresh device setup, has already a fresh access token
            qCDebug(dcMiele()) << "Miele OAuth setup complete";
            miele = m_setupMieleConnections.take(thing->id());
            m_mieleConnections.insert(thing, miele);
            info->finish(Thing::ThingErrorNoError);
        } else {
            //device loaded from the device database, needs a new access token;
            pluginStorage()->beginGroup(thing->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();
            if (refreshToken.isEmpty()) {
                info->finish(Thing::ThingErrorAuthenticationFailure, tr("Refresh token is not available."));
                return;
            }
            QByteArray clientId = configValue(mielePluginCustomClientIdParamTypeId).toByteArray();
            if (clientId.isEmpty()) {
                clientId = apiKeyStorage()->requestKey("miele").data("clientId");
            }
            if (clientId.isEmpty()) {
                info->finish(Thing::ThingErrorAuthenticationFailure, tr("Client id is not available."));
                return;
            }
            miele = new Miele(hardwareManager()->networkManager(), clientId, this);
            miele->getAccessTokenFromRefreshToken(refreshToken);
            m_asyncSetup.insert(miele, info);
        }
    } else if (m_idParamTypeIds.contains(thing->thingClassId())) {
        Thing *parentThing = myThings().findById(thing->parentId());
        if (parentThing->setupComplete()) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            connect(parentThing, &Thing::setupStatusChanged, info, [parentThing, info]{
                if (parentThing->setupComplete()) {
                    info->finish(Thing::ThingErrorNoError);
                }
            });
        }
    } else {
        Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }

}

void IntegrationPluginMiele::postSetupThing(Thing *thing)
{
    if (!m_pluginTimer15min) {
        m_pluginTimer15min = hardwareManager()->pluginTimerManager()->registerTimer(60*15);
        connect(m_pluginTimer15min, &PluginTimer::timeout, this, [this]() {
            Q_FOREACH (Thing *thing, myThings().filterByThingClassId(mieleAccountThingClassId)) {
                Miele *miele = m_mieleConnections.value(thing);
                if (!miele) {
                    qWarning(dcMiele()) << "No Miele account found for" << thing->name();
                    continue;
                }
                miele->getDevices();
                Q_FOREACH (Thing *childThing, myThings().filterByParentId(thing->id())) {
                    QString deviceId = childThing->paramValue(m_idParamTypeIds.value(childThing->thingClassId())).toString();
                    miele->getDevice(deviceId);
                }
            }
        });
    }

    if (thing->thingClassId() == mieleAccountThingClassId) {
           Miele *miele = m_mieleConnections.value(thing);
           miele->getDevices();
           //miele->connectEventStream();
           thing->setStateValue(mieleAccountConnectedStateTypeId, true);
           thing->setStateValue(mieleAccountLoggedInStateTypeId, true);
           //TBD Set user name
       } else if (m_idParamTypeIds.contains(thing->thingClassId())) {
           Thing *parentThing = myThings().findById(thing->parentId());
           if (!parentThing)
               qCWarning(dcMiele()) << "Could not find parent with Id" << thing->parentId().toString();
           Miele *miele = m_mieleConnections.value(parentThing);
           QString deviceId = thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString();
           if (!miele) {
               qCWarning(dcMiele()) << "Could not find HomeConnect connection for thing" << thing->name();
           } else {
               miele->getDevice(deviceId);
           }
       } else {
           Q_ASSERT_X(false, "postSetupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
       }
}

void IntegrationPluginMiele::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();
    Miele *miele = m_mieleConnections.value(myThings().findById(thing->parentId()));
    if (!miele) {
        return info->finish(Thing::ThingErrorHardwareNotAvailable);
    }
    QString haid = thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString();

    if (thing->thingClassId() == ovenThingClassId) {

    } else if (thing->thingClassId() == ovenThingClassId) {

    } else {

    }
}

void IntegrationPluginMiele::thingRemoved(Thing *thing)
{
    qCDebug(dcMiele) << "Delete " << thing->name();

    if (thing->thingClassId() == mieleAccountThingClassId) {
        m_mieleConnections.take(thing)->deleteLater();
    }

    if (myThings().empty()) {
        if (m_pluginTimer15min) {
            hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer15min);
            m_pluginTimer15min = nullptr;
        }
    }
}


