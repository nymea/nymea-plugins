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

#include "lifxcloud.h"
#include "extern-plugininfo.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

LifxCloud::LifxCloud(NetworkAccessManager *networkManager, QObject *parent) :
    QObject(parent),
    m_networkManager(networkManager)
{

}

void LifxCloud::setAuthorizationToken(const QByteArray &token)
{
    m_authorizationToken = token;
}

bool LifxCloud::cloudAuthenticated()
{
    return m_authenticated;
}

bool LifxCloud::cloudConnected()
{
    return m_connected;
}

void LifxCloud::listLights()
{
    if (m_authorizationToken.isEmpty()) {
        qCWarning(dcLifx()) << "Authorization token is not set";
        return;
    }
    QNetworkRequest request;
    request.setUrl(QUrl("https://api.lifx.com/v1/lights/all"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization","Bearer "+m_authorizationToken);

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [reply,  this] {
        if(!checkHttpStatusCode(reply)) {
            return;
        }
        QByteArray rawData = reply->readAll();

        QJsonDocument data; QJsonParseError error;
        data = QJsonDocument::fromJson(rawData, &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcLifx()) << "List lights: Received invalide JSON object" << error.errorString();
            return;
        }

        if (!data.isArray())
            qCWarning(dcLifx()) << "Data is not an array";

        QJsonArray array = data.array();
        QList<Light> descriptors;
        foreach (QJsonValue jsonValue, array) {

            QJsonObject object = jsonValue.toObject();
            qCDebug(dcLifx()) << "Light object:" << object;
            Light light;
            light.id = object["id"].toString().toUtf8();
            light.uuid = object["uuid"].toString().toUtf8();
            if (object["power"].toString() == "on") {
                light.power = true;
            } else {
                light.power = false;
            }
            light.label = object["label"].toString();
            light.connected = object["connected"].toBool();
            light.brightness = object["brightness"].toDouble();
            int hue = object["hue"].toObject().value("saturation").toDouble();
            int saturation = object["color"].toObject().value("saturation").toDouble();
            light.colorTemperature = object["color"].toObject().value("kelvin").toDouble();
            light.color = QColor::fromHsv(hue, saturation, light.brightness);
            Group group;
            group.name = object["group"].toObject().value("name").toString();
            group.id = object["group"].toObject().value("id").toString().toUtf8();
            light.group = group;
            Location location;
            location.name = object["location"].toObject().value("name").toString();
            location.id = object["location"].toObject().value("id").toString().toUtf8();
            light.location = location;
            Product product;
            QJsonObject productObject = object["product"].toObject();
            product.name = productObject["name"].toString();
            product.identifier = productObject["identifier"].toString();
            product.manufacturer = productObject["manufacturer"].toString();
            product.secondsSinceLastSeen = productObject["seconds_since_seen"].toInt();
            Capabilities capabilities;
            QJsonObject capabilitiesObject = productObject["capabilities"].toObject();
            capabilities.color = capabilitiesObject["has_color"].toBool();
            capabilities.colorTemperature = capabilitiesObject["has_variable_color_temp"].toBool();
            capabilities.ir = capabilitiesObject["has_ir"].toBool();
            capabilities.chain = capabilitiesObject["has_chain"].toBool();
            capabilities.multizone = capabilitiesObject["has_multizone"].toBool();
            capabilities.minKelvin= capabilitiesObject["min_kelvin"].toInt();
            capabilities.maxKelvin = capabilitiesObject["max_kelvin"].toInt();
            product.capabilities = capabilities;
            light.product = product;
            descriptors.append(light);
        }
        emit lightsListReceived(descriptors);
    });
}

void LifxCloud::listScenes()
{
    if (m_authorizationToken.isEmpty()) {
        qCWarning(dcLifx()) << "Authorization token is not set";
        return;
    }
    QNetworkRequest request;
    request.setUrl(QUrl("https://api.lifx.com/v1/scenes"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization","Bearer "+m_authorizationToken);

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [reply,  this] {
        if(!checkHttpStatusCode(reply)) {
            return;
        }
        QByteArray rawData = reply->readAll();
        qCDebug(dcLifx()) << "Got list scenes reply" << rawData;
        QJsonDocument data; QJsonParseError error;
        data = QJsonDocument::fromJson(rawData, &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcLifx()) << "List scenes: Received invalide JSON object" << error.errorString();
            return;
        }
        if (!data.isArray())
            qCWarning(dcLifx()) << "Data is not an array";

        QJsonArray array = data.array();
        QList<Scene> scenes;
        foreach (QJsonValue value, array) {
            Scene scene;
            scene.id = value.toObject().value("uuid").toString().toUtf8();
            scene.name = value.toObject().value("name").toString();
            scenes.append(scene);
        }
        emit scenesListReceived(scenes);
    });
}

int LifxCloud::setPower(const QString &lightId, bool power, int duration)
{
    return setState("id:"+lightId, StatePower, power, duration);
}

int LifxCloud::setBrightnesss(const QString &lightId, int brightness, int duration)
{
    return setState("id:"+lightId, StateBrightness, brightness/100.00, duration);
}

int LifxCloud::setColor(const QString &lightId, QColor color, int duration)
{
    return setState("id:"+lightId, StateColor, color.name(), duration);
}

int LifxCloud::setColorTemperature(const QString &lightId, int kelvin, int duration)
{
    return setState("id:"+lightId, StateColorTemperature, kelvin, duration);
}

int LifxCloud::setInfrared(const QString &lightId, int infrared, int duration)
{
    return setState("id:"+lightId, StateColor, infrared/100.00, duration);
}

