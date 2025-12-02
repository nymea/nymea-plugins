// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "ufo.h"
#include "extern-plugininfo.h"

#include <QUrlQuery>
#include <QJsonDocument>

Ufo::Ufo(NetworkAccessManager *networkManager, const QHostAddress &address, QObject *parent) :
    QObject(parent),
    m_networkManager(networkManager),
    m_address(address)
{

}

void Ufo::getId()
{
    QUrl url;
    url.setScheme("http");
    url.setHost(m_address.toString());
    url.setPath("/info", QUrl::ParsingMode::TolerantMode);
    QNetworkRequest request;
    request.setUrl(url);
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
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError){
            qCWarning(dcDynatrace()) << "JSON parsing error:" << error.errorString();
            return;
        }

        QString id = data.toVariant().toMap().value("ufoid").toString();
        emit idReceived(id);
    });
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
    QUrlQuery query;
    if (top) {
       query.addQueryItem("top_init", "0");
    }
    if (bottom) {
       query.addQueryItem("bottom_init", "0");
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

void Ufo::setBackgroundColor(bool top, bool initTop, bool bottom, bool initBottom, QColor color)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(m_address.toString());
    url.setPath("/api");
    QUrlQuery query;
    if (initTop){
        query.addQueryItem("top_init", "0");
    }
    if (initBottom){
        query.addQueryItem("bottom_init", "0");
    }
    if (top){
        query.addQueryItem("top_bg", color.name().remove(0,1));
    }
    if (bottom) {
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

void Ufo::startWhirl(bool top, bool bottom, QColor color, int speed, bool clockwise)
{
    Q_UNUSED(clockwise)
    QUrl url;
    url.setScheme("http");
    url.setHost(m_address.toString());
    url.setPath("/api");
    QUrlQuery query;
    if (top){
        query.addQueryItem("top_init", "0");
        query.addQueryItem("top_bg", color.name().remove(0,1));
        query.addQueryItem("top", "0|8|000000");
        query.addQueryItem("top_whirl", QString::number(speed));
    }
    if (bottom) {
        query.addQueryItem("bottom_init", "0");
        query.addQueryItem("bottom_bg", color.name().remove(0,1));
        query.addQueryItem("bottom", "0|8|000000");
        query.addQueryItem("bottom_whirl", QString::number(speed));
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

void Ufo::startMorph(bool top, bool bottom, QColor color, int time, int speed)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(m_address.toString());
    url.setPath("/api");
    QUrlQuery query;
    if (top){
        query.addQueryItem("top_init", "0");
        query.addQueryItem("top_bg", color.name().remove(0,1));
        query.addQueryItem("top", "0|16|000000");
        query.addQueryItem("top_morph", QString::number(time)+"|"+QString::number(speed));
    }
    if (bottom) {
        query.addQueryItem("bottom_init", "0");
        query.addQueryItem("bottom_bg", color.name().remove(0,1));
        query.addQueryItem("bottom", "0|16|000000");
        query.addQueryItem("bottom_morph", QString::number(time)+"|"+QString::number(speed));
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
