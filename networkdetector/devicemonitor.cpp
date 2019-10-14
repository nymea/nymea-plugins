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
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    // Actually we'd need this fix on older platforms too, but it's hard to figure this out without this API...
    connect(m_arpingProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            warn(QString("arping process failed to start. Falling back to ping. This plugin might not work properly on this system."));
            ping();
        }
    });
#endif
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
    log("Setting grace period to " + QString::number(minutes) + " minutes.");
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
        if (lladdrIndex == -1 || parts.count() <= lladdrIndex) {
            continue;
        }
        QString entryIP = parts.first();
        QString entryMAC = parts.at(lladdrIndex + 1);

        if (entryMAC.toLower() == m_macAddress.toLower()) {
            found = true;
            QString entryIP = parts.first();
            if (parts.last() == "REACHABLE") {
                log("Device found in ARP cache and claims to be REACHABLE (Cache IP: " + entryIP + ")");
                if (!m_reachable) {
                    m_reachable = true;
                    emit reachableChanged(true);
                }
                emit seen();
                m_lastSeenTime = QDateTime::currentDateTime();
                // Verify if IP address is still the same
                if (entryIP != mostRecentIP) {
                    mostRecentIP = entryIP;
                }
                // If we have a reachable entry, stop processing here
                needsPing = false;
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
        } else if (entryIP == m_ipAddress) {
            warn("There seems to be a device with our IP but different MAC. Resetting IP config.");
            if (mostRecentIP == m_ipAddress) {
                mostRecentIP.clear();
            }
        }
    }
    if (mostRecentIP != m_ipAddress) {
        log("Device has changed IP: " + m_ipAddress + " -> " + mostRecentIP + ")");
        m_ipAddress = mostRecentIP;
        emit addressChanged(mostRecentIP);
    }
    if (m_ipAddress.isEmpty()) {
        warn("Device not found in ARP cache and no IP config available. Marking as gone.");
        if (m_reachable) {
            m_reachable = false;
            emit reachableChanged(false);
        }
        return;
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

    log("Sending ARP Ping to " + m_ipAddress + "...");
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
        m_lastSeenTime = QDateTime::currentDateTime();
    } else {
        log("ARP Ping failed.");
        ping();
    }
    // read data to discard it from socket
    QString data = QString::fromLatin1(m_arpingProcess->readAll());
    Q_UNUSED(data)
//    qCDebug(dcNetworkDetector()) << "have ping data" << data;
}

void DeviceMonitor::ping()
{
    log("Sending ICMP Ping to " + m_ipAddress + "...");
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
        m_lastSeenTime = QDateTime::currentDateTime();
    } else {
        log("ICMP Ping failed. Last seen: " + m_lastSeenTime.toString() + ", grace period: " + QString::number(m_gracePeriod) + " (until " + m_lastSeenTime.addSecs(60 * m_gracePeriod).toString() + ")");
        if (m_reachable && m_lastSeenTime.addSecs(m_gracePeriod * 60) < QDateTime::currentDateTime()) {
            log("Exceeded grace period of " + QString::number(m_gracePeriod) + " minutes. Marking device as offline.");
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
