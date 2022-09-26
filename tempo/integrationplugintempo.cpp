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

        qCDebug(dcTempo()) << "Checking if the Tempo server is reachable: https://api.tempo.io/core/3";
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(QUrl("https://api.tempo.io/core/3")));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [reply, info] {

            if (reply->error() != QNetworkReply::NetworkError::HostNotFoundError) {
                qCDebug(dcTempo()) << "Tempo server is reachable";
                info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter your Tempo API integration token."));
            } else {
                qCWarning(dcTempo()) << "Got online check error" << reply->error() << reply->errorString();
                info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Tempo server not reachable, please check the internet connection."));
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
        Tempo *tempo = new Tempo(hardwareManager()->networkManager(), secret, this);
        tempo->getAccounts();
        connect(info, &ThingPairingInfo::aborted, tempo, &Tempo::deleteLater);
        connect(tempo, &Tempo::authenticationStatusChanged, info, [info, tempo, secret, this] (bool authenticated){
            if (authenticated) {
                pluginStorage()->beginGroup(info->thingId().toString());
                pluginStorage()->setValue("token", secret);
                pluginStorage()->endGroup();
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
    if (m_tempoConnections.isEmpty()) {
        return info->finish(Thing::ThingErrorHardwareNotAvailable, tr("Create a Tempo connection first"));
    }

    if (info->thingClassId() == accountThingClassId) {
        Q_FOREACH(Tempo *tempo, m_tempoConnections) {
            tempo->getAccounts();
            ThingId parentThingId = m_tempoConnections.key(tempo);
            if (parentThingId.isNull()) {
                qCWarning(dcTempo()) << "Parent not found";
                return;
            }
            connect(tempo, &Tempo::accountsReceived, info, [info, parentThingId] (const QList<Tempo::Account> &accounts) {
                Q_FOREACH(Tempo::Account account, accounts) {
                    if (account.status == Tempo::Status::Archived)
                        continue;

                    ThingDescriptor descriptor(accountThingClassId, account.name, account.customer.name, parentThingId);
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
            ThingId parentThingId = m_tempoConnections.key(tempo);
            if (parentThingId.isNull()) {
                qCWarning(dcTempo()) << "Parent not found";
                return;
            }
            connect(tempo, &Tempo::teamsReceived, info, [info, parentThingId] (const QList<Tempo::Team> &teams) {
                Q_FOREACH(Tempo::Team team, teams) {

                    ThingDescriptor descriptor(teamThingClassId, team.name, team.summary, parentThingId);
                    ParamList params;
                    params << Param(teamThingIdParamTypeId, team.id);
                    descriptor.setParams(params);
                    info->addThingDescriptor(descriptor);
                }
            });
        }
    }
    QTimer::singleShot(5000, info, [info] {
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginTempo::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcTempo()) << "Setup thing" << thing->name();

    if (thing->thingClassId() == tempoConnectionThingClassId) {

        Tempo *tempo;
        if (m_tempoConnections.contains(thing->id())) {
            qCDebug(dcTempo()) << "Setup after reconfiguration, cleaning up";
            m_tempoConnections.take(thing->id())->deleteLater();
        }
        if (m_setupTempoConnections.keys().contains(thing->id())) {
            // This thing setup is after a pairing process
            qCDebug(dcTempo()) << "Tempo OAuth setup complete";
            tempo = m_setupTempoConnections.take(thing->id());
            if (!tempo) {
                qCWarning(dcTempo()) << "Tempo connection object not found for thing" << thing->name();
            }
            m_tempoConnections.insert(thing->id(), tempo);
            connect(tempo, &Tempo::connectionChanged, this, &IntegrationPluginTempo::onConnectionChanged);
            connect(tempo, &Tempo::authenticationStatusChanged, this, &IntegrationPluginTempo::onAuthenticationStatusChanged);
            connect(tempo, &Tempo::accountsReceived, this, &IntegrationPluginTempo::onAccountsReceived);
            connect(tempo, &Tempo::teamsReceived, this, &IntegrationPluginTempo::onTeamsReceived);
            connect(tempo, &Tempo::accountWorklogsReceived, this, &IntegrationPluginTempo::onAccountWorkloadReceived);
            connect(tempo, &Tempo::teamWorklogsReceived, this, &IntegrationPluginTempo::onTeamWorkloadReceived);
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
            Tempo *tempo = new Tempo(hardwareManager()->networkManager(), token, this);
            connect(info, &ThingSetupInfo::aborted, tempo, &Tempo::deleteLater);
            connect(tempo, &Tempo::authenticationStatusChanged, info, [info, tempo, this] (bool authenticated){
                if (authenticated) {
                    m_tempoConnections.insert(info->thing()->id(), tempo);
                    connect(tempo, &Tempo::connectionChanged, this, &IntegrationPluginTempo::onConnectionChanged);
                    connect(tempo, &Tempo::authenticationStatusChanged, this, &IntegrationPluginTempo::onAuthenticationStatusChanged);
                    connect(tempo, &Tempo::accountsReceived, this, &IntegrationPluginTempo::onAccountsReceived);
                    connect(tempo, &Tempo::teamsReceived, this, &IntegrationPluginTempo::onTeamsReceived);
                    connect(tempo, &Tempo::accountWorklogsReceived, this, &IntegrationPluginTempo::onAccountWorkloadReceived);
                    connect(tempo, &Tempo::teamWorklogsReceived, this, &IntegrationPluginTempo::onTeamWorkloadReceived);
                    info->finish(Thing::ThingErrorNoError);
                } else {
                    info->finish(Thing::ThingErrorAuthenticationFailure, tr("The Token seems to be expired."));
                }
            });
            tempo->getAccounts();
        }

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
                Tempo *tempo = m_tempoConnections.value(thing->id());
                if (!tempo) {
                    qWarning(dcTempo()) << "No Tempo connection found for" << thing->name();
                    continue;
                }
                tempo->getAccounts();
                tempo->getTeams();
                Q_FOREACH (Thing *childThing, myThings().filterByParentId(thing->id())) {
                    if (childThing->thingClassId() == accountThingClassId) {
                        QString key = childThing->paramValue(accountThingKeyParamTypeId).toString();
                        tempo->getWorkloadByAccount(key, QDate(1970, 1, 1), QDate::currentDate(), 0, 1000);
                    } else if (childThing->thingClassId() == teamThingClassId) {
                        int id = childThing->paramValue(teamThingIdParamTypeId).toInt();
                        tempo->getWorkloadByTeam(id, QDate(1970, 1, 1), QDate::currentDate(), 0, 1000);
                    }
                }
            }
        });
    }

    if (thing->thingClassId() == tempoConnectionThingClassId) {
        Tempo *tempo = m_tempoConnections.value(thing->id());
        if (!tempo) {
            qCWarning(dcTempo()) << "Tempo connection not found for" << thing->name();
            return;
        }
        tempo->getAccounts();

    } else if (thing->thingClassId() == accountThingClassId) {
        Tempo *tempo = m_tempoConnections.value(thing->parentId());
        QString key = thing->paramValue(accountThingKeyParamTypeId).toString();
        QDate from(1970, 1, 1);
        tempo->getWorkloadByAccount(key, from, QDate::currentDate(), 0, 1000);
        tempo->getAccounts();
    } else if (thing->thingClassId() == teamThingClassId) {
        Tempo *tempo = m_tempoConnections.value(thing->parentId());
        int id = thing->paramValue(teamThingIdParamTypeId).toInt();
        QDate from(1970, 1, 1);
        tempo->getWorkloadByTeam(id, from, QDate::currentDate(), 0, 1000);
        tempo->getTeams();
    }
}

void IntegrationPluginTempo::thingRemoved(Thing *thing)
{
    qCDebug(dcTempo()) << "Thing removed" << thing->name();
    if (thing->thingClassId() == tempoConnectionThingClassId) {
        m_tempoConnections.take(thing->id())->deleteLater();
    } else if (thing->thingClassId() == teamThingClassId ||
               thing->thingClassId() == accountThingClassId) {
        m_worklogBuffer.remove(thing->id());
    }

    if (myThings().isEmpty()) {
        qCDebug(dcTempo()) << "Stopping plugin timer";
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer15min);
        m_pluginTimer15min = nullptr;
    }
}

void IntegrationPluginTempo::onConnectionChanged(bool connected)
{
    Tempo *tempo = static_cast<Tempo *>(sender());
    Thing *thing = myThings().findById(m_tempoConnections.key(tempo));
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

    Tempo *tempo = static_cast<Tempo *>(sender());
    Thing *thing = myThings().findById(m_tempoConnections.key(tempo));
    if (!thing)
        return;

    thing->setStateValue(tempoConnectionLoggedInStateTypeId, authenticated);
}

void IntegrationPluginTempo::onAccountsReceived(const QList<Tempo::Account> accounts)
{
    qCDebug(dcTempo()) << "Accounts received";

    Q_FOREACH(Tempo::Account account, accounts) {
        //        qCDebug(dcTempo()) << "     - Account" << account.name;
        //        qCDebug(dcTempo()) << "         - Key" << account.key;
        //        qCDebug(dcTempo()) << "         - Monthly budget"  << account.monthlyBudget;
        //        qCDebug(dcTempo()) << "         - Lead" << account.lead.displayName;
        //        qCDebug(dcTempo()) << "         - Is Global" << account.global;
        //        qCDebug(dcTempo()) << "         - Contact type" << account.contact.type;
        //        qCDebug(dcTempo()) << "         - Contact account id" << account.contact.accountId;
        //        qCDebug(dcTempo()) << "         - Contact" << account.contact.displayName;
        //        qCDebug(dcTempo()) << "         - Category Id" << account.category.id;
        //        qCDebug(dcTempo()) << "         - Category name" << account.category.name;
        //        qCDebug(dcTempo()) << "         - Category key" << account.category.key;
        //        qCDebug(dcTempo()) << "         - Customer id" << account.customer.id;
        //        qCDebug(dcTempo()) << "         - Customer key" << account.customer.key;
        //        qCDebug(dcTempo()) << "         - Customer name" << account.customer.name;

        Thing *thing = myThings().findByParams(ParamList() << Param(accountThingKeyParamTypeId, account.key));
        if (!thing) {
            continue;
        }
        thing->setName(account.name);
        thing->setStateValue(accountConnectedStateTypeId, (account.status != Tempo::Status::Archived));
        thing->setStateValue(accountLeadStateTypeId, account.lead.displayName);
        thing->setStateValue(accountGlobalStateTypeId, account.global);
        thing->setStateValue(accountCategoryStateTypeId, account.category.name);
        thing->setStateValue(accountCustomerStateTypeId, account.customer.name);
        thing->setStateValue(accountContactStateTypeId, account.contact.displayName);
        thing->setStateValue(accountMonthlyBudgetStateTypeId, account.monthlyBudget);

        if (account.status == Tempo::Status::Open) {
            thing->setStateValue(accountStatusStateTypeId, "Open");
        } else if (account.status == Tempo::Status::Closed) {
            thing->setStateValue(accountStatusStateTypeId, "Closed");
        } else if (account.status == Tempo::Status::Archived) {
            thing->setStateValue(accountStatusStateTypeId, "Archived");
        }
    }
}

void IntegrationPluginTempo::onTeamsReceived(const QList<Tempo::Team> teams)
{
    qCDebug(dcTempo()) << "Teams received";

    Q_FOREACH(Tempo::Team team, teams) {
        Thing *thing = myThings().findByParams(ParamList() << Param(teamThingIdParamTypeId, team.id));
        if (!thing) {
            continue;
        }
        thing->setName(team.name);
        thing->setStateValue(teamConnectedStateTypeId, true);
        thing->setStateValue(teamLeadStateTypeId, team.lead.displayName);
    }
}

void IntegrationPluginTempo::onAccountWorkloadReceived(const QString &accountKey, QList<Tempo::Worklog> workloads, int limit, int offset)
{
    qCDebug(dcTempo()) << "Account workload received, account key:" << accountKey << "Worklog etries: "<< workloads.count();
    Thing *thing = myThings().findByParams(ParamList() << Param(accountThingKeyParamTypeId, accountKey));
    if (!thing) {
        qCWarning(dcTempo()) << "Could not find account thing for account key" << accountKey;
        return;
    }

    if (offset == 0) {
        m_worklogBuffer.remove(thing->id());
    }
    if (workloads.count() >= limit) {
        //limit is reached
        if (m_worklogBuffer.contains(thing->id())) {
            m_worklogBuffer[thing->id()].append(workloads);
        } else {
            m_worklogBuffer.insert(thing->id(), workloads);
        }
        Tempo *tempo = m_tempoConnections.value(thing->parentId());
        if (tempo) {
            tempo->getWorkloadByAccount(accountKey, QDate(1970, 1, 1), QDate::currentDate(), offset+workloads.count(), limit);
        }

    } else {
        uint totalTimeSpentSeconds = 0;
        uint thisMonthTimeSpentSeconds = 0;
        QDate today = QDate::currentDate();
        Q_FOREACH(Tempo::Worklog workload, workloads) {
            if ((workload.startDate.month() == today.month()) && (workload.startDate.year() == today.year())) {
                thisMonthTimeSpentSeconds += workload.timeSpentSeconds;
            }
            totalTimeSpentSeconds += workload.timeSpentSeconds;
        }
        if (m_worklogBuffer.contains(thing->id())) {
            Q_FOREACH(Tempo::Worklog workload, m_worklogBuffer.take(thing->id())) {
                if ((workload.startDate.month() == today.month()) && (workload.startDate.year() == today.year())) {
                    thisMonthTimeSpentSeconds += workload.timeSpentSeconds;
                }
                totalTimeSpentSeconds += workload.timeSpentSeconds;
            }
        }
        thing->setStateValue(accountTotalTimeSpentStateTypeId, totalTimeSpentSeconds/3600.00);
        thing->setStateValue(accountMonthTimeSpentStateTypeId, thisMonthTimeSpentSeconds/3600.00);
    }
}

void IntegrationPluginTempo::onTeamWorkloadReceived(int teamId, QList<Tempo::Worklog> workloads, int limit, int offset)
{
    qCDebug(dcTempo()) << "Team workload received, team ID:" << teamId << "Worklog entries: "<< workloads.count();
    Thing *thing = myThings().findByParams(ParamList() << Param(teamThingIdParamTypeId, teamId));
    if (!thing) {
        qCWarning(dcTempo()) << "Could not find team thing for account key" << teamId;
        return;
    }
    if (offset == 0) {
        m_worklogBuffer.remove(thing->id());
    }
    if (workloads.count() >= limit) {
        //limit is reached#
        if (m_worklogBuffer.contains(thing->id())) {
            m_worklogBuffer[thing->id()].append(workloads);
        } else {
            m_worklogBuffer.insert(thing->id(), workloads);
        }
        Tempo *tempo = m_tempoConnections.value(thing->parentId());
        if (tempo) {
            tempo->getWorkloadByTeam(teamId, QDate(1970, 1, 1), QDate::currentDate(), offset+workloads.count(), limit);
        }

    } else {
        uint totalTimeSpentSeconds = 0;
        uint thisMonthTimeSpentSeconds = 0;
        QDate today = QDate::currentDate();
        Q_FOREACH(Tempo::Worklog workload, workloads) {
            if ((workload.startDate.month() == today.month()) && (workload.startDate.year() == today.year())) {
                thisMonthTimeSpentSeconds += workload.timeSpentSeconds;
            }
            totalTimeSpentSeconds += workload.timeSpentSeconds;
        }
        if (m_worklogBuffer.contains(thing->id())) {
            Q_FOREACH(Tempo::Worklog workload, m_worklogBuffer.take(thing->id())) {
                if ((workload.startDate.month() == today.month()) && (workload.startDate.year() == today.year())) {
                    thisMonthTimeSpentSeconds += workload.timeSpentSeconds;
                }
                totalTimeSpentSeconds += workload.timeSpentSeconds;
            }
        }
        thing->setStateValue(teamTotalTimeSpentStateTypeId, totalTimeSpentSeconds/3600.00);
        thing->setStateValue(teamMonthTimeSpentStateTypeId, thisMonthTimeSpentSeconds/3600.00);
    }
}

