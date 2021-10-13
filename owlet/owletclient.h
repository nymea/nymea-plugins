#ifndef OWLETCLIENT_H
#define OWLETCLIENT_H

#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>

class OwletClient : public QObject
{
    Q_OBJECT
public:
    explicit OwletClient(QObject *parent = nullptr);

    void connectToHost(const QHostAddress &address, int port);

    int sendCommand(const QString &method, const QVariantMap &params);

signals:
    void connected();
    void disconnected();
    void error();

    void replyReceived(int commandId, const QVariantMap &params);
    void notificationReceived(const QString &name, const QVariantMap &params);

private slots:
    void dataReceived(const QByteArray &data);

private:
    QTcpSocket *m_socket = nullptr;
    int m_commandId = 0;

    QByteArray m_receiveBuffer;

};

#endif // OWLETCLIENT_H
