#ifndef DEVICEMONITOR_H
#define DEVICEMONITOR_H

#include <QObject>
#include <QProcess>

#include "host.h"

class DeviceMonitor : public QObject
{
    Q_OBJECT
public:
    explicit DeviceMonitor(const QString &macAddress, const QString &ipAddress, QObject *parent = nullptr);

    ~DeviceMonitor();

    void update();

signals:
    void addressChanged(const QString &address);
    void reachableChanged(bool reachable);

private:
    void lookupArpCache();
    void ping();

private slots:
    void arpLookupFinished(int exitCode);
    void pingFinished(int exitCode);

private:
    Host *m_host;
    QProcess *m_arpLookupProcess;
    QProcess *m_pingProcess;

};

#endif // DEVICEMONITOR_H
