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

#include "integrationpluginreversessh.h"
#include "plugininfo.h"

#include <QFile>
#include <QDir>

IntegrationPluginReverseSsh::IntegrationPluginReverseSsh()
{

}

IntegrationPluginReverseSsh::~IntegrationPluginReverseSsh()
{
    foreach (QProcess *process, m_processes) {
        process->terminate();
    }
}

void IntegrationPluginReverseSsh::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QString(QT_TR_NOOP("Please enter your login credentials for %1.")).arg(info->params().paramValue(reverseSshThingAddressParamTypeId).toString()));
}

void IntegrationPluginReverseSsh::confirmPairing(ThingPairingInfo *info, const QString &user, const QString &secret)
{
    // Perform a test login on the remote server
    QString address = info->params().paramValue(reverseSshThingAddressParamTypeId).toString();
    int remotePort = info->params().paramValue(reverseSshThingRemotePortParamTypeId).toInt();

    QStringList arguments;
    arguments << "-p" << secret << "ssh" << "-o StrictHostKeyChecking=no" << "-oUserKnownHostsFile=/dev/null";
    arguments << QString("%1@%2").arg(user, address) << "-p" << QString::number(remotePort) << "whoami";

    QProcess *process = new QProcess(this);
    process->setProgram("sshpass");
    process->setArguments(arguments);

    arguments.replace(1, "xxxxxx");
    qCDebug(dcReverseSsh()) << "Testing SSH connection:" << process->program() << arguments.join(" ");

    typedef void (QProcess:: *finishedSignal)(int exitCode, QProcess::ExitStatus exitStatus);
    connect(process, static_cast<finishedSignal>(&QProcess::finished), this, [=](int exitCode, QProcess::ExitStatus exitStatus){
        process->deleteLater();
        qCDebug(dcReverseSsh()) << "Testing process finished. Exit code:" << exitCode << "Exit status:" << exitStatus;

        switch (exitCode) {
        case 0:
            pluginStorage()->beginGroup(info->thingId().toString());
            pluginStorage()->setValue("username", user);
            pluginStorage()->setValue("password", secret);
            pluginStorage()->endGroup();
            qCInfo(dcReverseSsh()) << "Reverse SSH test login successful.";
            info->finish(Thing::ThingErrorNoError);
            break;
        case 5:
            qCWarning(dcReverseSsh()) << "Reverse SSH test login failed.";
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Login error on remote SSH server."));
            break;
        default:
            qCWarning(dcReverseSsh()) << "Reverse SSH test login unable to connect to SSH server.";
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Cannot connect to remote SSH server."));
        }

    });
    process->start();
}

void IntegrationPluginReverseSsh::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();


    QStringList arguments;
    int localPort    = thing->paramValue(reverseSshThingLocalPortParamTypeId).toInt();
    int remoteOpenPort = thing->paramValue(reverseSshThingRemoteOpenPortParamTypeId).toInt();
    int remotePort   = thing->paramValue(reverseSshThingRemotePortParamTypeId).toInt();
    QString address  = thing->paramValue(reverseSshThingAddressParamTypeId).toString();

    pluginStorage()->beginGroup(thing->id().toString());
    QString user = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();

    arguments << "-p" << password << "ssh" << "-o StrictHostKeyChecking=no" << "-oUserKnownHostsFile=/dev/null";
    arguments << "-o ServerAliveInterval=60";
    arguments << "-TN" << "-R" << QString("%1:localhost:%2").arg(remoteOpenPort).arg(localPort) << QString("%1@%2").arg(user, address) << "-p" << QString::number(remotePort);
    QProcess *process = new QProcess(thing);
    process->setProgram("sshpass");
    process->setArguments(arguments);
    process->setProcessChannelMode(QProcess::MergedChannels);
    arguments.replace(1, "xxxxxx");
    qCDebug(dcReverseSsh()) << "Reverse SSH command:" << process->program() << arguments;

    m_processes.insert(info->thing(), process);

    connect(process, &QProcess::stateChanged, thing, [=](QProcess::ProcessState newState){
        switch (newState) {
        case QProcess::Starting:
            qCDebug(dcReverseSsh()) << "Connection starting for" << thing->name();
            return ;
        case QProcess::Running:
            qCInfo(dcReverseSsh()) << "Reverse SSH connected for" << thing->name();
            thing->setStateValue(reverseSshConnectedStateTypeId, true);
            return;
        case QProcess::NotRunning:
            qCInfo(dcReverseSsh()) << "Reverse SSH disconnected for" << thing->name();
            thing->setStateValue(reverseSshConnectedStateTypeId, false);
            return;
        }
    });
    connect(process, &QProcess::readyRead, thing, [=](){
        QByteArray data = process->readAll();
        qCWarning(dcReverseSsh()) << "Reverse SSH connection data for" << thing->name() << data;
    });


    // Start up now if enabled
    bool enabled = thing->setting(reverseSshSettingsActiveParamTypeId).toBool();
    if (enabled) {
        process->start();
    }

    // And connect to the enabled setting
    connect(thing, &Thing::settingChanged, this, [=](const ParamTypeId &settingId, const QVariant &value){
        if (settingId == reverseSshSettingsActiveParamTypeId) {
            if (value.toBool()) {
                process->start();
            } else {
                process->terminate();
            }
        }
    });

    info->finish(Thing::ThingErrorNoError);


    // Create a watchdog to reconnect if a connection drops...
    if (!m_watchdog) {
        m_watchdog = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_watchdog, &PluginTimer::timeout, this, [this](){
            foreach (Thing *thing, m_processes.keys()) {
                QProcess *process = m_processes.value(thing);
                if (thing->setting(reverseSshSettingsActiveParamTypeId).toBool() && process->state() == QProcess::NotRunning) {
                    qCInfo(dcReverseSsh()) << "Reconnecting reverse SSH for" << thing->name();
                    process->start();
                }
            }
        });
    }
}


void IntegrationPluginReverseSsh::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == reverseSshThingClassId) {
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
