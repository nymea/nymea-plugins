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

IntegrationPluginMiele::IntegrationPluginMiele()
{

}

void IntegrationPluginMiele::startPairing(ThingPairingInfo *info)
{
    if (info->thingClassId() == mieleAccountThingClassId) {

        QByteArray clientKey = configValue(mielePluginCustomClientKeyParamTypeId).toByteArray();
        QByteArray clientSecret = configValue(mielePluginCustomClientSecretParamTypeId).toByteArray();
        if (clientKey.isEmpty() || clientSecret.isEmpty()) {
            clientKey = apiKeyStorage()->requestKey("miele").data("clientKey");
            clientSecret = apiKeyStorage()->requestKey("miele").data("clientSecret");
        }
        if (clientKey.isEmpty() || clientSecret.isEmpty()) {
            info->finish(Thing::ThingErrorAuthenticationFailure, tr("Client key and/or seceret is not available."));
            return;
        }
        Miele *miele = new Miele(hardwareManager()->networkManager(), clientKey, clientSecret, this);
        QUrl url = miele->getLoginUrl(QUrl("https://127.0.0.1:8888"), scope);
        qCDebug(dcMiele()) << "HomeConnect url:" << url;
        m_setupHomeConnectConnections.insert(info->thingId(), miele);
        info->setOAuthUrl(url);
        info->finish(Thing::ThingErrorNoError);
    } else {
        qCWarning(dMiele()) << "Unhandled pairing metod!";
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


