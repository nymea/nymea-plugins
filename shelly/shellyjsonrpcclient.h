#ifndef SHELLYJSONRPCCLIENT_H
#define SHELLYJSONRPCCLIENT_H

#include <QObject>
#include <QWebSocket>

class ShellyRpcReply: public QObject
{
    Q_OBJECT
public:
    enum Status {
        StatusSuccess = 0,

        // Shelly common error codes
        StatusInvalidArgument = -103,
        StatusDeadlineExceeded = -104,
        StatusResourceExhausted = -108,
        StatusFailedPrecondition = -109,
        StatusUnavailable = -114,

        StatusCodeBadRequest = 400,
        StatusAuthenticationRequired = 401,

        // Our own
        StatusTimeout = -1

    };
    Q_ENUM(Status)

    explicit ShellyRpcReply(const QVariantMap &requestBody, QObject *parent = nullptr);

    int id() const;
    QString method() const;
    QVariantMap requestBody() const;

signals:
    void finished(Status status, const QVariantMap &response);

private:
    QVariantMap m_requestBody;
};

class ShellyJsonRpcClient : public QObject
{
    Q_OBJECT
public:
    explicit ShellyJsonRpcClient(QObject *parent = nullptr);

    void open(const QHostAddress &address, const QString &user, const QString &password, const QString &shellyId);

    ShellyRpcReply* sendRequest(const QString &method, const QVariantMap &params = QVariantMap());

signals:
    void stateChanged(QAbstractSocket::SocketState state);
    void notificationReceived(const QVariantMap &notification);

private slots:
    void onTextMessageReceived(const QString &message);

private:
    QVariantMap createAuthMap() const;

    QWebSocket *m_socket = nullptr;
    QHash<int, ShellyRpcReply*> m_pendingReplies;

    int m_currentId = 1;

    // Needed (only) for authentication
    QString m_user;
    QString m_password;
    QString m_shellyId;
    qulonglong m_nonce = 0;
    int m_nc = 0;
};

#endif // SHELLYJSONRPCCLIENT_H
