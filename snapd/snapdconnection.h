#ifndef SNAPDCONNECTION_H
#define SNAPDCONNECTION_H

#include <QQueue>
#include <QObject>
#include <QLocalSocket>

#include "snapdreply.h"

class SnapdConnection : public QLocalSocket
{
    Q_OBJECT
public:
    explicit SnapdConnection(QObject *parent = nullptr);

    SnapdReply *get(const QString &path);
    SnapdReply *post(const QString &path, const QByteArray &payload);

    bool isConnected() const;

private:
    bool m_chuncked = false;

    QByteArray m_header;
    QByteArray m_payload;

    bool m_connected = false;
    bool m_debug = false;

    SnapdReply *m_currentReply = nullptr;
    QQueue<SnapdReply *> m_replyQueue;

    void setConnected(const bool &connected);

    // Helper methods
    QByteArray createRequestHeader(const QString &method, const QString &path, const QByteArray &payload = QByteArray());
    QByteArray getChunckedPayload(const QByteArray &payload);

    void processData();
    void sendNextRequest();

private slots:
    void onConnected();
    void onDisconnected();
    void onError(const QLocalSocket::LocalSocketError &socketError);
    void onStateChanged(const QLocalSocket::LocalSocketState &state);
    void onReadyRead();

signals:
    void connectedChanged(const bool &connected);

};

#endif // SNAPDCONNECTION_H
