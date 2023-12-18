/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2023, nymea GmbH
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

#include "integrationplugintmate.h"
#include "plugininfo.h"

#include <QFile>
#include <QDir>
#include <QRegularExpression>

IntegrationPluginTmate::IntegrationPluginTmate()
{

}

IntegrationPluginTmate::~IntegrationPluginTmate()
{
    foreach (QProcess *process, m_processes) {
        process->terminate();
    }
}

void IntegrationPluginTmate::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();


    QStringList arguments;
    QString apiKey = thing->paramValue(tmateThingApiKeyParamTypeId).toString();
    QString sessionName = thing->paramValue(tmateThingSessionNameParamTypeId).toString();

    arguments << "-F";
    if (!apiKey.isEmpty()) {
        arguments << "-k" << apiKey;
        if (!sessionName.isEmpty()) {
            arguments << "-n" << sessionName;
            arguments << "-r" << "ro-" + sessionName;
        }
    }
    QProcess *process = new QProcess(thing);
    process->setProgram("tmate");
    process->setArguments(arguments);
    process->setProcessChannelMode(QProcess::MergedChannels);

    m_processes.insert(info->thing(), process);

    connect(process, &QProcess::stateChanged, thing, [=](QProcess::ProcessState newState){
        switch (newState) {
        case QProcess::Starting:
            qCDebug(dcTmate()) << "Connection starting for" << thing->name();
            return ;
        case QProcess::Running:
            qCInfo(dcTmate()) << "Reverse SSH connected for" << thing->name();
            thing->setStateValue(tmateConnectedStateTypeId, true);
            return;
        case QProcess::NotRunning:
            qCInfo(dcTmate()) << "Reverse SSH disconnected for" << thing->name();
            thing->setStateValue(tmateConnectedStateTypeId, false);
            thing->setStateValue(tmateSshStateTypeId, "");
            thing->setStateValue(tmateSshRoStateTypeId, "");
            thing->setStateValue(tmateWebStateTypeId, "");
            thing->setStateValue(tmateWebRoStateTypeId, "");
            thing->setStateValue(tmateClientsStateTypeId, 0);
            return;
        }
    });
    connect(process, &QProcess::readyRead, thing, [=](){
        while (process->canReadLine()) {
            QByteArray data = process->readLine();

            qCDebug(dcTmate()) << thing->name() << ":" << data;
            auto extractSession = [thing](const StateTypeId &stateTypeId, const QString &type, const QString &input) {
                int sessionStart = input.indexOf(type);
                if (sessionStart >= 0) {
                    int sessionEnd = input.indexOf(QChar('\n'), sessionStart);
                    qCInfo(dcTmate()) << input << "Session start" << sessionStart << "session end" << sessionEnd;
                    QString session =input.mid(sessionStart + type.length(), sessionEnd);
                    thing->setStateValue(stateTypeId, session);
                }
            };

            extractSession(tmateSshStateTypeId, "ssh session: ssh ", data);
            extractSession(tmateSshRoStateTypeId, "ssh session read only: ssh ", data);
            extractSession(tmateWebStateTypeId, "web session: ", data);
            extractSession(tmateWebRoStateTypeId, "web session read only: ", data);

            QRegularExpression joinAddressRegex("joined \\(([0-9\\.]+)\\)");
            QRegularExpressionMatchIterator it = joinAddressRegex.globalMatch(data);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                QString word = match.captured(1);
                qCInfo(dcTmate()) << "Connected:" << word;
                thing->emitEvent(tmateClientConnectedEventTypeId, {{tmateClientConnectedEventClientAddressParamTypeId, word}});
                thing->setStateValue(tmateClientsStateTypeId, thing->stateValue(tmateClientsStateTypeId).toUInt()+1);
            }
            QRegularExpression leftAddressRegex("left \\(([0-9\\.]+)\\)");
            it = leftAddressRegex.globalMatch(data);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                QString word = match.captured(1);
                qCInfo(dcTmate()) << "Disconnected:" << word;
                thing->emitEvent(tmateClientDisconnectedEventTypeId, {{tmateClientDisconnectedEventClientAddressParamTypeId, word}});
                thing->setStateValue(tmateClientsStateTypeId, thing->stateValue(tmateClientsStateTypeId).toUInt()-1);
            }
        }

    });


    // Start up now if enabled
    bool enabled = thing->stateValue(tmateActiveStateTypeId).toBool();
    if (enabled) {
        process->start();
    }

    info->finish(Thing::ThingErrorNoError);


    // Create a watchdog to reconnect if a connection drops...
    if (!m_watchdog) {
        m_watchdog = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_watchdog, &PluginTimer::timeout, this, [this](){
            foreach (Thing *thing, m_processes.keys()) {
                QProcess *process = m_processes.value(thing);
                if (thing->stateValue(tmateActiveStateTypeId).toBool() && process->state() == QProcess::NotRunning) {
                    qCInfo(dcTmate()) << "Reconnecting tmate for" << thing->name();
                    process->start();
                }
            }
        });
    }
}


void IntegrationPluginTmate::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == tmateThingClassId) {
        QProcess *process =  m_processes.take(thing);
        if (process->state() != QProcess::NotRunning) {
            process->terminate();
            process->waitForFinished();
        }
    }

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_watchdog);
        m_watchdog = nullptr;
    }
}


void IntegrationPluginTmate::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == tmateActiveActionTypeId) {
        bool active = info->action().paramValue(tmateActiveActionActiveParamTypeId).toBool();
        QProcess *process = m_processes.value(info->thing());
        if (active) {
            qCInfo(dcTmate()) << "Reconnecting tmate for" << info->thing()->name();
            process->start();
        } else {
            qCInfo(dcTmate()) << "Terminating session for" << info->thing()->name();
            process->terminate();
        }
        info->thing()->setStateValue(tmateActiveStateTypeId, active);
        info->finish(Thing::ThingErrorNoError);
    }
}
