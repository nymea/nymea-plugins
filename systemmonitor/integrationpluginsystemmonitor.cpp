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

#include "integrationpluginsystemmonitor.h"
#include "plugininfo.h"

#include <QStorageInfo>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>

IntegrationPluginSystemMonitor::IntegrationPluginSystemMonitor()
{

}

IntegrationPluginSystemMonitor::~IntegrationPluginSystemMonitor()
{
    if (m_refreshTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
    }
}

void IntegrationPluginSystemMonitor::setupThing(ThingSetupInfo *info)
{
    if (!m_refreshTimer) {
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
        connect(m_refreshTimer, &PluginTimer::timeout, this, [=](){

            foreach (Thing *thing, myThings()) {

                if (thing->thingClassId() == systemMonitorThingClassId) {
                    updateSystemMonitor(thing);

                } else if (thing->thingClassId() == processMonitorThingClassId) {
                    updateProcessMonitor(thing);
                }
            }
        });
    }
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginSystemMonitor::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
        m_refreshTimer = nullptr;
    }

    m_oldTotalJiffies.remove(thing);
    m_oldWorkJiffies.remove(thing);
    m_oldProcessWorkJiffies.remove(thing);
}

void IntegrationPluginSystemMonitor::updateSystemMonitor(Thing *thing)
{
    double cpuPercentage = readTotalCpuUsage(thing);
    if (cpuPercentage >= 0) {
        thing->setStateValue(systemMonitorCpuUsageStateTypeId, cpuPercentage);
    }
    thing->setStateValue(systemMonitorPercentMemoryStateTypeId, readTotalMemoryUsage());

    QStorageInfo storageInfo = QStorageInfo::root();
    double percentage = 100.0 * (storageInfo.bytesTotal() - storageInfo.bytesFree()) / storageInfo.bytesTotal();
    thing->setStateValue(systemMonitorPercentStorageStateTypeId, percentage);

}

void IntegrationPluginSystemMonitor::updateProcessMonitor(Thing *thing)
{
    QString processName = thing->paramValue(processMonitorThingProcessNameParamTypeId).toString();
    // For backwards compatibility we'll use nymead if the new parameter (version 1.3) isn't set at all yet.
    if (processName.isEmpty()) {
        processName = "nymead";
    }
    qint32 pid = getPidByName(processName);
    if (pid == -1) {
        thing->setStateValue(processMonitorRunningStateTypeId, false);
        return;
    }
    thing->setStateValue(processMonitorRunningStateTypeId, true);

    quint32 total, rss, shared;
    double percentage;
    if (readProcessMemoryUsage(pid, total, rss, shared, percentage)) {
        thing->setStateValue(processMonitorPercentMemoryStateTypeId, percentage);
        thing->setStateValue(processMonitorRssMemoryStateTypeId, rss);
        thing->setStateValue(processMonitorVirtualMemoryStateTypeId, total);
        thing->setStateValue(processMonitorSharedMemoryStateTypeId, shared);
    }

    thing->setStateValue(processMonitorCpuUsageStateTypeId, readProcessCpuUsage(pid, thing));
}

