#include "broadcastping.h"
#include "extern-plugininfo.h"

#include <QNetworkInterface>
#include <QProcess>

BroadcastPing::BroadcastPing(QObject *parent) : QObject(parent)
{

}

void BroadcastPing::run()
{
    qDeleteAll(m_runningPings.keys());
    m_runningPings.clear();

    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        if (!interface.flags().testFlag(QNetworkInterface::IsUp) || !interface.flags().testFlag(QNetworkInterface::CanBroadcast) || interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            continue;
        }
        foreach (const QNetworkAddressEntry &addressEntry, interface.addressEntries()) {
            if (addressEntry.broadcast().isNull()) {
                continue;
            }
            qCDebug(dcNetworkDetector()) << "Sending Broadcast Ping on" << addressEntry.broadcast().toString() + '/' + QString::number(addressEntry.prefixLength()) + "...";
            QProcess *p = new QProcess(this);
            m_runningPings.insert(p, addressEntry);
            p->start("fping", {"-a", "-c", "1", "-g", addressEntry.broadcast().toString() + "/" + QString::number(addressEntry.prefixLength())});
            connect(p, SIGNAL(finished(int)), this, SLOT(broadcastPingFinished(int)));
        }
    }
    if (m_runningPings.isEmpty()) {
        qCWarning(dcNetworkDetector()) << "Cound not find any suitable interface for broadcast pinging";
        emit finished();
    }
}

void BroadcastPing::broadcastPingFinished(int exitCode)
{
    Q_UNUSED(exitCode);
    QProcess *p = static_cast<QProcess*>(sender());
    QNetworkAddressEntry addressEntry = m_runningPings.take(p);
    qCDebug(dcNetworkDetector()) << "Broadcast ping finished for network" << addressEntry.broadcast().toString() + '/' + QString::number(addressEntry.prefixLength());
//        qCDebug(dcNetworkDetector()) << p->readAllStandardError();
    p->deleteLater();

    if (m_runningPings.isEmpty()) {
        qCDebug(dcNetworkDetector()) << "All broadcast pings finished";
        emit finished();
    }
}
