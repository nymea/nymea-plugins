#include <QGuiApplication>

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QTcpServer>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QDataStream>

#define MODEL "HS300"

QByteArray decryptPayload(const QByteArray &payload)
{
    QByteArray result;
    int k = 171;
    for (int i = 0; i < payload.length(); i++){
        char t = payload.at(i);
        result.append(t xor k);
        k = t;
    }
    return result;
}

QByteArray encryptPayload(const QByteArray &payload)
{
    QByteArray result;
    int k = 171;
    for (int i = 0; i < payload.length(); i++){
        char t = payload.at(i) xor k;
        k = t;
        result.append(t);
    }
    return result;
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    qDebug() << "Simulating" << MODEL;

    QFile f(QString("%1-UDP.txt").arg(MODEL));
    f.open(QFile::ReadOnly);
    QByteArray systemDataUdp = f.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(systemDataUdp);
    systemDataUdp = encryptPayload(jsonDoc.toJson(QJsonDocument::Compact));
    f.close();

    f.setFileName(QString("%1-TCP.txt").arg(MODEL));
    f.open(QFile::ReadOnly);
    QByteArray systemDataTcp = f.readAll();
    jsonDoc = QJsonDocument::fromJson(systemDataTcp);
    systemDataTcp = encryptPayload(jsonDoc.toJson(QJsonDocument::Compact));

    QUdpSocket *udp = new QUdpSocket(&app);
    udp->bind(QHostAddress("0.0.0.0"), 9999);
    QObject::connect(udp, &QUdpSocket::readyRead, &app, [=](){
        while (udp->hasPendingDatagrams()) {
            char buffer[4096];
            QHostAddress sourceAddress;
            quint16 sourcePort;
            udp->readDatagram(buffer, 4096, &sourceAddress, &sourcePort);
            QByteArray data(buffer);
            data = decryptPayload(data);
            qDebug() << "Incoming datagram:" << data << "from" << sourceAddress;

            udp->writeDatagram(systemDataUdp, sourceAddress, sourcePort);
        }
    });

    QTcpServer *tcp = new QTcpServer(&app);
    bool success = tcp->listen(QHostAddress("0.0.0.0"), 9999);
    qDebug() << "Listening on tcp" << success;
    QObject::connect(tcp, &QTcpServer::newConnection, &app, [=](){
        qDebug() << "new connection";
        QTcpSocket *client = tcp->nextPendingConnection();
        QObject::connect(client, &QTcpSocket::readyRead, tcp, [=](){
            QByteArray data = decryptPayload(client->readAll());
            qDebug() << "Incoming request:" << data;

            QByteArray output;
            QDataStream stream(&output, QIODevice::WriteOnly);
            qint32 len = systemDataTcp.length();
            stream << len;
            output.append(systemDataTcp);
            qDebug() << "written:" << client->write(output);

        });
    });

    return app.exec();
}
