#include "knxserverdiscovery.h"
#include "extern-plugininfo.h"

#include <QNetworkInterface>
#include <QKnxNetIpExtendedDeviceDibProxy>

KnxServerDiscovery::KnxServerDiscovery(QObject *parent) :
    QObject(parent)
{

}


QList<QKnxNetIpServerInfo> KnxServerDiscovery::discoveredServers() const
{
    return m_discoveredServers;
}

QString KnxServerDiscovery::serviceFamilyToString(QKnx::NetIp::ServiceFamily id)
{
    switch (id) {
    case QKnxNetIp::ServiceFamily::Core:
        return "Core";
    case QKnxNetIp::ServiceFamily::DeviceManagement:
        return "Device Management";
    case QKnxNetIp::ServiceFamily::IpTunneling:
        return "Tunnel";
    case QKnxNetIp::ServiceFamily::IpRouting:
        return "Routing";
    case QKnxNetIp::ServiceFamily::RemoteLogging:
        return "Remote Logging";
    case QKnxNetIp::ServiceFamily::RemoteConfigAndDiagnosis:
        return "Remote Configuration";
    case QKnxNetIp::ServiceFamily::ObjectServer:
        return "Object Server";
    case QKnxNetIp::ServiceFamily::Security:
        return "Security";
    default:
        break;
    }
    return "Unknown";
}

void KnxServerDiscovery::printServerInfo(const QKnxNetIpServerInfo &serverInfo)
{
    qCDebug(dcKnx()) << "Server:" << serverInfo.deviceName();
    qCDebug(dcKnx()) << "    Individual address:" << serverInfo.individualAddress().toString();
    qCDebug(dcKnx()) << "    Server control endpoint:" << QString("%1:%2").arg(serverInfo.controlEndpointAddress().toString()).arg(serverInfo.controlEndpointPort());

    QVector<QKnxServiceInfo> services = serverInfo.supportedServices();
    qCDebug(dcKnx()) << "  Supported services:";
    foreach (const QKnxServiceInfo &service, services)
        qCDebug(dcKnx()) << "    KNXnet/IP" << serviceFamilyToString(service.ServiceFamily) << ", Version:" << service.ServiceFamilyVersion;

    qCDebug(dcKnx()) << "  Mask version:" << serverInfo.maskVersion();
    qCDebug(dcKnx()) << "  Endpoint:" << serverInfo.endpoint();

    QKnxNetIpDib dib = serverInfo.hardware();
    QKnxNetIpExtendedDeviceDibProxy hardware(dib);
    if (hardware.isValid()) {
        qCDebug(dcKnx()) << "  Extended hardware information:";
        qCDebug(dcKnx()).noquote() << QString::fromLatin1("      Mask version: %1").arg(hardware.deviceDescriptorType0(), 4, 16, QLatin1Char('0'));
        qCDebug(dcKnx()).noquote() << QString::fromLatin1("      Max. local APDU length: %1").arg(hardware.maximumLocalApduLength());

        auto status = serverInfo.mediumStatus();
        if (status == QKnx::MediumStatus::CommunicationPossible)
            qCDebug(dcKnx()) << "      Medium status: Communication possible";
        else if (status == QKnx::MediumStatus::CommunicationImpossible)
            qCDebug(dcKnx()) << "      Medium status: Communication impossible";
        else
            qCDebug(dcKnx()) << "      Medium status: Unknown";
    }
}

void KnxServerDiscovery::onDiscoveryAgentErrorOccured(QKnxNetIpServerDiscoveryAgent::Error error)
{
    QKnxNetIpServerDiscoveryAgent *discovery = static_cast<QKnxNetIpServerDiscoveryAgent *>(sender());
    qCDebug(dcKnx()) << "Discovery error occured" << discovery->localAddress().toString() << error << discovery->errorString();
}

void KnxServerDiscovery::onDiscoveryAgentFinished()
{
    QKnxNetIpServerDiscoveryAgent *discovery = static_cast<QKnxNetIpServerDiscoveryAgent *>(sender());
    qCDebug(dcKnx()) << "Discovery agent for" << discovery->localAddress() << "has finished";
    qCDebug(dcKnx()) << "Found" << discovery->discoveredServers().count() << "servers";
    foreach (const QKnxNetIpServerInfo &serverInfo, discovery->discoveredServers()) {
        m_discoveredServers.append(serverInfo);
    }

    m_runningDiscoveryAgents.removeAll(discovery);
    discovery->deleteLater();

    if (m_runningDiscoveryAgents.isEmpty()) {
        emit discoveryFinished();
    }
}

bool KnxServerDiscovery::startDisovery()
{
    if (!m_runningDiscoveryAgents.isEmpty()) {
        qCWarning(dcKnx()) << "Could not start discovery. There are still discovery agents running. (" << m_runningDiscoveryAgents.count() << "discoveries )";
        return false;
    }

    qCDebug(dcKnx()) << "Start KNX server discovery on all interfaces";
    m_discoveredServers.clear();

    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        //qCDebug(dcKnx()) << "Checking network interface" << interface.name() << interface.type();
        foreach (const QNetworkAddressEntry &addressEntry, interface.addressEntries()) {
            // If ipv4 and not local host
            if (addressEntry.ip().protocol() == QAbstractSocket::IPv4Protocol && !addressEntry.ip().isLoopback()) {
                qCDebug(dcKnx()) << "Start discovery on" << interface.name() << addressEntry.ip().toString();

                // Create discovery agent
                QKnxNetIpServerDiscoveryAgent *discovery = new QKnxNetIpServerDiscoveryAgent(this);
                discovery->setLocalAddress(addressEntry.ip());
                discovery->setLocalPort(0);
                discovery->setSearchFrequency(60);
                discovery->setResponseType(QKnxNetIpServerDiscoveryAgent::ResponseType::Unicast);
                discovery->setDiscoveryMode(QKnxNetIpServerDiscoveryAgent::DiscoveryMode::CoreV1 | QKnxNetIpServerDiscoveryAgent::DiscoveryMode::CoreV2);

                connect(discovery, &QKnxNetIpServerDiscoveryAgent::finished, this, &KnxServerDiscovery::onDiscoveryAgentFinished);
                connect(discovery, &QKnxNetIpServerDiscoveryAgent::errorOccurred, this, &KnxServerDiscovery::onDiscoveryAgentErrorOccured);
                m_runningDiscoveryAgents.append(discovery);

                // Start the discovery
                discovery->start(m_discoveryTimeout);
            }
        }
    }

    return true;
}
