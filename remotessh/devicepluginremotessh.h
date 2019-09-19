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

#ifndef DEVICEPLUGINREMOTESSH_H
#define DEVICEPLUGINREMOTESSH_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"

#include <QProcess>

class DevicePluginRemoteSsh : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginremotessh.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginRemoteSsh();

    void init() override;
    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

private:
    QHash<QProcess *, Device *> m_reverseSSHProcess;
    QHash<QProcess *, Device *> m_sshKeyGenProcess;

    QHash<QProcess *, DeviceActionInfo*> m_startingProcess;
    QHash<QProcess *, DeviceActionInfo*> m_killingProcess;

    PluginTimer *m_pluginTimer = nullptr;

    bool m_aboutToQuit = false;
    QString m_identityFilePath;

    QProcess *startReverseSSHProcess(Device *device);

private slots:
    void onPluginTimeout();
    void processReadyRead();
    void processStateChanged(QProcess::ProcessState state);
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // DEVICEPLUGINREMOTESSH_H
