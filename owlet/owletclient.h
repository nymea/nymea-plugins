#ifndef OWLETCLIENT_H
#define OWLETCLIENT_H

#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>

class OwletTransport;

class OwletClient : public QObject
{
    Q_OBJECT
public:
    explicit OwletClient(OwletTransport *transport, QObject *parent = nullptr);

    bool isConnected() const;

    OwletTransport *transport() const;

    int sendCommand(const QString &method, const QVariantMap &params = QVariantMap());

signals:
    void connected();
    void disconnected();
    void error();

    void replyReceived(int commandId, const QVariantMap &params);
    void notificationReceived(const QString &name, const QVariantMap &params);

private slots:
    void dataReceived(const QByteArray &data);

private:
    OwletTransport *m_transport = nullptr;
    int m_commandId = 0;

    QByteArray m_receiveBuffer;

};

#endif // OWLETCLIENT_H
