#include "discovery.h"

#include <QUdpSocket>
#include <QTimer>
#include <QMetaObject>

#include "extern-plugininfo.h"

Discovery::Discovery(QObject *parent) : QObject(parent)
{

}

void Discovery::discover()
{
    QUdpSocket *searchSocket = new QUdpSocket(this);

    // Note: This will fail, and it's not a problem, but it is required to force the socket to stick to IPv4...
    searchSocket->bind(QHostAddress::AnyIPv4, 30303);

    QString discoveryString = "Durchsuchen: Wer ist da?";
    qint64 len = searchSocket->writeDatagram(discoveryString.toUtf8(), QHostAddress("255.255.255.255"), 30303);
    if (len != discoveryString.length()) {
        searchSocket->deleteLater();
        qCWarning(dcAnelElektronik()) << "Error sending discovery";
        QTimer::singleShot(0, this, [=](){
            emit finished(true);
        });
        return;
    }

    QTimer::singleShot(2000, this, [this, searchSocket](){
        while(searchSocket->hasPendingDatagrams()) {
            char buffer[1024];
            QHostAddress senderAddress;
            int len = searchSocket->readDatagram(buffer, 1024, &senderAddress);
            QByteArray data = QByteArray::fromRawData(buffer, len);
            qCDebug(dcAnelElektronik()) << "Have datagram:" << data;
            if (!data.startsWith("NET-CONTROL")) {
                qCDebug(dcAnelElektronik()) << "Failed to parse discovery datagram from" << senderAddress << data;
                continue;
            }
            QStringList parts = QString(data).split("\r\n");
            if (parts.count() != 4) {
                qCDebug(dcAnelElektronik()) << "Failed to parse discovery datagram from" << senderAddress << data;
                continue;
            }
            qCDebug(dcAnelElektronik()) << "Found NET-CONTROL:" << senderAddress << parts.at(0) << parts.at(1) << parts.at(2) << parts.at(3) << senderAddress.protocol();
            Result result;
            result.name = parts.at(2);
            result.macAddress = parts.at(1);
            result.ipAddress = senderAddress.toString();
            result.port = parts.at(3).toInt();

            m_results.insert(result.macAddress, result);
        }
        emit finished(false);
        searchSocket->deleteLater();
    });
}

QHash<QString, Discovery::Result> Discovery::results() const
{
    return m_results;
}
