#ifndef SPEEDWIREDISCOVERY_H
#define SPEEDWIREDISCOVERY_H

#include <QTimer>
#include <QObject>
#include <QUdpSocket>

#include <network/networkdevicediscovery.h>

#include "speedwireinterface.h"

class SpeedwireDiscovery : public QObject
{
    Q_OBJECT
public:
    typedef struct SpeedwireDiscoveryResult {
        QHostAddress address;
        NetworkDeviceInfo networkDeviceInfo;
        SpeedwireInterface::DeviceType deviceType = SpeedwireInterface::DeviceTypeUnknown;
        quint16 modelId = 0;
        quint32 serialNumber = 0;
    } SpeedwireDiscoveryResult;

    explicit SpeedwireDiscovery(NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent = nullptr);
    ~SpeedwireDiscovery();

    bool initialize();
    bool initialized() const;

    bool startDiscovery();
    bool discoveryRunning() const;

    QList<SpeedwireDiscoveryResult> discoveryResult() const;

signals:
    void discoveryFinished();

private:
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;
    QUdpSocket *m_multicastSocket = nullptr;
    QUdpSocket *m_unicastSocket = nullptr;
    QHostAddress m_multicastAddress = QHostAddress("239.12.255.254");
    quint16 m_port = 9522;
    bool m_initialized = false;

    // Discovery
    QTimer m_discoveryTimer;
    bool m_discoveryRunning = false;
    NetworkDeviceInfos m_networkDeviceInfos;
    QHash<QHostAddress, SpeedwireDiscoveryResult> m_results;

    // Static discovery datagrams for speedwire
    QByteArray m_discoveryDatagramMulticast = QByteArray::fromHex("534d4100000402a0ffffffff0000002000000000");
    QByteArray m_discoveryResponseDatagram = QByteArray::fromHex("534d4100000402A000000001000200000001");

    QByteArray m_discoveryDatagramUnicast = QByteArray::fromHex("534d4100000402a00000000100260010606509a0ffffffffffff00007d0052be283a000000000000018000020000000000000000000000000000");

    void startMulticastDiscovery();
    void sendUnicastDiscoveryRequest(const QHostAddress &targetHostAddress);

private slots:
    void readPendingDatagramsMulticast();
    void readPendingDatagramsUnicast();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);

    void processDatagram(const QHostAddress &senderAddress, quint16 senderPort, const QByteArray &datagram);

    void sendDiscoveryRequest();

    void onDiscoveryProcessFinished();

};

#endif // SPEEDWIREDISCOVERY_H
