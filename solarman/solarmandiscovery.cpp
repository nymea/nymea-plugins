#include "solarmandiscovery.h"

#include <QTimer>
#include <QUdpSocket>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(dcSolarman)

SolarmanDiscovery::SolarmanDiscovery(QObject *parent)
    : QObject{parent}
{
}

bool SolarmanDiscovery::discover()
{
    if (!initUdp()) {
        return false;
    }

    if (m_discoveryTimer) {
        qCDebug(dcSolarman()) << "Already discovering...";
        return true;
    }

    m_discoveryTimer = new QTimer(this);
    m_discoveryTimer->start(1000);

    connect(m_discoveryTimer, &QTimer::timeout, this, [=](){
        int counter = m_discoveryTimer->property("counter").toInt();
        if (counter < 5) {
            m_discoveryTimer->setProperty("counter", counter+1);
            sendDiscoveryMessage();
        } else {
            emit discoveryResults(m_discoveryResults);
            m_discoveryResults.clear();
            delete m_discoveryTimer;
            m_discoveryTimer = nullptr;
        }
    });

    bool status = sendDiscoveryMessage();
    if (!status) {
        delete m_discoveryTimer;
        m_discoveryTimer = nullptr;
    }
    return status;
}

bool SolarmanDiscovery::monitor(const QString &serial)
{
    if (!initUdp()) {
        return false;
    }

    m_monitoringResult.serial = serial;

    if (!m_monitorTimer) {
        m_monitorTimer = new QTimer(this);
        m_monitorTimer->start(15000);
        connect(m_monitorTimer, &QTimer::timeout, this, [=](){
            sendDiscoveryMessage();

            if (m_monitoringResult.lastSeen.msecsTo(QDateTime::currentDateTime()) > 60000) {
                m_monitoringResult.online = false;
                emit monitorStateChanged(m_monitoringResult);
            }
        });
    }

    bool status = sendDiscoveryMessage();
    if (!status) {
        delete m_monitorTimer;
        m_monitorTimer = nullptr;
    }
    return status;
}

SolarmanDiscovery::DiscoveryResult SolarmanDiscovery::monitorResult() const
{
    return m_monitoringResult;
}

void SolarmanDiscovery::onReadyRead()
{
    while (m_udp->hasPendingDatagrams()) {
        QByteArray data;
        data.resize(m_udp->pendingDatagramSize());
        QHostAddress host;
        m_udp->readDatagram(data.data(), m_udp->pendingDatagramSize(), &host);
        if (data == "WIFIKIT-214028-READ") {
            continue;
        }
        qCDebug(dcSolarman) << "UDP datagram from" << host.toString() << data;

        QStringList parts = QString(data).split(",");
        if (parts.count() != 3) {
            qCDebug(dcSolarman()) << "Unexpected discovery reply format:" << data;
            continue;
        }
        DiscoveryResult result;
        result.ip = parts.at(0);
        result.mac = parts.at(1);
        result.serial = parts.at(2);
        result.online = true;

        // Sanity check if the claimed IP matches with the sender
        if (result.ip.toIPv4Address() != host.toIPv4Address()) {
            qCDebug(dcSolarman) << "Sender IP doesn't match claimed IP:" << result.ip << host;
            continue;
        }

        if (m_discoveryTimer) {
            qCDebug(dcSolarman()) << "Found solarman device" << result.serial << "on" << result.ip;
            if (!m_discoveryResults.contains(result)) {
                m_discoveryResults.append(result);
            }
        }

        if (m_monitorTimer) {
            if (result.serial == m_monitoringResult.serial) {
                m_monitoringResult = result;
                m_monitoringResult.lastSeen = QDateTime::currentDateTime();
                emit monitorStateChanged(result);
            }
        }
    }
}

bool SolarmanDiscovery::initUdp()
{
    if (m_udp) {
        return true;
    }

    m_udp = new QUdpSocket(this);
    connect(m_udp, &QUdpSocket::readyRead, this, &SolarmanDiscovery::onReadyRead);
    bool status = m_udp->bind(48899, QUdpSocket::ShareAddress);
    if (!status) {
        qCWarning(dcSolarman()) << "Unable to bind UDP port 48899. SolarmanDiscovery won't work.";
        delete m_udp;
        m_udp  = nullptr;
    }
    return status;
}

bool SolarmanDiscovery::sendDiscoveryMessage()
{
    qCDebug(dcSolarman()) << "Discovering solarman inverters...";
    QByteArray discoveryString("WIFIKIT-214028-READ");
    int len = m_udp->writeDatagram(discoveryString, QHostAddress::Broadcast, 48899);
    return len == discoveryString.length();
}
