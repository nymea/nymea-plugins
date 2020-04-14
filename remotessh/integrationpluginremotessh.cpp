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

#include "integrationpluginremotessh.h"
#include "plugininfo.h"

#include <QFile>
#include <QDir>

IntegrationPluginRemoteSsh::IntegrationPluginRemoteSsh()
{

}

void IntegrationPluginRemoteSsh::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginRemoteSsh::onPluginTimeout);
}

void IntegrationPluginRemoteSsh::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    qCDebug(dcRemoteSsh()) << "Setup" << thing->name() << thing->params();

    if (thing->thingClassId() == reverseSshThingClassId) {
        m_identityFilePath = QString("%1/.ssh/id_rsa_guh").arg(QDir::homePath());
        return info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginRemoteSsh::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();
    if (thing->thingClassId() == reverseSshThingClassId ) {

        if (action.actionTypeId() == reverseSshConnectedActionTypeId) {

            if (action.param(reverseSshConnectedActionConnectedParamTypeId).value().toBool() == true) {
                QProcess *process = startReverseSSHProcess(thing);
                m_reverseSSHProcess.insert(process, thing);
                m_startingProcess.insert(process, info);
                // in case action call is cancelled, detach result reporting
                connect(info, &ThingActionInfo::destroyed, process, [this, process]{
                    m_startingProcess.remove(process);
                });
                return;
            } else {
                QProcess *process =  m_reverseSSHProcess.key(thing);

                // Check if the application is running...
                if (!process)
                    return info->finish(Thing::ThingErrorNoError);

                if (process->state() == QProcess::NotRunning)
                    return info->finish(Thing::ThingErrorNoError);

                process->kill();
                m_killingProcess.insert(process, info);
                // in case action call is cancelled, detach result reporting
                connect(info, &ThingActionInfo::destroyed, process, [this, process]{
                    m_killingProcess.remove(process);
                });
                return;
            }
        }
    }
}


void IntegrationPluginRemoteSsh::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == reverseSshThingClassId) {
        QProcess *process =  m_reverseSSHProcess.key(thing);
        if (!process)
            return;

        m_reverseSSHProcess.remove(process);
        if (process->state() != QProcess::NotRunning) {
            process->kill();
        }
        process->deleteLater();
    }
}

void IntegrationPluginRemoteSsh::processReadyRead()
{
    QByteArray data = static_cast<QProcess*>(sender())->readAll();
    qCWarning(dcRemoteSsh()) << "process read" << data;
}


QProcess * IntegrationPluginRemoteSsh::startReverseSSHProcess(Thing *thing)
{
    qCDebug(dcRemoteSsh()) << "Start reverse SSH";
    QProcess *process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished(int,QProcess::ExitStatus)));
    connect(process, &QProcess::stateChanged, this, &IntegrationPluginRemoteSsh::processStateChanged);
    connect(process, &QProcess::readyRead, this, &IntegrationPluginRemoteSsh::processReadyRead);

    QStringList arguments;
    int localPort    = thing->paramValue(reverseSshThingLocalPortParamTypeId).toInt();
    int remotePort   = thing->paramValue(reverseSshThingRemotePortParamTypeId).toInt();
    QString user     = thing->paramValue(reverseSshThingUserParamTypeId).toString();
    QString password = thing->paramValue(reverseSshThingPasswordParamTypeId).toString();
    QString address  = thing->paramValue(reverseSshThingAddressParamTypeId).toString();

    arguments << "-p" << password;
    arguments << "ssh" << "-o StrictHostKeyChecking=no" << "-oUserKnownHostsFile=/dev/null";
    arguments << "-TN" << "-R" << QString("%1:localhost:%2").arg(remotePort).arg(localPort) << QString("%1@%2").arg(user, address);
    process->start(QStringLiteral("sshpass"), arguments);
    qCDebug(dcRemoteSsh()) << "Command:" << process->program() << process->arguments();
    return process;
}

void IntegrationPluginRemoteSsh::onPluginTimeout()
{
    foreach(QProcess *process, m_reverseSSHProcess.keys()) {
        if (process->state() == QProcess::NotRunning)
            qCDebug(dcRemoteSsh()) << "SSH Process not running";
    }
}


void IntegrationPluginRemoteSsh::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    QProcess *process = static_cast<QProcess*>(sender());

    if(exitStatus != QProcess::NormalExit || exitCode != 0) {
        qCWarning(dcRemoteSsh()) << "Error:" << process->readAllStandardError();
    }

    if (m_reverseSSHProcess.contains(process)) {
        qCDebug(dcRemoteSsh()) << "SSH process finished";
        Thing *thing = m_reverseSSHProcess.value(process);
        thing->setStateValue(reverseSshConnectedStateTypeId, false);
        m_reverseSSHProcess.remove(process);

    } else if (m_sshKeyGenProcess.contains(process)) {
        qCDebug(dcRemoteSsh()) << "SSH Key generation process finished" <<  process->readAll();
        Thing *thing = m_sshKeyGenProcess.value(process);
        QFile file(QString(m_identityFilePath + ".pub"));
        if(!file.open(QIODevice::ReadOnly))
            qCWarning(dcRemoteSsh()) << "error" << file.errorString();

        QTextStream in(&file);
        QString sshKey = in.readLine();
        thing->setStateValue(reverseSshSshKeyStateTypeId, sshKey);
        process->kill();
        m_sshKeyGenProcess.remove(process);
        file.close();
    }
}


void IntegrationPluginRemoteSsh::processStateChanged(QProcess::ProcessState state)
{
    QProcess *process = static_cast<QProcess*>(sender());
    Thing *thing = m_reverseSSHProcess.value(process);

    switch (state) {
    case QProcess::Running:
        thing->setStateValue(reverseSshConnectedStateTypeId, true);
        if (m_startingProcess.contains(process)) {
            m_startingProcess.take(process)->finish(Thing::ThingErrorNoError);
        }
        break;

    case QProcess::NotRunning:
        if (thing)
            thing->setStateValue(reverseSshConnectedStateTypeId, false);

        if (m_startingProcess.contains(process)) {
            m_startingProcess.take(process)->finish(Thing::ThingErrorInvalidParameter);
        }

        if (m_killingProcess.contains(process)) {
            m_killingProcess.take(process)->finish(Thing::ThingErrorNoError);
            m_reverseSSHProcess.remove(process);
        }
        break;
    default:
        break;
    }
}
