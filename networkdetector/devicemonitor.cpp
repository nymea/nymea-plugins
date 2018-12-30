#include "devicemonitor.h"

#include "extern-plugininfo.h"

#include <QNetworkInterface>

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
    m_pingProcess->setReadChannelMode(QProcess::MergedChannels);
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
    m_arpLookupProcess->start("ip", {"-4", "-s", "neighbor", "list"});
}

void DeviceMonitor::ping()
{
//    qCDebug(dcNetworkDetector()) << "Running:" << "ping" << "-c" << "1" << m_host->address();
    QNetworkInterface targetInterface;
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        foreach (const QNetworkAddressEntry &addressEntry, interface.addressEntries()) {
            QHostAddress clientAddress(m_host->address());
            if (clientAddress.isInSubnet(addressEntry.ip(), addressEntry.prefixLength())) {
                targetInterface = interface;
                break;
            }
        }
    }
    if (!targetInterface.isValid()) {
        qCWarning(dcNetworkDetector()) << "Could not find a suitable interface to ping for" << m_host->address();
        return;
    }
    m_pingProcess->start("arping", {"-I", targetInterface.name(), "-f", "-w", "5", m_host->address()});
}

void DeviceMonitor::arpLookupFinished(int exitCode)
{
    if (exitCode != 0) {
        qCWarning(dcNetworkDetector()) << "Error looking up ARP cache.";
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
                qCDebug(dcNetworkDetector()) << "Device" << m_host->macAddress() << "found in ARP cache and claims to be REACHABLE";
                m_host->seen();
                if (!m_host->reachable()) {
                    m_host->setReachable(true);
                    emit reachableChanged(true);
                }
                emit seen();
            } else {
                // ARP claims the device to be stale... try to ping it.
                qCDebug(dcNetworkDetector()) << "Device" << m_host->macAddress() << "found in ARP cache but is marked as" << parts.last() << ". Trying to ping it on" << m_host->address();
                ping();
            }
            break;
        }
    }
    if (!found) {
        qCDebug(dcNetworkDetector()) << "Device" << m_host->macAddress() << "not found in ARP cache. Trying to ping it on" << m_host->address();
        ping();
    }

    if (m_host->reachable() && m_host->lastSeenTime().addSecs(20) < QDateTime::currentDateTime()) {
        qCDebug(dcNetworkDetector()) << "Could not reach device for 20 seconds. Marking it as gone." << m_host->address() << m_host->macAddress();
        m_host->setReachable(false);
        emit reachableChanged(false);
    }
}

void DeviceMonitor::pingFinished(int exitCode)
{
    if (exitCode == 0) {
        // we were able to ping the device
        qCDebug(dcNetworkDetector()) << "Ping successful for" << m_host->macAddress() << m_host->address();
        m_host->seen();
        if (!m_host->reachable()) {
            m_host->setReachable(true);
            emit reachableChanged(true);
        }
        emit seen();
    } else {
        qCDebug(dcNetworkDetector()) << "Could not ping device" << m_host->macAddress() << m_host->address();
    }
    // read data to discard it from socket
    QString data = QString::fromLatin1(m_pingProcess->readAll());
    Q_UNUSED(data)
//    qCDebug(dcNetworkDetector()) << "have ping data" << data;
}
