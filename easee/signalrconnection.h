#ifndef SIGNALRCONNECTION_H
#define SIGNALRCONNECTION_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QWebSocket>
#include <QTimer>
#include <network/networkaccessmanager.h>

class SignalRConnection : public QObject
{
    Q_OBJECT
public:
    explicit SignalRConnection(const QUrl &url, const QByteArray &accessToken, NetworkAccessManager *nam, QObject *parent = nullptr);

    void subscribe(const QString &chargerId);
    bool connected() const;

    void updateToken(const QByteArray &accessToken);

signals:
    void connectionStateChanged(bool connected);
    void dataReceived(const QVariantMap &data);

private:
    QByteArray encode(const QVariantMap &message);

private slots:
    void connectToHost();

private:
    QUrl m_url;
    QByteArray m_accessToken;
    NetworkAccessManager *m_nam = nullptr;
    QWebSocket *m_socket = nullptr;
    QTimer *m_watchdog = nullptr;

    bool m_waitingForHandshakeReply = false;
};

#endif // SIGNALRCONNECTION_H