int LifxCloud::activateScene(const QString &sceneId)
{
    if (m_authorizationToken.isEmpty()) {
        qCWarning(dcLifx()) << "Authorization token is not set";
        return -1;
    }
    int requestId = qrand();

    QNetworkRequest request;
    request.setUrl(QUrl(QString("https://api.lifx.com/v1/scenes/scene_id:%1/activate").arg(sceneId)));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization","Bearer "+m_authorizationToken);
    QNetworkReply *reply = m_networkManager->put(request, "");
    connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        emit requestExecuted(requestId, checkHttpStatusCode(reply));
        QByteArray rawData = reply->readAll();
        qCDebug(dcLifx()) << "Got activate scene reply" << rawData;
    });
    return requestId;
}

int LifxCloud::setEffect(const QString &lightId, LifxCloud::Effect effect, QColor color)
{
    if (m_authorizationToken.isEmpty()) {
        qCWarning(dcLifx()) << "Authorization token is not set";
        return -1;
    }
    int requestId = qrand();
    QNetworkRequest request;
    QUrlQuery params;
    switch (effect) {
    case LifxCloud::EffectNone:
        request.setUrl(QUrl(QString("https://api.lifx.com/v1/lights/id:%1/effects/off").arg(lightId)));
        break;
    case LifxCloud::EffectBreathe:
        request.setUrl(QUrl(QString("https://api.lifx.com/v1/lights/id:%1/effects/breathe").arg(lightId)));
        params.addQueryItem("color", color.name().trimmed());
        params.addQueryItem("period", "2");
        params.addQueryItem("cycles", "3");
        break;
    case LifxCloud::EffectMove:
        request.setUrl(QUrl(QString("https://api.lifx.com/v1/lights/id:%1/effects/move").arg(lightId)));
        break;
    case LifxCloud::EffectMorph:
        request.setUrl(QUrl(QString("https://api.lifx.com/v1/lights/id:%1/effects/morph").arg(lightId)));
        break;
    case LifxCloud::EffectFlame:
        request.setUrl(QUrl(QString("https://api.lifx.com/v1/lights/id:%1/effects/flame").arg(lightId)));
        break;
    case LifxCloud::EffectPulse:
        request.setUrl(QUrl(QString("https://api.lifx.com/v1/lights/id:%1/effects/pulse").arg(lightId)));
        params.addQueryItem("color", color.name().trimmed());
        params.addQueryItem("period", "2");
        params.addQueryItem("cycles", "3");
        break;
    }
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded.");
    request.setRawHeader("Authorization","Bearer "+m_authorizationToken);
    qCDebug(dcLifx()) << "Set effect request" << request.url() << params.toString().toUtf8();

    QNetworkReply *reply = m_networkManager->post(request, params.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply,  this] {

        QByteArray rawData = reply->readAll();
        qCDebug(dcLifx()) << "Got set effect reply" << rawData;
        emit requestExecuted(requestId, checkHttpStatusCode(reply));
    });
    return requestId;
}

int LifxCloud::setState(const QString &selector, State state, QVariant stateValue, int duration)
{
    if (m_authorizationToken.isEmpty()) {
        qCWarning(dcLifx()) << "Authorization token is not set";
        return -1;
    }
    int requestId = qrand();
    QNetworkRequest request;
    request.setUrl(QUrl(QString("https://api.lifx.com/v1/lights/%1/state").arg(selector)));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization","Bearer "+m_authorizationToken);
    QJsonDocument doc;
    QJsonObject payload;
    payload["duration"] = duration;
    payload["fast"] = false;
    switch (state) {
    case StatePower:
        if (stateValue.toBool())
            payload["power"] = "on";
        else
            payload["power"] = "off";

        qCDebug(dcLifx()) << "Set state power" << stateValue.toBool();
        break;
    case StateBrightness:
        payload["brightness"] = stateValue.toDouble();
        qCDebug(dcLifx()) << "Set state brightness" << stateValue;
        break;
    case StateColor:
        payload["color"] = stateValue.toString();
        qCDebug(dcLifx()) << "Set state color" << stateValue;
        break;
    case StateColorTemperature:
        payload["color"] = "kelvin:"+stateValue.toString();
        qCDebug(dcLifx()) << "Set state color" << stateValue;
        break;
    case StateInfrared:
        payload["infrared"] = stateValue.toDouble();
        qCDebug(dcLifx()) << "Set state infrared" << stateValue;
    }

    doc.setObject(payload);
    qCDebug(dcLifx()) << "Set state request" << request.url() << doc.toJson();
    QNetworkReply *reply = m_networkManager->put(request, doc.toJson());
    connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [requestId, duration,reply,  this] {

        QByteArray rawData = reply->readAll();
        qCDebug(dcLifx()) << "Got set state reply" << rawData;
        if (checkHttpStatusCode(reply)) {
            emit requestExecuted(requestId, true);
            QTimer::singleShot(duration*1000+500, this, [=] {listLights();});
        } else {
            emit requestExecuted(requestId, false);
        }
    });
    return requestId;
}

bool LifxCloud::checkHttpStatusCode(QNetworkReply *reply)
{
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(dcLifx()) << "Request error:" << status << reply->errorString();
        if (m_connected) {
            m_connected = false;
            emit connectionChanged(false);
        }
        return false;
    }
    // check HTTP status code
    if (status == 401 || status == 403) {
        if (m_authenticated) {
            m_authenticated = false;
            emit authenticationChanged(false);
        }
    }
    if (status > 207) {
        qCWarning(dcLifx()) << "Error get scene list" << status;
        return false;
    }
    if (!m_authenticated) {
        m_authenticated = true;
        emit authenticationChanged(true);
    }
    if (!m_connected) {
        m_connected = true;
        emit connectionChanged(true);
    }
    return true;
}
