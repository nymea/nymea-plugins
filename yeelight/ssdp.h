#ifndef SSDP_H
#define SSDP_H

#include <QObject>

#include <QUdpSocket>
#include <QUrl>
#include <QTimer>
#include <QUdpSocket>
#include <QHostAddress>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

class Ssdp : public QObject
{
    Q_OBJECT
public:
    explicit Ssdp(QObject *parent = nullptr);
    ~Ssdp();
    bool enable();
    void discover();
    bool disable();
private:
    QUdpSocket *m_socket = nullptr;
    QHostAddress m_host;
    quint16 m_port;

    QTimer *m_notificationTimer = nullptr;

    QNetworkAccessManager *m_networkAccessManager = nullptr;

    //QList<UpnpDiscoveryRequest *> m_discoverRequests;
   // QHash<QNetworkReply*, UpnpDeviceDescriptor> m_informationRequestList;

    bool m_available = false;
    bool m_enabled = false;

signals:
    void discovered(const QString &address, int port, int id, const QString &model);

private slots:
    void error(QAbstractSocket::SocketError error);
    void readData();
};

#endif // SSDP_H
