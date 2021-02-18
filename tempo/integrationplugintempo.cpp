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


        QString jiraCloudInstanceName = info->params().paramValue(tempoConnectionThingAtlassianAccountNameParamTypeId).toString();

        qCDebug(dcTempo()) << "Checking if the Tempo server is reachable: https://api.tempo.io/core/3";
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(QUrl("https://api.tempo.io/core/3")));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [reply, info, this] {

            if (reply->error() != QNetworkReply::NetworkError::HostNotFoundError) {
                qCDebug(dcTempo()) << "Tempo server is reachable";
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

    if (info->thingClassId() == tempoConnectionThingClassId) {
        qCDebug(dcTempo()) << "Confirm  pairing" << info->thingName();
        if (secret.isEmpty()) {
            qCWarning(dcTempo()) << "No authorization code received.";
            return info->finish(Thing::ThingErrorAuthenticationFailure);
        }
        QString atlassianAccountName = info->params().paramValue(tempoConnectionThingAtlassianAccountNameParamTypeId).toString();
        Tempo *tempo = new Tempo(hardwareManager()->networkManager(), atlassianAccountName, secret, this);
        tempo->getAccounts();
        connect(info, &ThingPairingInfo::aborted, tempo, &Tempo::deleteLater);
        connect(tempo, &Tempo::authenticationStatusChanged, info, [info, tempo, this] (bool authenticated){
            if (authenticated) {
                m_setupTempoConnections.insert(info->thingId(), tempo);
                info->finish(Thing::ThingErrorNoError);
            }
        });
    } else {
        Q_ASSERT_X(false, "confirmPairing", QString("Unhandled thingClassId: %1").arg(info->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginTempo::discoverThings(ThingDiscoveryInfo *info)
{
    qCDebug(dcTempo()) << "Discover things";
    if (info->thingClassId() == accountThingClassId) {
        Q_FOREACH(Tempo *tempo, m_tempoConnections) {
            tempo->getAccounts();
            Thing *parentThing = m_tempoConnections.key(tempo);
            if (!parentThing) {
                qCWarning(dcTempo()) << "Parent not found";
                return;
            }
            connect(tempo, &Tempo::accountsReceived, info, [info, parentThing, this, tempo] (const QList<Tempo::Account> &accounts) {
                Q_FOREACH(Tempo::Account account, accounts) {
                    ThingDescriptor descriptor(accountThingClassId, account.name, account.customer.name, parentThing->id());
                    ParamList params;
                    params << Param(accountThingKeyParamTypeId, account.key);
                    descriptor.setParams(params);
                    info->addThingDescriptor(descriptor);
                }
            });
        }
    } else if (info->thingClassId() == teamThingClassId) {
        Q_FOREACH(Tempo *tempo, m_tempoConnections) {
            tempo->getTeams();
            Thing *parentThing = m_tempoConnections.key(tempo);
            if (!parentThing) {
                qCWarning(dcTempo()) << "Parent not found";
                return;
            }
        }
    }
    QTimer::singleShot(5000, info, [info] {
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginTempo::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcTempo()) << "Setup thing";

    if (thing->thingClassId() == tempoConnectionThingClassId) {

        Tempo *tempo;
        if (m_tempoConnections.contains(thing)) {
            qCDebug(dcTempo()) << "Setup after reconfiguration, cleaning up";
            m_tempoConnections.take(thing)->deleteLater();
        }
        if (m_setupTempoConnections.keys().contains(thing->id())) {
            // This thing setup is after a pairing process
            qCDebug(dcTempo()) << "Tempo OAuth setup complete";
            tempo = m_setupTempoConnections.take(thing->id());
            if (!tempo) {
                qCWarning(dcTempo()) << "Tempo connection object not found for thing" << thing->name();
            }
            m_tempoConnections.insert(thing, tempo);
            info->finish(Thing::ThingErrorNoError);
        } else {
            //device loaded from the device database, needs a new access token;
            pluginStorage()->beginGroup(thing->id().toString());
            QByteArray token = pluginStorage()->value("token").toByteArray();
            pluginStorage()->endGroup();
            if (token.isEmpty()) {
                info->finish(Thing::ThingErrorAuthenticationFailure, tr("Token is not available."));
                return;
            }
            QString jiraInstanceName = thing->paramValue(tempoConnectionThingAtlassianAccountNameParamTypeId).toString();
            Tempo *tempo = new Tempo(hardwareManager()->networkManager(), jiraInstanceName, token, this);
            tempo->getAccounts();

            connect(info, &ThingSetupInfo::aborted, tempo, &Tempo::deleteLater);
            connect(tempo, &Tempo::authenticationStatusChanged, info, [info, tempo, this] (bool authenticated){
                if (authenticated) {
                    m_tempoConnections.insert(info->thing(), tempo);
                    info->finish(Thing::ThingErrorNoError);
                }
            });
        }
        connect(tempo, &Tempo::connectionChanged, this, &IntegrationPluginTempo::onConnectionChanged);
        connect(tempo, &Tempo::authenticationStatusChanged, this, &IntegrationPluginTempo::onAuthenticationStatusChanged);
        connect(tempo, &Tempo::accountsReceived, this, &IntegrationPluginTempo::onReceivedAccounts);

    } else if (thing->thingClassId() == accountThingClassId ||
               thing->thingClassId() == teamThingClassId){
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

void IntegrationPluginTempo::postSetupThing(Thing *thing)
{
    qCDebug(dcTempo()) << "Post setup thing" << thing->name();

    if (!m_pluginTimer15min) {
        m_pluginTimer15min = hardwareManager()->pluginTimerManager()->registerTimer(60*15);
        connect(m_pluginTimer15min, &PluginTimer::timeout, this, [this]() {
            qCDebug(dcTempo()) << "Refresh timer timout, polling all Tempo accounts.";
            Q_FOREACH (Thing *thing, myThings().filterByThingClassId(tempoConnectionThingClassId)) {
                Tempo *tempo = m_tempoConnections.value(thing);
                if (!tempo) {
                    qWarning(dcTempo()) << "No Tempo connection found for" << thing->name();
                    continue;
                }
                tempo->getAccounts();
                Q_FOREACH (Thing *childThing, myThings().filterByParentId(thing->id())) {
                    QString key = childThing->paramValue(accountThingKeyParamTypeId).toString();
                    QDate from(1970, 1, 1);
                    tempo->getWorkloadByAccount(key, from, QDate::currentDate());
                }
            }
        });
    }

    if (thing->thingClassId() == tempoConnectionThingClassId) {
        Tempo *tempo = m_tempoConnections.value(thing);
        tempo->getAccounts();

    } else if (thing->thingClassId() == accountThingClassId) {

    } else if (thing->thingClassId() == teamThingClassId) {

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

void IntegrationPluginTempo::onConnectionChanged(bool connected)
{
    Tempo *tempo = static_cast<Tempo *>(sender());
    Thing *thing = m_tempoConnections.key(tempo);
    if (!thing)
        return;
    thing->setStateValue(tempoConnectionConnectedStateTypeId, connected);
    if (!connected) {
        Q_FOREACH(Thing *child, myThings().filterByParentId(thing->id())) {
            child->setStateValue(accountConnectedStateTypeId, connected);
        }
    }
}

void IntegrationPluginTempo::onAuthenticationStatusChanged(bool authenticated)
{
    qCDebug(dcTempo()) << "Authentication changed" << authenticated;

    Tempo *tempoConnection = static_cast<Tempo *>(sender());

    Thing *thing = m_tempoConnections.key(tempoConnection);
    if (!thing)
        return;

    thing->setStateValue(tempoConnectionLoggedInStateTypeId, authenticated);
}


void IntegrationPluginTempo::onReceivedAccounts(const QList<Tempo::Account> &accounts)
{
    qCDebug(dcTempo()) << "Received" << accounts.count() << "accounts";
}

void IntegrationPluginTempo::onAccountWorkloadReceived(const QString &accountKey, QList<Tempo::Worklog> workloads)
{
    Thing *thing = myThings().findByParams(ParamList() << Param(accountThingKeyParamTypeId, accountKey));
    if (!thing)
        return;

    uint totalTimeSpentSeconds = 0;
    Q_FOREACH(Tempo::Worklog workload, workloads) {
        totalTimeSpentSeconds += workload.timeSpentSeconds;
    }
    thing->setStateValue(accountTotalTimeSpentStateTypeId, totalTimeSpentSeconds/60);
}

