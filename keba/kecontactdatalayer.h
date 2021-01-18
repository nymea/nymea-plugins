#ifndef KECONTACTDATALAYER_H
#define KECONTACTDATALAYER_H

#include <QObject>
#include <QUdpSocket>

class KeContactDataLayer : public QObject
{
    Q_OBJECT
public:
    explicit KeContactDataLayer(QObject *parent = nullptr);
    bool init();

    void write(const QHostAddress &address, const QByteArray &data);

private:
    int m_port = 7090;
    QUdpSocket *m_udpSocket = nullptr;

signals:
    void datagramReceived(const QHostAddress &address, const QByteArray &data);

private slots:
    void readPendingDatagrams();
};

#endif // KECONTACTDATALAYER_H
