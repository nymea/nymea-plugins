#ifndef SPEEDWIREINTERFACE_H
#define SPEEDWIREINTERFACE_H

#include <QObject>
#include <QUdpSocket>

class SpeedwireInterface : public QObject
{
    Q_OBJECT
public:
    explicit SpeedwireInterface(QObject *parent = nullptr);
    ~SpeedwireInterface();

    bool initialize();
    bool initialized() const;

    bool startDiscovery();
    bool discoveryRunning() const;

signals:
    void discoveryFinished();

private:
    QUdpSocket *m_socket = nullptr;
    QHostAddress m_multicastAddress = QHostAddress("239.12.255.254");
    quint16 m_port = 9522;
    bool m_initialized = false;
    bool m_discoveryRunning = false;

private slots:
    void readPendingDatagrams();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);

};

#endif // SPEEDWIREINTERFACE_H
