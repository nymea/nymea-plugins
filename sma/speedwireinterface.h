#ifndef SPEEDWIREINTERFACE_H
#define SPEEDWIREINTERFACE_H

#include <QObject>
#include <QUdpSocket>
#include <QDataStream>




class SpeedwireInterface : public QObject
{
    Q_OBJECT
public:
    enum ProtocolId {
        ProtocolIdUnknown = 0x0000,
        ProtocolIdMeter = 0x6069,
        ProtocolIdInverter = 0x6065,
        ProtocolIdDiscoveryResponse = 0x0001,
        ProtocolIdDiscovery = 0xffff
    };
    Q_ENUM(ProtocolId)

    enum DeviceType {
        DeviceTypeUnknown,
        DeviceTypeMeter,
        DeviceTypeInverter
    };
    Q_ENUM(DeviceType)

    class SpeedwireHeader
    {
    public:
        SpeedwireHeader() = default;
        quint32 smaSignature = 0;
        quint16 headerLength = 0;
        quint16 tagType = 0;
        quint16 tagVersion = 0;
        quint16 group = 0;
        quint16 payloadLength = 0;
        quint16 smaNet2Version = 0;
        ProtocolId protocolId = ProtocolIdUnknown;

        inline bool isValid() const {
            return smaSignature == 0x534d4100 && protocolId != ProtocolIdUnknown;
        }

    };


    explicit SpeedwireInterface(const QHostAddress &address, bool multicast, QObject *parent = nullptr);
    ~SpeedwireInterface();

    bool initialize();
    void deinitialize();

    bool initialized() const;

    quint16 sourceModelId() const;
    quint32 sourceSerialNumber() const;

    static SpeedwireInterface::SpeedwireHeader parseHeader(QDataStream &stream);

public slots:
    void sendData(const QByteArray &data);

signals:
    void dataReceived(const QByteArray &data);

private:
    QUdpSocket *m_socket = nullptr;
    QHostAddress m_address;
    quint16 m_port = 9522;
    QHostAddress m_multicastAddress = QHostAddress("239.12.255.254");
    bool m_multicast = false;
    bool m_initialized = false;

    // Requester
    quint16 m_sourceModelId = 0x007d;
    quint32 m_sourceSerialNumber = 0x3a28be42;

private slots:
    void readPendingDatagrams();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);

};

#endif // SPEEDWIREINTERFACE_H
