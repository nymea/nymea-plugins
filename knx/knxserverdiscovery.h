#ifndef KNXSERVERDISCOVERY_H
#define KNXSERVERDISCOVERY_H

#include <QObject>
#include <QHostAddress>
#include <QKnxNetIpServerDiscoveryAgent>
#include <QKnxNetIpServerDescriptionAgent>

class KnxServerDiscovery : public QObject
{
    Q_OBJECT
public:
    explicit KnxServerDiscovery(QObject *parent = nullptr);

    QList<QKnxNetIpServerInfo> discoveredServers() const;

    static QString serviceFamilyToString(QKnx::NetIp::ServiceFamily id);
    static void printServerInfo(const QKnxNetIpServerInfo &serverInfo);

private:
    int m_discoveryTimeout = 5000;
    QList<QKnxNetIpServerDiscoveryAgent *> m_runningDiscoveryAgents;
    QList<QKnxNetIpServerInfo> m_discoveredServers;

signals:
    void discoveryFinished();

private slots:
    void onDiscoveryAgentErrorOccured(QKnxNetIpServerDiscoveryAgent::Error error);
    void onDiscoveryAgentFinished();


public slots:
    bool startDisovery();

};

#endif // KNXSERVERDISCOVERY_H
