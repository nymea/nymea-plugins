#ifndef BROADCASTPING_H
#define BROADCASTPING_H

#include <QObject>
#include <QHash>
#include <QProcess>
#include <QNetworkAddressEntry>

class BroadcastPing : public QObject
{
    Q_OBJECT
public:
    explicit BroadcastPing(QObject *parent = nullptr);

signals:
    void finished();

public slots:
    void run();

private slots:
    void broadcastPingFinished(int exitCode);

private:
    QHash<QProcess*, QNetworkAddressEntry> m_runningPings;
};

#endif // BROADCASTPING_H
