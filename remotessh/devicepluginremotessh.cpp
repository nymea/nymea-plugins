/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "devicepluginremotessh.h"
#include "plugininfo.h"

#include <QFile>
#include <QDir>

DevicePluginRemoteSsh::DevicePluginRemoteSsh()
{

}

void DevicePluginRemoteSsh::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginRemoteSsh::onPluginTimeout);
}

void DevicePluginRemoteSsh::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    qCDebug(dcRemoteSsh()) << "Setup" << device->name() << device->params();

    if (device->deviceClassId() == reverseSshDeviceClassId) {
        m_identityFilePath = QString("%1/.ssh/id_rsa_guh").arg(QDir::homePath());
        return info->finish(Device::DeviceErrorNoError);
    }
}

void DevicePluginRemoteSsh::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();
    if (device->deviceClassId() == reverseSshDeviceClassId ) {

        if (action.actionTypeId() == reverseSshConnectedActionTypeId) {

            if (action.param(reverseSshConnectedActionConnectedParamTypeId).value().toBool() == true) {
                QProcess *process = startReverseSSHProcess(device);
                m_reverseSSHProcess.insert(process, device);
                m_startingProcess.insert(process, info);
                // in case action call is cancelled, detach result reporting
                connect(info, &DeviceActionInfo::destroyed, process, [this, process]{
                    m_startingProcess.remove(process);
                });
                return;
            } else {
                QProcess *process =  m_reverseSSHProcess.key(device);

                // Check if the application is running...
                if (!process)
                    return info->finish(Device::DeviceErrorNoError);

                if (process->state() == QProcess::NotRunning)
                    return info->finish(Device::DeviceErrorNoError);

                process->kill();
                m_killingProcess.insert(process, info);
                // in case action call is cancelled, detach result reporting
                connect(info, &DeviceActionInfo::destroyed, process, [this, process]{
                    m_killingProcess.remove(process);
                });
                return;
            }
        }
    }
}


void DevicePluginRemoteSsh::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == reverseSshDeviceClassId) {
        QProcess *process =  m_reverseSSHProcess.key(device);
        if (!process)
            return;

        m_reverseSSHProcess.remove(process);
        if (process->state() != QProcess::NotRunning) {
            process->kill();
        }
        process->deleteLater();
    }
}

void DevicePluginRemoteSsh::processReadyRead()
{
    QByteArray data = static_cast<QProcess*>(sender())->readAll();
    qCWarning(dcRemoteSsh()) << "process read" << data;
}


QProcess * DevicePluginRemoteSsh::startReverseSSHProcess(Device *device)
{
    qCDebug(dcRemoteSsh()) << "Start reverse SSH";
    QProcess *process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished(int,QProcess::ExitStatus)));
    connect(process, &QProcess::stateChanged, this, &DevicePluginRemoteSsh::processStateChanged);
    connect(process, &QProcess::readyRead, this, &DevicePluginRemoteSsh::processReadyRead);

    QStringList arguments;
    int localPort    = device->paramValue(reverseSshDeviceLocalPortParamTypeId).toInt();
    int remotePort   = device->paramValue(reverseSshDeviceRemotePortParamTypeId).toInt();
    QString user     = device->paramValue(reverseSshDeviceUserParamTypeId).toString();
    QString password = device->paramValue(reverseSshDevicePasswordParamTypeId).toString();
    QString address  = device->paramValue(reverseSshDeviceAddressParamTypeId).toString();

    arguments << "-p" << password;
    arguments << "ssh" << "-o StrictHostKeyChecking=no" << "-oUserKnownHostsFile=/dev/null";
    arguments << "-TN" << "-R" << QString("%1:localhost:%2").arg(remotePort).arg(localPort) << QString("%1@%2").arg(user, address);
    process->start(QStringLiteral("sshpass"), arguments);
    qCDebug(dcRemoteSsh()) << "Command:" << process->program() << process->arguments();
    return process;
}

void DevicePluginRemoteSsh::onPluginTimeout()
{
    foreach(QProcess *process, m_reverseSSHProcess.keys()) {
        if (process->state() == QProcess::NotRunning)
            qCDebug(dcRemoteSsh()) << "SSH Process not running";
    }
}


void DevicePluginRemoteSsh::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    QProcess *process = static_cast<QProcess*>(sender());

    if(exitStatus != QProcess::NormalExit || exitCode != 0) {
        qCWarning(dcRemoteSsh()) << "Error:" << process->readAllStandardError();
    }

    if (m_reverseSSHProcess.contains(process)) {
        qCDebug(dcRemoteSsh()) << "SSH process finished";
        Device *device = m_reverseSSHProcess.value(process);
        device->setStateValue(reverseSshConnectedStateTypeId, false);
        m_reverseSSHProcess.remove(process);

    } else if (m_sshKeyGenProcess.contains(process)) {
        qCDebug(dcRemoteSsh()) << "SSH Key generation process finished" <<  process->readAll();
        Device *device = m_sshKeyGenProcess.value(process);
        QFile file(QString(m_identityFilePath + ".pub"));
        if(!file.open(QIODevice::ReadOnly))
            qCWarning(dcRemoteSsh()) << "error" << file.errorString();

        QTextStream in(&file);
        QString sshKey = in.readLine();
        device->setStateValue(reverseSshSshKeyStateTypeId, sshKey);
        process->kill();
        m_sshKeyGenProcess.remove(process);
        file.close();
    }
}


void DevicePluginRemoteSsh::processStateChanged(QProcess::ProcessState state)
{
    QProcess *process = static_cast<QProcess*>(sender());
    Device *device = m_reverseSSHProcess.value(process);

    switch (state) {
    case QProcess::Running:
        device->setStateValue(reverseSshConnectedStateTypeId, true);
        if (m_startingProcess.contains(process)) {
            m_startingProcess.take(process)->finish(Device::DeviceErrorNoError);
        }
        break;

    case QProcess::NotRunning:
        if (device)
            device->setStateValue(reverseSshConnectedStateTypeId, false);

        if (m_startingProcess.contains(process)) {
            m_startingProcess.take(process)->finish(Device::DeviceErrorInvalidParameter);
        }

        if (m_killingProcess.contains(process)) {
            m_killingProcess.take(process)->finish(Device::DeviceErrorNoError);
            m_reverseSSHProcess.remove(process);
        }
        break;
    default:
        break;
    }
}
