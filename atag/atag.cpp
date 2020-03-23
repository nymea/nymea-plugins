/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "atag.h"
#include "extern-plugininfo.h"

#include <QJsonObject>
#include <QJsonDocument>


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
        qCDebug(dcAtag()) << "Get info" << reply->readAll();
    });
}

void Atag::getInfo(Atag::InfoCategory infoCategory)
{
    QUrl url;
    url.setUrl(m_address.toString());
    url.setScheme("http");
    url.setPath("/retrieve");
    url.setPort(m_port);
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
    QJsonDocument doc;
    QJsonObject message;
    message.insert("seqnr", 1);
    message.insert("device_id", "id");
    message.insert("info", infoCategory);
    QJsonObject retrieve;
    retrieve.insert("retrieve_message", message);
    doc.setObject(retrieve);
    qCDebug(dcAtag()) << "Sending request" << url.toString() << doc;
    QNetworkReply *reply = m_networkManager->post(request, doc.toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        qCDebug(dcAtag()) << "Get info" << reply->readAll();
    });
}

void Atag::onRefreshTimer()
{

}
