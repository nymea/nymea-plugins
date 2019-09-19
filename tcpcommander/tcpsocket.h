#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>

class TcpSocket : public QObject
{
    Q_OBJECT
public:
    explicit TcpSocket(const QHostAddress address, const quint16 &port, QObject *parent = nullptr);
    void sendCommand(QByteArray command);

    void connectionTest();

private:
    QTcpSocket *m_tcpSocket = nullptr;

    quint16 m_port;
    QHostAddress m_address;

    QList<QByteArray> m_pendingCommands;

signals:
    void connectionChanged(bool connected);
    void commandSent(bool successfull);
    void connectionTestFinished(bool successfull);

private slots:

    void onConnected();
    void onDisconnected();
    void onBytesWritten();
    void onError(QAbstractSocket::SocketError error);

};

#endif // TCPSOCKET_H
