#include "devicemonitor.h"

#include "extern-plugininfo.h"

DeviceMonitor::DeviceMonitor(const QString &macAddress, const QString &ipAddress, QObject *parent):
    QObject(parent)
{
    m_host = new Host();
    m_host->setMacAddress(macAddress);
    m_host->setAddress(ipAddress);
    m_host->setReachable(false);

    m_arpLookupProcess = new QProcess(this);
    connect(m_arpLookupProcess, SIGNAL(finished(int)), this, SLOT(arpLookupFinished(int)));

    m_pingProcess = new QProcess(this);
    connect(m_pingProcess, SIGNAL(finished(int)), this, SLOT(pingFinished(int)));
}

DeviceMonitor::~DeviceMonitor()
{
    delete m_host;
}

void DeviceMonitor::update()
{
    lookupArpCache();
}

void DeviceMonitor::lookupArpCache()
{
    m_arpLookupProcess->start("ip", {"-s", "neighbor", "list"});
}

void DeviceMonitor::ping()
{
    m_pingProcess->start("ping", {"-c", "1", m_host->address()});
}

void DeviceMonitor::arpLookupFinished(int exitCode)
{
    if (exitCode != 0) {
        qWarning(dcNetworkDetector()) << "Error looking up ARP cache.";
        return;
    }

    QString data = QString::fromLatin1(m_arpLookupProcess->readAll());
    bool found = false;
    foreach (QString line, data.split('\n')) {
        line.replace(QRegExp("[ ]{1,}"), " ");
        QStringList parts = line.split(" ");
        int lladdrIndex = parts.indexOf("lladdr");
        if (lladdrIndex >= 0 && parts.count() > lladdrIndex && parts.at(lladdrIndex+1).toLower() == m_host->macAddress().toLower()) {
            found = true;
            // Verify if IP address is still the same
            if (parts.first() != m_host->address()) {
                m_host->setAddress(parts.first());
                emit addressChanged(parts.first());
            }
            if (parts.last() == "REACHABLE") {
                qDebug(dcNetworkDetector()) << "Device" << m_host->macAddress() << "found in ARP cache and claims to be REACHABLE";
                m_host->seen();
                if (!m_host->reachable()) {
                    m_host->setReachable(true);
                    emit reachableChanged(true);
                }
            } else {
                // ARP claims the device to be stale... try to ping it.
                qCDebug(dcNetworkDetector()) << "Device" << m_host->macAddress() << "found in ARP cache but is marked as" << parts.last() << ". Trying to ping it...";
                ping();
            }
            break;
        }
    }
    if (!found) {
        qCDebug(dcNetworkDetector()) << "Device" << m_host->macAddress() << "not found in ARP cache. Trying to ping it...";
        ping();
    }

    if (m_host->reachable() && m_host->lastSeenTime().addSecs(150) < QDateTime::currentDateTime()) {
        qCDebug(dcNetworkDetector()) << "Could not reach device for > 150 secs. Marking it as gone." << m_host->address() << m_host->macAddress();
        m_host->setReachable(false);
        emit reachableChanged(false);
    }
}

void DeviceMonitor::pingFinished(int exitCode)
{
    if (exitCode == 0) {
        // we were able to ping the device
        m_host->seen();
        if (!m_host->reachable()) {
            m_host->setReachable(true);
            emit reachableChanged(true);
        }
    } else {
        qDebug(dcNetworkDetector()) << "Could not ping device" << m_host->macAddress() << m_host->address();
    }
    // read data to discard it from socket
    QString data = QString::fromLatin1(m_pingProcess->readAll());
    Q_UNUSED(data)
//    qCDebug(dcNetworkDetector()) << "have ping data" << data;
}
