#ifndef WEBOSCONNECTION_H
#define WEBOSCONNECTION_H

#include <QObject>
#include <QWebSocket>

class WebosConnection : public QObject
{
    Q_OBJECT
public:
    explicit WebosConnection(QObject *parent = nullptr);

    QHostAddress hostAddress() const;
    void setHostAddress(const QHostAddress &hostAddress);

    QString apiKey() const;
    void setApiKey(const QString &apiKey);

    bool connected() const;

private:
    QHostAddress m_hostAddress;
    QWebSocket *m_websocket = nullptr;

    QString m_apiKey;
    int m_id = 0;

    void sendRequest(const QVariantMap &request);
    void getVolume();

signals:
    void connectedChanged(bool connected);

private slots:
    void onConnected();
    void onDisconnected();
    void onStateChanged(const QAbstractSocket::SocketState &state);
    void onTextMessageReceived(const QString &message);

public slots:
    void registerClient();
    void connectTv();


};

#endif // WEBOSCONNECTION_H
