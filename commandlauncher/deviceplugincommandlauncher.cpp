/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *  Copyright (C) 2014 Michael Zanetti <michael_zanetti@gmx.net>           *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "deviceplugincommandlauncher.h"

#include "devices/device.h"
#include "plugininfo.h"

#include <QDebug>

DevicePluginCommandLauncher::DevicePluginCommandLauncher()
{

}

void DevicePluginCommandLauncher::setupDevice(DeviceSetupInfo *info)
{
    // Application
    if(info->device()->deviceClassId() == applicationDeviceClassId) {
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    // Script
    if(info->device()->deviceClassId() == scriptDeviceClassId){
        QStringList scriptArguments = info->device()->paramValue(scriptDeviceScriptParamTypeId).toString().split(QRegExp("[ \r\n][ \r\n]*"));
        // check if script exists and if it is executable
        QFileInfo fileInfo(scriptArguments.first());
        if (!fileInfo.exists()) {
            qCWarning(dcCommandLauncher) << "script " << scriptArguments.first() << "does not exist.";
            info->finish(Device::DeviceErrorItemNotFound, QString(QT_TR_NOOP("The script \"%1\" does not exist.")).arg(scriptArguments.first()));
            return;
        }
        if (!fileInfo.isExecutable()) {
            qCWarning(dcCommandLauncher) << "script " << scriptArguments.first() << "is not executable. Please check the permissions.";
            info->finish(Device::DeviceErrorItemNotExecutable, QString(QT_TR_NOOP("The script \"%1\" is not executable.")).arg(scriptArguments.first()));
            return;
        }
        if (!fileInfo.isReadable()) {
            qCWarning(dcCommandLauncher) << "script " << scriptArguments.first() << "is not readable. Please check the permissions.";
            info->finish(Device::DeviceErrorAuthenticationFailure, QString(QT_TR_NOOP("The script \"%1\" cannot be opened. Please check permissions.")).arg(scriptArguments.first()));
            return;
        }

        info->finish(Device::DeviceErrorNoError);
        return;
    }

    info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginCommandLauncher::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();

    // Application
    if (device->deviceClassId() == applicationDeviceClassId ) {
        // execute application...
        if (info->action().actionTypeId() == applicationTriggerActionTypeId) {
            // check if we already have started the application
            if (m_applications.values().contains(device)) {
                if (m_applications.key(device)->state() == QProcess::Running) {
                    info->finish(Device::DeviceErrorDeviceInUse, QT_TR_NOOP("This application is already running."));
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
                    info->finish(Device::DeviceErrorNoError);
                    return;
                case QProcess::NotRunning:
                    qCDebug(dcCommandLauncher()) << "Application failed to start.";
                    info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("The application failed to start."));
                    return;
                }
            });
            connect(process, &QProcess::stateChanged, device, [this, process, device](QProcess::ProcessState newState){
                switch (newState) {
                case QProcess::Starting:
                    return; // Nothing to do here
                case QProcess::Running:
                    device->setStateValue(applicationRunningStateTypeId, true);
                    return;
                case QProcess::NotRunning:
                    device->setStateValue(applicationRunningStateTypeId, false);
                    m_applications.remove(process);
                    process->deleteLater();
                    return;
                }
            });

            m_applications.insert(process, device);
            process->start("/bin/bash", QStringList() << "-c" << device->paramValue(applicationDeviceCommandParamTypeId).toString());
            return;
        }
        // kill application...
        if (info->action().actionTypeId() == applicationKillActionTypeId) {
            // check if the application is running...
            QProcess *process = m_applications.key(info->device());

            if (!process || process->state() == QProcess::NotRunning) {
                info->finish(Device::DeviceErrorNoError);
                return;
            }

            connect(process, &QProcess::stateChanged, info, [info](QProcess::ProcessState newState){
                switch (newState) {
                case QProcess::Starting:
                case QProcess::Running:
                    qCWarning(dcCommandLauncher()) << "The applicaton has started while it should be stopping.";
                    info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("An unexpected error happened."));
                    return;
                case QProcess::NotRunning:
                    qCDebug(dcCommandLauncher()) << "Application stopped.";
                    info->finish(Device::DeviceErrorNoError);
                    return;
                }
            });

            process->kill();
            return;
        }
        info->finish(Device::DeviceErrorActionTypeNotFound);
    }

    // Script
    if (info->device()->deviceClassId() == scriptDeviceClassId ) {
        // execute script...
        if (info->action().actionTypeId() == scriptTriggerActionTypeId) {
            // check if we already have started the script
            if (m_scripts.values().contains(info->device())) {
                if (m_scripts.key(info->device())->state() == QProcess::Running) {
                    info->finish(Device::DeviceErrorDeviceInUse, QT_TR_NOOP("This script is already running."));
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
                    info->finish(Device::DeviceErrorNoError);
                    return;
                case QProcess::NotRunning:
                    qCDebug(dcCommandLauncher()) << "Script failed to start.";
                    info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("The script failed to start."));
                    return;
                }
            });
            connect(process, &QProcess::stateChanged, device, [this, process, device](QProcess::ProcessState newState){
                switch (newState) {
                case QProcess::Starting:
                    return; // Nothing to do here
                case QProcess::Running:
                    device->setStateValue(applicationRunningStateTypeId, true);
                    return;
                case QProcess::NotRunning:
                    device->setStateValue(applicationRunningStateTypeId, false);
                    m_scripts.remove(process);
                    process->deleteLater();
                    return;
                }
            });

            m_scripts.insert(process, info->device());
            process->start("/bin/bash", QStringList() << info->device()->paramValue(scriptDeviceScriptParamTypeId).toString());
            return;
        }
        // kill script...
        if (info->action().actionTypeId() == scriptKillActionTypeId) {
            // check if the script is running...
            QProcess *process = m_scripts.key(info->device());
            if (!process || process->state() == QProcess::NotRunning) {
                info->finish(Device::DeviceErrorNoError);
                return;
            }

            connect(process, &QProcess::stateChanged, info, [info](QProcess::ProcessState newState){
                switch (newState) {
                case QProcess::Starting:
                case QProcess::Running:
                    qCWarning(dcCommandLauncher()) << "The script has started while it should be stopping.";
                    info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("An unexpected error happened."));
                    return;
                case QProcess::NotRunning:
                    qCDebug(dcCommandLauncher()) << "Script stopped.";
                    info->finish(Device::DeviceErrorNoError);
                    return;
                }
            });

            process->kill();
            return;
        }

        info->finish(Device::DeviceErrorActionTypeNotFound);
        return;
    }
    info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginCommandLauncher::deviceRemoved(Device *device)
{
    if (m_applications.values().contains(device)) {
        QProcess * process = m_applications.key(device);
        if (process->state() != QProcess::NotRunning) {
            process->kill();
        }
        m_applications.remove(process);
        process->deleteLater();
    }
    if (m_scripts.values().contains(device)) {
        QProcess * process = m_scripts.key(device);
        if (process->state() != QProcess::NotRunning) {
            process->kill();
        }
        m_scripts.remove(process);
        process->deleteLater();
    }
}
