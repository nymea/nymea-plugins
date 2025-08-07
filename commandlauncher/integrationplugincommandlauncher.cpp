/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#include "integrationplugincommandlauncher.h"
#include "plugininfo.h"

#include <integrations/thing.h>

#include <QRegularExpression>

IntegrattionPluginCommandLauncher::IntegrattionPluginCommandLauncher()
{

}

void IntegrattionPluginCommandLauncher::setupThing(ThingSetupInfo *info)
{
    // Application
    if(info->thing()->thingClassId() == applicationThingClassId) {
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    // Script
    if(info->thing()->thingClassId() == scriptThingClassId){
        QStringList scriptArguments = info->thing()->paramValue(scriptThingScriptParamTypeId).toString().split(QRegularExpression("[ \r\n][ \r\n]*"));
        // check if script exists and if it is executable
        QFileInfo fileInfo(scriptArguments.first());
        if (!fileInfo.exists()) {
            qCWarning(dcCommandLauncher) << "script " << scriptArguments.first() << "does not exist.";
            //: Error setting up thing
            info->finish(Thing::ThingErrorItemNotFound, QString(QT_TR_NOOP("The script \"%1\" does not exist.")).arg(scriptArguments.first()));
            return;
        }
        if (!fileInfo.isExecutable()) {
            qCWarning(dcCommandLauncher) << "script " << scriptArguments.first() << "is not executable. Please check the permissions.";
            //: Error setting up thing
            info->finish(Thing::ThingErrorItemNotExecutable, QString(QT_TR_NOOP("The script \"%1\" is not executable.")).arg(scriptArguments.first()));
            return;
        }
        if (!fileInfo.isReadable()) {
            qCWarning(dcCommandLauncher) << "script " << scriptArguments.first() << "is not readable. Please check the permissions.";
            //: Error setting up thing
            info->finish(Thing::ThingErrorAuthenticationFailure, QString(QT_TR_NOOP("The script \"%1\" cannot be opened. Please check permissions.")).arg(scriptArguments.first()));
            return;
        }

        info->finish(Thing::ThingErrorNoError);
        return;
    }

    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrattionPluginCommandLauncher::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();

    // Application
    if (thing->thingClassId() == applicationThingClassId ) {
        // execute application...
        if (info->action().actionTypeId() == applicationTriggerActionTypeId) {
            // check if we already have started the application
            if (m_applications.values().contains(thing)) {
                if (m_applications.key(thing)->state() == QProcess::Running) {
                    //: Error running the application
                    info->finish(Thing::ThingErrorThingInUse, QT_TR_NOOP("This application is already running."));
                    return;
                }
            }
            QProcess *process = new QProcess(this);
            connect(process, &QProcess::stateChanged, info, [info](QProcess::ProcessState newState){
                switch (newState) {
                case QProcess::Starting:
                    qCDebug(dcCommandLauncher()) << "Application starting...";
                    return;
                case QProcess::Running:
                    qCDebug(dcCommandLauncher()) << "Application started.";
                    info->finish(Thing::ThingErrorNoError);
                    return;
                case QProcess::NotRunning:
                    qCDebug(dcCommandLauncher()) << "Application failed to start.";
                    //: Error running the application
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The application failed to start."));
                    return;
                }
            });
            connect(process, &QProcess::stateChanged, thing, [this, process, thing](QProcess::ProcessState newState){
                switch (newState) {
                case QProcess::Starting:
                    return; // Nothing to do here
                case QProcess::Running:
                    thing->setStateValue(applicationRunningStateTypeId, true);
                    return;
                case QProcess::NotRunning:
                    thing->setStateValue(applicationRunningStateTypeId, false);
                    m_applications.remove(process);
                    process->deleteLater();
                    return;
                }
            });

            m_applications.insert(process, thing);
            process->start("/bin/bash", QStringList() << "-c" << thing->paramValue(applicationThingCommandParamTypeId).toString());
            return;
        }
        // kill application...
        if (info->action().actionTypeId() == applicationKillActionTypeId) {
            // check if the application is running...
            QProcess *process = m_applications.key(info->thing());

            if (!process || process->state() == QProcess::NotRunning) {
                info->finish(Thing::ThingErrorNoError);
                return;
            }

            connect(process, &QProcess::stateChanged, info, [info](QProcess::ProcessState newState){
                switch (newState) {
                case QProcess::Starting:
                case QProcess::Running:
                    qCWarning(dcCommandLauncher()) << "The applicaton has started while it should be stopping.";
                    //: Error stopping the application
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An unexpected error happened."));
                    return;
                case QProcess::NotRunning:
                    qCDebug(dcCommandLauncher()) << "Application stopped.";
                    info->finish(Thing::ThingErrorNoError);
                    return;
                }
            });

            process->kill();
            return;
        }
        info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    // Script
    if (info->thing()->thingClassId() == scriptThingClassId ) {
        // execute script...
        if (info->action().actionTypeId() == scriptTriggerActionTypeId) {
            // check if we already have started the script
            if (m_scripts.values().contains(info->thing())) {
                if (m_scripts.key(info->thing())->state() == QProcess::Running) {
                    //: Error running the script
                    info->finish(Thing::ThingErrorThingInUse, QT_TR_NOOP("This script is already running."));
                    return;
                }
            }
            QProcess *process = new QProcess(this);

            connect(process, &QProcess::stateChanged, info, [info](QProcess::ProcessState newState){
                switch (newState) {
                case QProcess::Starting:
                    qCDebug(dcCommandLauncher()) << "Script starting...";
                    return;
                case QProcess::Running:
                    qCDebug(dcCommandLauncher()) << "Script started.";
                    info->finish(Thing::ThingErrorNoError);
                    return;
                case QProcess::NotRunning:
                    qCDebug(dcCommandLauncher()) << "Script failed to start.";
                    //: Error running the script
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The script failed to start."));
                    return;
                }
            });
            connect(process, &QProcess::stateChanged, thing, [this, process, thing](QProcess::ProcessState newState){
                switch (newState) {
                case QProcess::Starting:
                    return; // Nothing to do here
                case QProcess::Running:
                    thing->setStateValue(applicationRunningStateTypeId, true);
                    return;
                case QProcess::NotRunning:
                    thing->setStateValue(applicationRunningStateTypeId, false);
                    m_scripts.remove(process);
                    process->deleteLater();
                    return;
                }
            });

            m_scripts.insert(process, info->thing());
            process->start("/bin/bash", QStringList() << info->thing()->paramValue(scriptThingScriptParamTypeId).toString());
            return;
        }
        // kill script...
        if (info->action().actionTypeId() == scriptKillActionTypeId) {
            // check if the script is running...
            QProcess *process = m_scripts.key(info->thing());
            if (!process || process->state() == QProcess::NotRunning) {
                info->finish(Thing::ThingErrorNoError);
                return;
            }

            connect(process, &QProcess::stateChanged, info, [info](QProcess::ProcessState newState){
                switch (newState) {
                case QProcess::Starting:
                case QProcess::Running:
                    qCWarning(dcCommandLauncher()) << "The script has started while it should be stopping.";
                    //: Error stopping the script
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An unexpected error happened."));
                    return;
                case QProcess::NotRunning:
                    qCDebug(dcCommandLauncher()) << "Script stopped.";
                    info->finish(Thing::ThingErrorNoError);
                    return;
                }
            });

            process->kill();
            return;
        }

        info->finish(Thing::ThingErrorActionTypeNotFound);
        return;
    }
    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrattionPluginCommandLauncher::thingRemoved(Thing *thing)
{
    if (m_applications.values().contains(thing)) {
        QProcess * process = m_applications.key(thing);
        if (process->state() != QProcess::NotRunning) {
            process->kill();
        }
        m_applications.remove(process);
        process->deleteLater();
    }
    if (m_scripts.values().contains(thing)) {
        QProcess * process = m_scripts.key(thing);
        if (process->state() != QProcess::NotRunning) {
            process->kill();
        }
        m_scripts.remove(process);
        process->deleteLater();
    }
}
