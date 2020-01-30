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

void DevicePluginSystemMonitor::setupDevice(DeviceSetupInfo *info)
{
    if (!m_refreshTimer) {
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
        connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginSystemMonitor::onRefreshTimer);
    }
    info->finish(Device::DeviceErrorNoError);
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
    p->start("ps", {"-C", "nymead", "-o", "%mem=,vsz=,rss=,pcpu="});
}

void DevicePluginSystemMonitor::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess *p = static_cast<QProcess*>(sender());
    p->deleteLater();

    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
            qWarning(dcSystemMonitor) << "Error reading process memory usage:" << p->readAllStandardError();
            return;
    }
    QString data = QString(p->readAllStandardOutput().trimmed()).replace(QRegExp("[ ]{2,}"), " ");
    QStringList parts = data.split(' ');
    if (parts.count() != 4) {
        qCWarning(dcSystemMonitor()) << "Unexpected result from ps" << data << parts;
        return;
    }
    bool ok;
    qreal percentMem = parts.at(0).toDouble(&ok);
    if (!ok) {
        qWarning(dcSystemMonitor) << "Failed to parse % memory value to a number:" << parts.at(0);
        return;
    }
    qint64 virtualMem = parts.at(1).toLongLong(&ok);
    if (!ok) {
        qWarning(dcSystemMonitor) << "Failed to parse virtual memory value to a number:" << parts.at(1);
        return;
    }
    quint64 rssMem = parts.at(2).toLongLong(&ok);
    if (!ok) {
        qWarning(dcSystemMonitor) << "Failed to parse RSS memory value to a number:" << data;
        return;
    }
    qreal cpuUsage = parts.at(3).toDouble(&ok);
    if (!ok) {
        qWarning(dcSystemMonitor) << "Failed to parse CPU usage value to a number:" << parts.at(3);
        return;
    }
    foreach (Device *dev, myDevices()) {
        dev->setStateValue(systemMonitorRssMemoryStateTypeId, rssMem);
        dev->setStateValue(systemMonitorPercentMemoryStateTypeId, percentMem);
        dev->setStateValue(systemMonitorVirtualMemoryStateTypeId, virtualMem);
        dev->setStateValue(systemMonitorCpuUsageStateTypeId, cpuUsage);
    }
}

