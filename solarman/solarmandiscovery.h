#ifndef SOLARMANDISCOVERY_H
#define SOLARMANDISCOVERY_H

#include <QObject>
#include <QHostAddress>
#include <QTimer>
#include <QUdpSocket>
#include <QDateTime>

class SolarmanDiscovery : public QObject
{
    Q_OBJECT
public:
    class DiscoveryResult {
    public:
        QString serial;
        QHostAddress ip;
        QString mac;
        bool online;
        QDateTime lastSeen;
        bool operator==(const DiscoveryResult &other) {
            return serial == other.serial;
        }
    };

    explicit SolarmanDiscovery(QObject *parent = nullptr);

    bool discover();

    bool monitor(const QString &serial);
    DiscoveryResult monitorResult() const;

signals:
    void discoveryResults(const QList<DiscoveryResult> &results);
    void monitorStateChanged(const DiscoveryResult &result);

private slots:
    void onReadyRead();

private:
    bool initUdp();
    bool sendDiscoveryMessage();

    QList<DiscoveryResult> m_discoveryResults;

    QUdpSocket *m_udp = nullptr;;
    DiscoveryResult m_monitoringResult;
    QTimer *m_discoveryTimer = nullptr;
    QTimer *m_monitorTimer = nullptr;
};

#endif // SOLARMANDISCOVERY_H
