#ifndef OWLETCLIENT_H
#define OWLETCLIENT_H

#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>
#include <QTimer>

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

    void sendNextRequest();

private:
    struct Command {
        int id;
        QVariantMap payload;
    };
    OwletTransport *m_transport = nullptr;
    quint16 m_commandId = 0;

    QByteArray m_receiveBuffer;

    QList<Command> m_commandQueue;
    qint32 m_pendingCommandId = -1;
    QTimer m_commandTimeoutTimer;

};

#endif // OWLETCLIENT_H
