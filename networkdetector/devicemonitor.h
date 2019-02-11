#ifndef DEVICEMONITOR_H
#define DEVICEMONITOR_H

#include <QObject>
#include <QProcess>

class DeviceMonitor : public QObject
{
    Q_OBJECT
public:
    explicit DeviceMonitor(const QString &name, const QString &macAddress, const QString &ipAddress, bool initialState, QObject *parent = nullptr);

    ~DeviceMonitor();

    void setGracePeriod(int minutes);

    void update();

signals:
    void addressChanged(const QString &address);
    void reachableChanged(bool reachable);
    void seen();

private:
    void lookupArpCache();
    void arping();
    void ping();

    void log(const QString &message);
    void warn(const QString &message);

private slots:
    void arpLookupFinished(int exitCode);
    void arpingFinished(int exitCode);
    void pingFinished(int exitCode);

private:
    QString m_name;
    QString m_macAddress;
    QString m_ipAddress;

    bool m_reachable = false;

    int m_gracePeriod = 5;

    QProcess *m_arpLookupProcess = nullptr;
    QProcess *m_arpingProcess = nullptr;
    QProcess *m_pingProcess = nullptr;
    int m_failedPings = 0;

};

#endif // DEVICEMONITOR_H
