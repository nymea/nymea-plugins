#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <QObject>
#include <QProcess>
#include <QHostInfo>
#include <QTimer>

#include "host.h"

class Discovery : public QObject
{
    Q_OBJECT
public:
    explicit Discovery(QObject *parent = nullptr);

    void discoverHosts(int timeout);
    void abort();

    bool isRunning() const;


signals:
    void finished(QList<Host> hosts);

private:
    QStringList getDefaultTargets();

    void finishDiscovery();

private slots:
    void discoveryFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void hostLookupDone(const QHostInfo &info);
    void arpLookupDone(int exitCode, QProcess::ExitStatus exitStatus);
    void onTimeout();

private:
    QProcess * m_discoveryProcess = nullptr;
    QTimer m_timeoutTimer;

    QHash<QProcess*, Host*> m_pendingArpLookups;
    QHash<QString, Host*> m_pendingNameLookups;
    QList<Host*> m_scanResults;

};

#endif // DISCOVERY_H
