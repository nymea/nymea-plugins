/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Michael Zanetti <michael.zanetti@guh.io             *
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

#include "devicepluginsystemmonitor.h"
#include "plugininfo.h"


DevicePluginSystemMonitor::DevicePluginSystemMonitor()
{

}

DevicePluginSystemMonitor::~DevicePluginSystemMonitor()
{
    if (m_refreshTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
    }
}

DeviceManager::DeviceSetupStatus DevicePluginSystemMonitor::setupDevice(Device *device)
{
    Q_UNUSED(device)

    if (!m_refreshTimer) {
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
        connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginSystemMonitor::onRefreshTimer);
    }


    return DeviceManager::DeviceSetupStatusSuccess;
}

void DevicePluginSystemMonitor::deviceRemoved(Device *device)
{
    Q_UNUSED(device)

    if (myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
        m_refreshTimer = nullptr;

    }
}

void DevicePluginSystemMonitor::onRefreshTimer()
{
    QProcess *p = new QProcess(this);
    connect(p, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onProcessFinished(int,QProcess::ExitStatus)));
    p->start("ps", {"-C", "nymead", "-o", "rss="});
}

void DevicePluginSystemMonitor::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess *p = static_cast<QProcess*>(sender());
    p->deleteLater();

    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
            qWarning(dcSystemMonitor) << "Error reading process memory usage:" << p->readAllStandardError();
            return;
    }
    QByteArray data = p->readAllStandardOutput().trimmed();
    bool ok;
    qreal rssMem = data.toDouble(&ok);
    if (!ok) {
        qWarning(dcSystemMonitor) << "Failed to parse RSS memory value to a number:" << data;
        return;
    }
    foreach (Device *dev, myDevices()) {
        dev->setStateValue(systemMonitorRssMemoryStateTypeId, rssMem);
    }
}