double IntegrationPluginSystemMonitor::readTotalCpuUsage(Thing *thing)
{
    QFile f("/proc/stat");
    if (!f.open(QFile::ReadOnly)) {
        qCWarning(dcSystemMonitor()) << "Unable to open /proc/stat. Cannot read CPU usage";
        return -1;
    }
    QByteArray cpuStat = f.readLine().replace("  ", " ");
    f.close();
    qCDebug(dcSystemMonitor()) << "SystemCPU:" << "stat:" << cpuStat;
    QList<QByteArray> parts = cpuStat.split(' ');
    if (parts.first() != "cpu" || parts.count() < 8) {
        qCWarning(dcSystemMonitor()) << "/proc/stat not in expected format";
        return -1;
    }

    qulonglong systemJiffies = parts.at(1).toULong();
//    qulonglong niceJiffies = parts.at(2).toULong();
    qulonglong userJiffies = parts.at(3).toULong();
    qulonglong idleJiffies = parts.at(4).toULong();
//    qulonglong ioWaitJiffies = parts.at(5).toULong();
//    qulonglong irqJiffies = parts.at(6).toULong();
//    qulonglong softirqJiffies = parts.at(7).toULong();

    qulonglong workJiffies = systemJiffies + userJiffies;
    qulonglong totalJiffies = workJiffies + idleJiffies;


    double cpuPercentage = 0;
    if (m_oldTotalJiffies.contains(thing)) {
        qulonglong oldTotalJiffies = m_oldTotalJiffies.value(thing);
        qulonglong oldWorkJiffies = m_oldWorkJiffies.value(thing);
        qCDebug(dcSystemMonitor()) << "SystemCPU:" << "Current work:" << workJiffies << "total:" << totalJiffies << "Old work:" << oldWorkJiffies << "total:" << oldTotalJiffies;
        if (workJiffies < oldWorkJiffies || totalJiffies < oldTotalJiffies) {
            // One of the values overflowed. Skipping this cycle
            m_oldTotalJiffies[thing] = totalJiffies;
            m_oldWorkJiffies[thing] = workJiffies;
            return -1;
        }
        qulonglong totalJiffDiff = totalJiffies - oldTotalJiffies;
        qulonglong workJiffDiff = workJiffies - oldWorkJiffies;

        cpuPercentage = 100.0 * workJiffDiff / totalJiffDiff;
    }
    m_oldTotalJiffies[thing] = totalJiffies;
    m_oldWorkJiffies[thing] = workJiffies;

    return cpuPercentage;
}

double IntegrationPluginSystemMonitor::readTotalMemoryUsage()
{
    struct sysinfo memInfo;
    sysinfo(&memInfo);

    qulonglong totalPhysicalMem = memInfo.totalram;
    // Multiply in next statement to avoid int overflow on right hand side...
    totalPhysicalMem *= memInfo.mem_unit;


    qulonglong physicalMemUsed = (memInfo.totalram - memInfo.freeram - memInfo.bufferram);
    // Multiply in next statement to avoid int overflow on right hand side...
    physicalMemUsed *= memInfo.mem_unit;


//    qCDebug(dcSystemMonitor()) << "Total RAM" << totalPhysicalMem << "used:" << physicalMemUsed;

    return 100.0 * physicalMemUsed / totalPhysicalMem;

}

bool IntegrationPluginSystemMonitor::readProcessMemoryUsage(qint32 pid, quint32 &total, quint32 &rss, quint32 &shared, double &percentage)
{
    QFile f(QString("/proc/%1/statm").arg(pid));
    if (!f.open(QFile::ReadOnly)) {
        qCWarning(dcSystemMonitor()).nospace() << "Unable to open " << f.fileName() << ". Cannot read memory usage.";
        return false;
    }

    QByteArray memStat = f.readLine();
    f.close();

    QList<QByteArray> parts = memStat.split(' ');
    if (parts.count() < 3) {
        qCWarning(dcSystemMonitor()) << f.fileName() << "not in expected format";
        return false;
    }
    int tsize = parts.at(0).toInt();
    int resident = parts.at(1).toInt();
    int share = parts.at(2).toInt();

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;

    total = tsize;
    total *= page_size_kb;
    rss = resident;
    rss *= page_size_kb;
    shared = share;
    shared *= page_size_kb;

    struct sysinfo memInfo;
    sysinfo(&memInfo);
    qulonglong totalMem = memInfo.totalram;
    totalMem *= memInfo.mem_unit;

    // totalMem is in Bytes, rss in kb
    percentage = 100000.0 * rss / totalMem;

//    qCDebug(dcSystemMonitor()) << "totalMem:" << totalMem << "total:" << total << "rss" << rss << "shared" << shared << "%" << percentage;
    return true;
}

