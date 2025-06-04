#ifndef EVERESTJSONRPCCLIENT_H
#define EVERESTJSONRPCCLIENT_H

#include <QObject>
#include <QWebSocket>

class EverestJsonRpcClient : public QObject
{
    Q_OBJECT
public:
    explicit EverestJsonRpcClient(QObject *parent = nullptr);
    ~EverestJsonRpcClient();

    void sendData(const QByteArray &data);

public slots:
    void connectServer(const QUrl &serverUrl);
    void disconnectServer();

signals:
    void connectedChanged(bool connected);
    void dataReceived(const QByteArray &data);

private slots:
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void onStateChanged(QAbstractSocket::SocketState state);
    void onTextMessageReceived(const QString &message);

private:
    QWebSocket *m_webSocket = nullptr;
    QUrl m_serverUrl;
    bool m_connected = false;
};

#endif // EVERESTJSONRPCCLIENT_H
