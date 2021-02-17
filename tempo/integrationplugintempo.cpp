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

#include "integrationplugintempo.h"
#include "plugininfo.h"

#include "network/networkaccessmanager.h"

#include <QTimer>
#include <QUrlQuery>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>

IntegrationPluginTempo::IntegrationPluginTempo()
{
}

void IntegrationPluginTempo::startPairing(ThingPairingInfo *info)
{
    qCDebug(dcTempo()) << "Start pairing";

    if (info->thingClassId() == tempoConnectionThingClassId) {

        QByteArray clientId = configValue(tempoPluginCustomClientIdParamTypeId).toByteArray();
        QByteArray clientSecret = configValue(tempoPluginCustomClientSecretParamTypeId).toByteArray();
        if (clientId.isEmpty() || clientSecret.isEmpty()) {
            clientId = apiKeyStorage()->requestKey("tempo").data("clientId");
            clientSecret = apiKeyStorage()->requestKey("tempo").data("clientSecret");
        } else {
            qCDebug(dcTempo()) << "Using custom client secret and id";
        }
        if (clientId.isEmpty() || clientSecret.isEmpty()) {
            info->finish(Thing::ThingErrorAuthenticationFailure, tr("Client id and/or seceret is not available."));
            return;
        }

        QString jiraCloudInstanceName = info->params().paramValue(tempoConnectionAtlassianAccountParamTypeId).toString();
        QUrl url = Tempo::getLoginUrl(QUrl("https://127.0.0.1:8888"), jiraCloudInstanceName, clientId);
        qCDebug(dcTempo()) << "Checking if the Tempo server is reachable";
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [reply, info, url, this] {

            if (reply->error() != QNetworkReply::NetworkError::HostNotFoundError) {
                qCDebug(dcTempo()) << "Tempo server is reachable";
                info->setOAuthUrl(url);
                info->finish(Thing::ThingErrorNoError);
            } else {
                qCWarning(dcTempo()) << "Got online check error" << reply->error() << reply->errorString();
                info->finish(Thing::ThingErrorSetupFailed, tr("Tempo server not reachable, please check the internet connection"));
            }
        });
    } else {
        qCWarning(dcTempo()) << "Unhandled pairing method!";
        info->finish(Thing::ThingErrorCreationMethodNotSupported);
    }
}

void IntegrationPluginTempo::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username);
    qCDebug(dcTempo()) << "Confirm pairing";
    if (info->thingClassId() == tempoConnectionThingClassId) {

        QUrl url(secret);
        QUrlQuery query(url);
        QByteArray authorizationCode = query.queryItemValue("code").toLocal8Bit();
        if (authorizationCode.isEmpty()) {
            qCWarning(dcTempo()) << "No authorization code received.";
            return info->finish(Thing::ThingErrorAuthenticationFailure);
        }

        Tempo *tempo = m_setupTempoConnections.value(info->thingId());
        if (!tempo) {
            qWarning(dcTempo()) << "No Tempo connection found for device:"  << info->thingName();
            m_setupTempoConnections.remove(info->thingId());
            return info->finish(Thing::ThingErrorHardwareFailure);
        }
        qCDebug(dcTempo()) << "Authorization code" << authorizationCode.mid(0, 4)+QString().fill('*', authorizationCode.length()-4) ;
        tempo->getAccessTokenFromAuthorizationCode(authorizationCode);
        connect(tempo, &Tempo::receivedRefreshToken, info, [info, this](const QByteArray &refreshToken){
            qCDebug(dcTempo()) << "Token:" << refreshToken.mid(0, 4)+QString().fill('*', refreshToken.length()-4) ;

            pluginStorage()->beginGroup(info->thingId().toString());
            pluginStorage()->setValue("refresh_token", refreshToken);
            pluginStorage()->endGroup();

            info->finish(Thing::ThingErrorNoError);
        });
    } else {
        Q_ASSERT_X(false, "confirmPairing", QString("Unhandled thingClassId: %1").arg(info->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginTempo::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcTempo()) << "Setup thing";

    if (thing->thingClassId() == tempoConnectionThingClassId) {

    }
}

void IntegrationPluginTempo::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == tempoConnectionThingClassId) {

    } else if (thing->thingClassId() == accountThingClassId) {

    }
}

void IntegrationPluginTempo::thingRemoved(Thing *thing)
{
    qCDebug(dcTempo()) << "Thing removed" << thing->name();
}

void IntegrationPluginTempo::onReceivedAccounts(const QList<Tempo::Account> &accounts)
{
    qCDebug(dcTempo()) << "Received" << accounts.count() << "accounts";

    Tempo *tempoConnection = static_cast<Tempo *>(sender());
    Thing *parentThing = m_tempoConnections.key(tempoConnection);
    if (!parentThing)
        return;

    ThingDescriptors desciptors;
    Q_FOREACH(Tempo::Account account, accounts) {
        ThingClassId thingClassId;

        Thing * existingThing = myThings().findByParams(ParamList() << Param(m_idParamTypeIds.value(thingClassId), appliance.homeApplianceId));
        if (existingThing) {
            qCDebug(dcTempo()) << "Thing is already added to system" << existingThing->name();
            //Set connected state;
            //existingThing->setStateValue(m_connectedStateTypeIds.value(thingClassId), appliance.connected);
            continue;
        }
        qCDebug(dcTempo()) << "Found new account:" << account.name << "key:" << account.key << "id:" << account.id;
        ThingDescriptor descriptor(thingClassId, account.name, account.key, parentThing->id());

        ParamList params;
        //params << Param(m_idParamTypeIds.value(thingClassId), appliance.homeApplianceId);
        descriptor.setParams(params);
        desciptors.append(descriptor);
    }
    if (!desciptors.isEmpty())
        emit autoThingsAppeared(desciptors);
}

