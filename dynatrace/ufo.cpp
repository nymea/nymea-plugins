/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by copyright law, and
* remains the property of nymea GmbH. All rights, including reproduction, publication,
* editing and translation, are reserved. The use of this project is subject to the terms of a
* license agreement to be concluded with nymea GmbH in accordance with the terms
* of use of nymea GmbH, available under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* This project may also contain libraries licensed under the open source software license GNU GPL v.3.
* Alternatively, this project may be redistributed and/or modified under the terms of the GNU
* Lesser General Public License as published by the Free Software Foundation; version 3.
* this project is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along with this project.
* If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under contact@nymea.io
* or see our FAQ/Licensing Information on https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "ufo.h"
#include "extern-plugininfo.h"
#include <QUrlQuery>

Ufo::Ufo(NetworkAccessManager *networkManager, const QHostAddress &address, QObject *parent) :
    QObject(parent),
    m_networkManager(networkManager),
    m_address(address)
{

}

void Ufo::resetLogo()
{
    QUrl url;
    url.setScheme("http");
    url.setHost(m_address.toString());
    url.setPath("/api");
    url.setQuery("logo_reset");
    QNetworkRequest request;
    request.setUrl(url);
    qCDebug(dcDynatrace()) << "Sending request" << url;
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDynatrace()) << "Request error:" << status << reply->errorString();
            emit connectionChanged(false);
            return;
        }
        emit connectionChanged(true);
    });

}

void Ufo::setLogo(QColor led1, QColor led2, QColor led3, QColor led4)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(m_address.toString());
    url.setPath("/api");
    QUrlQuery query;
    query.addQueryItem("logo", led1.name().remove(0,1)+"|"+led2.name().remove(0,1)+"|"+led3.name().remove(0,1)+"|"+led4.name().remove(0,1));
    url.setQuery(query);
    QNetworkRequest request;
    request.setUrl(url);

    qCDebug(dcDynatrace()) << "Sending request" << url;
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDynatrace()) << "Request error:" << status << reply->errorString();
            emit connectionChanged(false);
            return;
        }
        emit connectionChanged(true);
    });
}

void Ufo::initBackgroundColor(bool top, bool bottom)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(m_address.toString());
    url.setPath("/api");
    if (top) {
        url.setQuery("top_init");
    }
    if (bottom) {
        url.setQuery("bottom_init");
    }
    QNetworkRequest request;
    request.setUrl(url);

    qCDebug(dcDynatrace()) << "Sending request" << url;
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDynatrace()) << "Request error:" << status << reply->errorString();
            emit connectionChanged(false);
            return;
        }
        emit connectionChanged(true);
    });
}

void Ufo::setBackgroundColor(bool top, bool bottom, QColor color)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(m_address.toString());
    url.setPath("/api");
    QUrlQuery query;
    if (top){
        query.addQueryItem("top_init", "0");
        query.addQueryItem("top_bg", color.name().remove(0,1));
    }
    if (bottom) {
        query.addQueryItem("bottom_init", "0");
        query.addQueryItem("bottom_bg", color.name().remove(0,1));
    }
    url.setQuery(query);
    QNetworkRequest request;
    request.setUrl(url);

    qCDebug(dcDynatrace()) << "Sending request" << url;
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDynatrace()) << "Request error:" << status << reply->errorString();
            emit connectionChanged(false);
            return;
        }
        emit connectionChanged(true);
    });
}
