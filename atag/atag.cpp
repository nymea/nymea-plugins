
#include "atag.h"
#include "extern-plugininfo.h"

Atag::Atag(NetworkAccessManager *networkManager, const QHostAddress &address, int port, const QString &macAddress, QObject *parent) :
    QObject(parent),
    m_networkManager(networkManager),
    m_address(address),
    m_port(port),
    m_macAddress(macAddress)
{

}

void Atag::requestAuthorization()
{
    QUrl url;
    url.setUrl(m_address.toString());
    url.setScheme("http");
    url.setPath("/pair_message");
    url.setPort(m_port);
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
    QByteArray body;
    body.append("{\"pair_message\": {\"seqnr\": 1, \"account_auth\": {\"user_account\": \"\",\"mac_address\": \"TODO\"}, \"accounts\": {\"entries\": [{ \"user_account\": \"\",\"mac_address\": \"TODO\",\"device_name\": \"nymea\",\"account_type\": 0} ]}}}""");
    qCDebug(dcAtag()) << "Sending request" << url.toString() << body;
    QNetworkReply *reply = m_networkManager->post(request, body);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
    });
}

void Atag::onRefreshTimer()
{

}
