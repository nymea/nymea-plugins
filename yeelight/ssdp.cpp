#include "ssdp.h"
#include "extern-plugininfo.h"

#include <QMetaObject>
#include <QNetworkInterface>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QNetworkConfiguration>
#include <QNetworkConfigurationManager>

Ssdp::Ssdp(QObject *parent) : QObject(parent)
{

}

Ssdp::~Ssdp()
{
    if (m_socket) {
        m_socket->waitForBytesWritten(1000);
        m_socket->close();
    }
}

bool Ssdp::enable()
{
    // Clean up
    if (m_socket) {
        delete m_socket;
        m_socket = nullptr;
    }

    // Bind udp socket and join multicast group
    m_socket = new QUdpSocket(this);
    m_port = 1982;
    m_host = QHostAddress("239.255.255.250");

    m_socket->setSocketOption(QAbstractSocket::MulticastTtlOption,QVariant(1));
    m_socket->setSocketOption(QAbstractSocket::MulticastLoopbackOption,QVariant(1));

    if(!m_socket->bind(QHostAddress::AnyIPv4, m_port, QUdpSocket::ShareAddress)){
        qCWarning(dcYeelight()) << "could not bind to port" << m_port;
        m_available = false;
        delete m_socket;
        m_socket = nullptr;
        return false;
    }

    if(!m_socket->joinMulticastGroup(m_host)){
        qCWarning(dcYeelight()) << "could not join multicast group" << m_host;
        m_available = false;
        delete m_socket;
        m_socket = nullptr;
        return false;
    }
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));
    connect(m_socket, &QUdpSocket::readyRead, this, &Ssdp::readData);
    return true;
}

void Ssdp::discover()
{
   QByteArray searchMessage = QByteArray("M-SEARCH * HTTP/1.1\r\n"
                                              "HOST: 239.255.255.250:1982\r\n"
                                              "MAN: \"ssdp:discover\"\r\n"
                                              "ST: wifi_bulb\r\n\r\n");
    m_socket->writeDatagram(searchMessage, m_host, m_port);
}


void Ssdp::error(QAbstractSocket::SocketError error)
{
    qCWarning(dcYeelight()) << "socket error:" << error << m_socket->errorString();
}

void Ssdp::readData()
{
    QByteArray data;
    quint16 port;
    QHostAddress hostAddress;
    QUrl location;
    int id;
    QString model;

    // read the answere from the multicast
    while (m_socket->hasPendingDatagrams()) {
        data.resize(m_socket->pendingDatagramSize());
        m_socket->readDatagram(data.data(), data.size(), &hostAddress, &port);
    }
    qCDebug(dcYeelight())<< "SSDP message received " << data;

    if (data.contains("NOTIFY") && !QNetworkInterface::allAddresses().contains(hostAddress)) {
        return;
    }

    // if the data contains the HTTP OK header...
    if (data.contains("HTTP/1.1 200 OK")) {
        const QStringList lines = QString(data).split("\r\n");
        foreach (const QString& line, lines) {
            int separatorIndex = line.indexOf(':');
            QString key = line.left(separatorIndex).toUpper();
            QString value = line.mid(separatorIndex+1).trimmed();

            if (key.contains("Location")) {
                location = QUrl(value);
            } else if (key.contains("id")) {
                id = value.toUInt();
            } else if (key.contains("model")) {

            }
        }
        emit discovered(location.host(), location.port(), id, model);
    }
}


bool Ssdp::disable()
{
    if (!m_socket) {
        return false;
    }

    m_socket->waitForBytesWritten();
    m_socket->close();
    delete m_socket;
    m_socket = nullptr;

    m_notificationTimer->stop();
    return true;
}

