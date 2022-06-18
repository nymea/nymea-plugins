#ifndef SHELLYJSONRPCCLIENT_H
#define SHELLYJSONRPCCLIENT_H

#include <QObject>
#include <QWebSocket>

class ShellyRpcReply: public QObject
{
    Q_OBJECT
public:
    enum Status {
        StatusSuccess,
        StatusTimeout
    };
    Q_ENUM(Status)

    explicit ShellyRpcReply(int id, QObject *parent = nullptr);

    int id() const;

signals:
    void finished(Status status, const QVariantMap &response);

private:
    int m_id = 0;
};

class ShellyJsonRpcClient : public QObject
{
    Q_OBJECT
public:
    explicit ShellyJsonRpcClient(QObject *parent = nullptr);

    void open(const QHostAddress &address);

    ShellyRpcReply* sendRequest(const QString &method);

signals:
    void stateChanged(QAbstractSocket::SocketState state);
    void notificationReceived(const QVariantMap &notification);

private slots:
    void onTextMessageReceived(const QString &message);

private:
    QWebSocket *m_socket = nullptr;
    QHash<int, ShellyRpcReply*> m_pendingReplies;

    int m_currentId = 1;
};

#endif // SHELLYJSONRPCCLIENT_H
