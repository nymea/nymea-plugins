#include "devicemonitor.h"

#include "extern-plugininfo.h"

#include <QNetworkInterface>

DeviceMonitor::DeviceMonitor(const QString &name, const QString &macAddress, const QString &ipAddress, bool initialState, QObject *parent):
    QObject(parent),
    m_name(name),
    m_macAddress(macAddress),
    m_ipAddress(ipAddress),
    m_reachable(initialState)
{
    m_arpLookupProcess = new QProcess(this);
    connect(m_arpLookupProcess, SIGNAL(finished(int)), this, SLOT(arpLookupFinished(int)));

    m_arpingProcess = new QProcess(this);
    m_arpingProcess->setReadChannelMode(QProcess::MergedChannels);
    connect(m_arpingProcess, SIGNAL(finished(int)), this, SLOT(arpingFinished(int)));

    m_pingProcess = new QProcess(this);
    m_pingProcess->setReadChannelMode(QProcess::MergedChannels);
    connect(m_pingProcess, SIGNAL(finished(int)), this, SLOT(pingFinished(int)));
}

DeviceMonitor::~DeviceMonitor()
{
}

void DeviceMonitor::setGracePeriod(int minutes)
{
    m_gracePeriod = minutes;
}

void DeviceMonitor::update()
{
    if (m_arpingProcess->state() != QProcess::NotRunning || m_pingProcess->state() != QProcess::NotRunning) {
//        log("Previous ping still running. Not updating.");
        return;
    }
    lookupArpCache();
}

void DeviceMonitor::lookupArpCache()
{
    m_arpLookupProcess->start("ip", {"-4", "-s", "neighbor", "list"});
}

void DeviceMonitor::arpLookupFinished(int exitCode)
{
    if (exitCode != 0) {
        warn("Error looking up ARP cache.");
        return;
    }

    QString data = QString::fromLatin1(m_arpLookupProcess->readAll());
    bool found = false;
    bool needsPing = true;
    QString mostRecentIP = m_ipAddress;
    qlonglong secsSinceLastSeen = -1;
    foreach (QString line, data.split('\n')) {
        line.replace(QRegExp("[ ]{1,}"), " ");
        QStringList parts = line.split(" ");
        int lladdrIndex = parts.indexOf("lladdr");
        if (lladdrIndex >= 0 && parts.count() > lladdrIndex + 1 && parts.at(lladdrIndex+1).toLower() == m_macAddress.toLower()) {
            found = true;
            QString entryIP = parts.first();
            if (parts.last() == "REACHABLE") {
                log("Device found in ARP cache and claims to be REACHABLE (Cache IP: " + entryIP + ")");
                if (!m_reachable) {
                    m_reachable = true;
                    emit reachableChanged(true);
                }
                emit seen();
                // Verify if IP address is still the same
                if (entryIP != mostRecentIP) {
                    mostRecentIP = entryIP;
                }
                // If we have a reachable entry, stop processing here
                needsPing = false;
                m_failedPings = 0;
                break;
            } else {
                // ARP claims the device to be stale... Flagging device to require a ping.
                log("Device found in ARP cache but is marked as " + parts.last() + " (Cache IP: " + entryIP + ")");

                int usedIndex = parts.indexOf("used");
                if (usedIndex >= 0 && parts.count() > usedIndex + 1) {
                    QString usedFields = parts.at(usedIndex + 1);
                    qlonglong newSecsSinceLastSeen = usedFields.split("/").first().toInt();
                    if (secsSinceLastSeen == -1 || newSecsSinceLastSeen < secsSinceLastSeen) {
                        secsSinceLastSeen = newSecsSinceLastSeen;
                        mostRecentIP = entryIP;
                    }
                }
            }
        }
    }
    if (mostRecentIP != m_ipAddress) {
        log("Device has changed IP: " + m_ipAddress + " -> " + mostRecentIP + ")");
        m_ipAddress = mostRecentIP;
        emit addressChanged(mostRecentIP);
    }
    if (!found) {
        log("Device not found in ARP cache.");
        arping();
    } else if (needsPing) {
        arping();
    }
}

void DeviceMonitor::arping()
{
    log("Sending ARP Ping...");
    QNetworkInterface targetInterface;
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        foreach (const QNetworkAddressEntry &addressEntry, interface.addressEntries()) {
            QHostAddress clientAddress(m_ipAddress);
            if (clientAddress.isInSubnet(addressEntry.ip(), addressEntry.prefixLength())) {
                targetInterface = interface;
                break;
            }
        }
    }
    if (!targetInterface.isValid()) {
        warn("Could not find a suitable interface to ARP Ping.");
        if (m_reachable) {
            m_reachable = false;
            emit reachableChanged(false);
        }
        return;
    }

    m_arpingProcess->start("arping", {"-I", targetInterface.name(), "-f", "-w", "30", m_ipAddress});
}

void DeviceMonitor::arpingFinished(int exitCode)
{
    if (exitCode == 0) {
        // we were able to ping the device
        log("ARP Ping successful.");
        if (!m_reachable) {
            m_reachable = true;
            emit reachableChanged(true);
        }
        emit seen();
        m_failedPings = 0;
    } else {
        log("ARP Ping failed.");
        ping();
    }
    // read data to discard it from socket
    QString data = QString::fromLatin1(m_pingProcess->readAll());
    Q_UNUSED(data)
//    qCDebug(dcNetworkDetector()) << "have ping data" << data;
}

void DeviceMonitor::ping()
{
    log("Sending ICMP Ping...");
    m_pingProcess->start("ping", {"-c", "30", m_ipAddress});
}

void DeviceMonitor::pingFinished(int exitCode)
{
    if (exitCode == 0) {
        // we were able to ping the device
        log("ICMP Ping successful.");
        if (!m_reachable) {
            m_reachable = true;
            emit reachableChanged(true);
        }
        emit seen();
        m_failedPings = 0;
    } else {
        m_failedPings++;
        log("ICMP Ping failed for " + QString::number(m_failedPings) + " times. (Grace Period: " + QString::number(m_gracePeriod) + ")");
        if (m_failedPings > m_gracePeriod && m_reachable) {
            log("Marking device as offline.");
            m_reachable = false;
            emit reachableChanged(false);
        }
    }
    // read data to discard it from socket
    QString data = QString::fromLatin1(m_pingProcess->readAll());
    Q_UNUSED(data)
//    qCDebug(dcNetworkDetector()) << "have ping data" << data;
}

void DeviceMonitor::log(const QString &message)
{
    qCDebug(dcNetworkDetector()).noquote().nospace() << m_name << " (" << m_macAddress  << ", " << m_ipAddress << "): " << message;
}

void DeviceMonitor::warn(const QString &message)
{
    qCWarning(dcNetworkDetector()).noquote().nospace() << m_name << " (" << m_macAddress << ", " << m_ipAddress << "): " << message;
}
