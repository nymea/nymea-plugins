/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "nanoleaf.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

Nanoleaf::Nanoleaf(NetworkAccessManager *networkManager, const QHostAddress &address, int port, QObject *parent) :
    QObject(parent),
    m_networkManager(networkManager),
    m_address(address),
    m_port(port)
{

}

void Nanoleaf::setIpAddress(const QHostAddress &address)
{
    m_address = address;
}

QHostAddress Nanoleaf::ipAddress()
{
    return m_address;
}

void Nanoleaf::setPort(int port)
{
    m_port = port;
}

int Nanoleaf::port()
{
    return m_port;
}

void Nanoleaf::setAuthToken(const QString &token)
{
    m_authToken = token;
}

QString Nanoleaf::authToken()
{
    return m_authToken;
}

void Nanoleaf::addUser()
{
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath("/api/v1/new");

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_networkManager->post(request, "");
    qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status >= 400 && status <= 410) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcNanoleaf()) << "Recieved invalide JSON object";
            return;
        }

        m_authToken = data.toVariant().toMap().value("auth_token").toString();

        emit authTokenRecieved(m_authToken);
        emit authenticationStatusChanged(true);
    });
}

void Nanoleaf::deleteUser()
{
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath("/api/v1/"+m_authToken);

    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = m_networkManager->deleteResource(request);
    qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 204 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit authenticationStatusChanged(false);
    });
}

void Nanoleaf::getControllerInfo()
{
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath("/api/v1/"+m_authToken);

    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = m_networkManager->get(request);
    qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 204 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            emit authenticationStatusChanged(false);
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
    });
}

void Nanoleaf::getPower()
{
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath("/api/v1/"+m_authToken+"/state/on");

    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = m_networkManager->get(request);
    qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            emit connectionChanged(false);
            return;
        }
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcNanoleaf()) << "Recieved invalide JSON object";
            return;
        }
        bool power = data.toVariant().toMap().value("value").toBool();
        emit connectionChanged(true);
        emit powerReceived(power);
    });
}

void Nanoleaf::getHue()
{
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath("/api/v1/"+m_authToken+"/state/hue");

    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = m_networkManager->get(request);
    qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcNanoleaf()) << "Recieved invalide JSON object";
            return;
        }
        int hue = data.toVariant().toMap().value("value").toBool();
        emit connectionChanged(true);
        emit hueReceived(hue);
    });
}

void Nanoleaf::getBrightness()
{
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath("/api/v1/"+m_authToken+"/state/brightness");

    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcNanoleaf()) << "Recieved invalide JSON object";
            return;
        }
        int brightness = data.toVariant().toMap().value("value").toInt();
        emit connectionChanged(true);
        emit brightnessReceived(brightness);
    });
}

void Nanoleaf::getSaturation()
{
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath("/api/v1/"+m_authToken+"/state/sat");

    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcNanoleaf()) << "Recieved invalide JSON object";
            return;
        }
        int brightness = data.toVariant().toMap().value("value").toInt();
        emit connectionChanged(true);
        emit saturationReceived(brightness);
    });
}

void Nanoleaf::getColorTemperature()
{
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath("/api/v1/"+m_authToken+"/state/ct");

    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            emit connectionChanged(false);
            return;
        }
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcNanoleaf()) << "Recieved invalide JSON object";
            return;
        }
        int mired = data.toVariant().toMap().value("value").toInt();
        emit connectionChanged(true);
        emit colorTemperatureReceived(mired);
    });
}

void Nanoleaf::getColorMode()
{
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath("/api/v1/"+m_authToken+"/state/colorMode");

    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            emit connectionChanged(false);
            return;
        }
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcNanoleaf()) << "Recieved invalide JSON object";
            return;
        }
        QString colorMode = data.toVariant().toMap().value("value").toString();
        emit connectionChanged(true);
        emit colorModeReceived(colorMode);
    });
}

QUuid Nanoleaf::setPower(bool power)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath(QString("/api/v1/%1/state").arg(m_authToken));

    QVariantMap map;
    QVariantMap value;
    value["value"] = power;
    map.insert("on", value);
    QJsonDocument body = QJsonDocument::fromVariant(map);

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_networkManager->put(request, body.toJson());
    qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 204 || reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId, false);
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit requestExecuted(requestId, true);
    });
    return requestId;
}

QUuid Nanoleaf::setHue(QColor color)
{
    Q_UNUSED(color);
    QUuid requestId = QUuid::createUuid();
    return requestId;
}

QUuid Nanoleaf::setBrightness(int percentage)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath(QString("/api/v1/%1/state").arg(m_authToken));

    QVariantMap map;
    QVariantMap value;
    value["value"] = percentage;
    map.insert("brightness", value);
    QJsonDocument body = QJsonDocument::fromVariant(map);

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_networkManager->put(request, body.toJson());
    qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 204 || reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId, false);
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit requestExecuted(requestId, true);
    });
    return requestId;
}

QUuid Nanoleaf::setSaturation(int percentage)
{
    Q_UNUSED(percentage);
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath(QString("/api/v1/%1/state/sat").arg(m_authToken));

    QVariantMap map;
    QVariantMap value;
    value["value"] = percentage;
    map.insert("sat", value);
    QJsonDocument body = QJsonDocument::fromVariant(map);

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_networkManager->put(request, body.toJson());
    qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 204 || reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId, false);
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit requestExecuted(requestId, true);
    });
    return requestId;
}

QUuid Nanoleaf::setColorTemperature(int mired)
{
    Q_UNUSED(mired);
    QUuid requestId = QUuid::createUuid();
    return requestId;
}

void Nanoleaf::getEffects()
{
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath("/api/v1/"+m_authToken+"/effects/effectsList");

    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            emit connectionChanged(false);
            return;
        }
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcNanoleaf()) << "Recieved invalide JSON object";
            return;
        }
        QStringList effects;
        foreach (QVariant effect, data.toVariant().toList()) {
            effects.append(effect.toString());
        }

        emit connectionChanged(true);
        emit effectListReceived(effects);
    });
}

void Nanoleaf::getSelectedEffect()
{
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath("/api/v1/"+m_authToken+"/effects/select");

    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            emit connectionChanged(false);
            return;
        }
        QString effect = reply->readAll();
        emit connectionChanged(true);
        emit selectedEffectReceived(effect);
    });
}

QUuid Nanoleaf::setEffect(const QString &effect)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath(QString("/api/v1/%1/effects").arg(m_authToken));

    QVariantMap map;
    map.insert("select", effect);
    QJsonDocument body = QJsonDocument::fromVariant(map);

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_networkManager->put(request, body.toJson());
    qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status != 204 || reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId, false);
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit requestExecuted(requestId, true);
    });
    return requestId;
}