double IntegrationPluginSystemMonitor::readProcessCpuUsage(qint32 pid, Thing *thing)
{
    QFile systemFile("/proc/stat");
    if (!systemFile.open(QFile::ReadOnly)) {
        qCWarning(dcSystemMonitor()) << "Unable to open /proc/stat. Cannot read CPU usage";
        return 0;
    }
    QByteArray cpuStat = systemFile.readLine().replace("  ", " ");
    systemFile.close();
    qCDebug(dcSystemMonitor()) << "ProcessCPU:" << "System stat:" << cpuStat;
    QList<QByteArray> parts = cpuStat.split(' ');
    if (parts.first() != "cpu" || parts.count() < 8) {
        qCWarning(dcSystemMonitor()) << "/proc/stat not in expected format";
        return 0;
    }
    parts.removeFirst();

    qulonglong systemJiffies = parts.at(1).toULong();
    qulonglong userJiffies = parts.at(3).toULong();
//    qulonglong idleJiffies = parts.at(4).toULong();

    qulonglong totalJiffies = systemJiffies + userJiffies;// + idleJiffies;

    QFile f(QString("/proc/%1/stat").arg(pid));
    if (!f.open(QFile::ReadOnly)) {
        qCWarning(dcSystemMonitor()).nospace() << "Unable to open " << f.fileName() << ". Cannot read CPU usage.";
        return 0;
    }

    QByteArray stat = f.readLine();
    f.close();

    qCDebug(dcSystemMonitor()) << "ProcessCPU:" << "Process stat:" << stat;

    parts = stat.split(' ');
    if (parts.length() < 15) {
        qCWarning(dcSystemMonitor()) << f.fileName() << "not in expected format";
        return 0;
    }

    qulonglong processUserJiffies = parts.at(13).toULongLong();
    qulonglong processKernelJiffies = parts.at(14).toULongLong();
    qulonglong processChildUserJiffies = parts.at(15).toULongLong();
    qulonglong processChildKernelJiffies = parts.at(16).toULongLong();
    qulonglong processWorkJiffies = processUserJiffies + processKernelJiffies + processChildUserJiffies + processChildKernelJiffies;

    double percentage = 0;
    if (m_oldTotalJiffies.contains(thing)) {
        qulonglong oldTotalJiffies = m_oldTotalJiffies.value(thing);
        qulonglong oldProcessWorkJiffies = m_oldProcessWorkJiffies.value(thing);

        qCDebug(dcSystemMonitor()) << "ProcessCPU:" << "Current total:" << totalJiffies << "process:" << processWorkJiffies << "Old total:" << oldTotalJiffies << "process:" << oldProcessWorkJiffies;

        qulonglong totalJiffDiff = totalJiffies - oldTotalJiffies;
        qulonglong processWorkJiffDiff = processWorkJiffies - oldProcessWorkJiffies;
        if (totalJiffDiff > 0) {
            percentage = 100.0 * processWorkJiffDiff / totalJiffDiff;
        }
    }

    m_oldTotalJiffies[thing] = totalJiffies;
    m_oldProcessWorkJiffies[thing] = processWorkJiffies;

    return percentage;
}

qint32 IntegrationPluginSystemMonitor::getPidByName(const QString &processName)
{
    QDir proc("/proc");
    foreach (const QString dirName, proc.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
        QFile statusFile(proc.absoluteFilePath(dirName + QDir::separator() + "status"));
        if (!statusFile.open(QFile::ReadOnly)) {
            continue;
        }
        QString line = statusFile.readLine().trimmed();
        line.remove(QRegExp("Name:(\\s)*"));
//        qCDebug(dcSystemMonitor()) << "Found process:" << line << "looking for" << processName.left(15);
        // names in /proc/<pid>/status are trimmed to 15 characters...
        if (processName.left(15) == line.left(15)) {
            return dirName.toInt();
        }
    }
    return -1;
}

