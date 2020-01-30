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
    //qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
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
    //qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
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
    //qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            emit authenticationStatusChanged(false);
            return;
        }
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcNanoleaf()) << "Recieved invalide JSON object";
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        QVariantMap map = data.toVariant().toMap();
        ControllerInfo info;
        info.name = map.value("name").toString();
        info.serialNumber = map.value("serialNo").toString();
        info.model = map.value("model").toString();
        info.manufacturer = map.value("manufacturer").toString();
        info.firmwareVersion = map.value("firmwareVersion").toString();
        emit controllerInfoReceived(info);

        if (map.contains("state")) {
            QVariantMap state = map.value("state").toMap();
            if (state.contains("on")) {
                emit powerReceived(state["on"].toMap().value("value").toBool());
            }
            if (state.contains("hue") &&  state.contains("sat") && state.contains("brightness")) {
                int brightness = state["brightness"].toMap().value("value").toInt();
                emit brightnessReceived(brightness);
                int hue = state["hue"].toMap().value("value").toInt();
                emit hueReceived(hue);
                int sat = state["sat"].toMap().value("value").toInt();
                emit saturationReceived(sat);
                QColor color;
                color.setHsv(hue, sat, brightness);
                emit colorReceived(color);
            }
            if (state.contains("ct")) {
                emit colorTemperatureReceived(state["ct"].toMap().value("value").toInt());
            }
            if (state.contains("colorMode")) {
                QString colorModeString = state["colorMode"].toString();
                if (colorModeString == "effect") {
                    emit colorModeReceived(ColorMode::EffectMode);
                } else if (colorModeString == "hs") {
                    emit colorModeReceived(ColorMode::HueSaturationMode);
                } else if (colorModeString == "ct") {
                    emit colorModeReceived(ColorMode::ColorTemperatureMode);
                } else {
                    qCWarning(dcNanoleaf()) << "Unrecognized color mode";
                }
            }
        }
        if (map.contains("effects")) {
            QVariantMap effects = map.value("effects").toMap();
            emit selectedEffectReceived(effects.value("select").toString());
        }

        if (map.contains("panelLayout")) {
            //QVariantMap panelLayout = map.value("panelLayout").toMap();
            //emit panelLayoutReceived();
        }

        if (map.contains("rhythm")) {
            //QVariantMap rhythm = map.value("rhythm").toMap();
            //emit rhythmModulReceived(rhythm.value("select").toString());
        }
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
    //qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
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
    //qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
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

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
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

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
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

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
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
        int kelvin = data.toVariant().toMap().value("value").toInt();
        emit connectionChanged(true);
        emit colorTemperatureReceived(kelvin);
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

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
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
        emit connectionChanged(true);
        QString colorModeString = data.toVariant().toMap().value("value").toString();
        if (colorModeString == "effect") {
            emit colorModeReceived(ColorMode::EffectMode);
        } else if (colorModeString == "hs") {
            emit colorModeReceived(ColorMode::HueSaturationMode);
        } else if (colorModeString == "ct") {
            emit colorModeReceived(ColorMode::ColorTemperatureMode);
        } else {
            qCWarning(dcNanoleaf()) << "Unrecognized color mode";
        }
    });
}

void Nanoleaf::registerForEvents()
{
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath("/api/v1/"+m_authToken+"/events");
    QUrlQuery query;
    query.addQueryItem("id", "1,2,3,4");
    url.setQuery(query);
    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::readyRead, this, [reply, this] {
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcNanoleaf()) << "Recieved invalide JSON object";
            return;
        }
        qCDebug(dcNanoleaf()) << "On event stream" << data.toJson();
    });
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
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
        qCDebug(dcNanoleaf()) << "Event received" << data.toJson();
        QVariantList events = data.toVariant().toList();

        foreach (QVariant variant, events) {
            QVariantMap event = variant.toMap();
            switch (event["attr"].toInt()) {
            case 1:  //ON
                emit powerReceived(event["value"].toBool());
                break;
            case 2:  //Brightness
                emit brightnessReceived(event["value"].toInt());
                break;
            case 3: //Hue
                emit hueReceived(event["value"].toInt());
                break;
            case 4: //Saturation
                emit saturationReceived(event["value"].toInt());
                break;
            case 5: //Color Temperature
                emit colorTemperatureReceived(event["value"].toInt());
                break;
            case 6: { //colorMode
                QString colorModeString = event["value"].toString();
                if (colorModeString == "effect") {
                    emit colorModeReceived(ColorMode::EffectMode);
                } else if (colorModeString == "hs") {
                    emit colorModeReceived(ColorMode::HueSaturationMode);
                } else if (colorModeString == "ct") {
                    emit colorModeReceived(ColorMode::ColorTemperatureMode);
                } else {
                    qCWarning(dcNanoleaf()) << "Unrecognized color mode";
                }
                break;
            }
            default:
                qCWarning(dcNanoleaf()) << "Unrecognised Event received";
            }

        }
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
    //qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId, false);
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit requestExecuted(requestId, true);
    });
    return requestId;
}

QUuid Nanoleaf::setColor(QColor color)
{
    QUuid requestId = setHue(color.hue());
    setSaturation(static_cast<int>(color.saturation()/2.55)); //QColor saturation is 0-255
    return requestId;
}

QUuid Nanoleaf::setHue(int hue)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath(QString("/api/v1/%1/state").arg(m_authToken));

    QVariantMap map;
    QVariantMap hueMap;
    hueMap["value"] = hue;
    map.insert("hue", hueMap);
    QJsonDocument body = QJsonDocument::fromVariant(map);

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_networkManager->put(request, body.toJson());
    //qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId, false);
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit requestExecuted(requestId, true);
    });
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
    //qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
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
    //qDebug(dcNanoleaf()) << "Sending request" << request.url() << body.toJson();
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId, false);
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit requestExecuted(requestId, true);
    });
    return requestId;
}

QUuid Nanoleaf::setMired(int mired)
{
    //NOTE: this is just a rough conversion between mired and kelvin
    int kelvin = static_cast<int>((653-mired) * 13);
    QUuid requestId = setKelvin(kelvin);
    return requestId;
}

QUuid Nanoleaf::setKelvin(int kelvin)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath(QString("/api/v1/%1/state").arg(m_authToken));

    QVariantMap map;
    QVariantMap value;
    value["value"] = kelvin;
    map.insert("ct", value);
    QJsonDocument body = QJsonDocument::fromVariant(map);

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_networkManager->put(request, body.toJson());
    qDebug(dcNanoleaf()) << "Sending request" << request.url() << body.toJson();
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId, false);
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit requestExecuted(requestId, true);
    });
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

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
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

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
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

        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId, false);
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit requestExecuted(requestId, true);
    });
    return requestId;
}

QUuid Nanoleaf::identify()
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_address.toString());
    url.setPort(m_port);
    url.setScheme("http");
    url.setPath(QString("/api/v1/%1/identify").arg(m_authToken));

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_networkManager->put(request, "");
    qDebug(dcNanoleaf()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status < 200 || status > 204 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status >= 400 && status <= 410) {
                emit authenticationStatusChanged(false);
            }
            emit requestExecuted(requestId, false);
            qCWarning(dcNanoleaf()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit requestExecuted(requestId, true);
    });
    return requestId;
}



