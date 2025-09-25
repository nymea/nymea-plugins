/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#include "integrationpluginopenuv.h"
#include "plugininfo.h"

#include <integrations/thing.h>
#include <network/networkaccessmanager.h>

#include <QDebug>
#include <QJsonDocument>
#include <QVariantMap>
#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>
#include <QTimeZone>

IntegrationPluginOpenUv::IntegrationPluginOpenUv()
{
}

void IntegrationPluginOpenUv::init()
{
    m_apiKey = configValue(openUvPluginApiKeyParamTypeId).toByteArray();

    connect(this, &IntegrationPluginOpenUv::configValueChanged, this, [this]{
        m_apiKey = configValue(openUvPluginApiKeyParamTypeId).toByteArray();
    });
}


void IntegrationPluginOpenUv::discoverThings(ThingDiscoveryInfo *info)
{
    QNetworkRequest request(QUrl("http://ip-api.com/json"));
    QNetworkReply* reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, reply, info]() {
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcOpenUv()) << "Error fetching geolocation from ip-api:" << reply->error() << reply->errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Failed to fetch data from the internet."));
            return;
        }
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcOpenUv()) << "Failed to parse json from ip-api:" << error.error << error.errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The server returned unexpected data."));
            return;
        }
        if (!jsonDoc.toVariant().toMap().contains("lat") || !jsonDoc.toVariant().toMap().contains("lon")) {
            qCWarning(dcOpenUv()) << "Reply missing geolocation info" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The server returned unexpected data."));
            return;
        }
        qreal lat = jsonDoc.toVariant().toMap().value("lat").toDouble();
        qreal lon = jsonDoc.toVariant().toMap().value("lon").toDouble();

        ThingDescriptor descriptor(info->thingClassId(), tr("OpenUV location"), jsonDoc.toVariant().toMap().value("city").toString());
        ParamList params;
        params.append(Param(openUvThingLatitudeParamTypeId, lat));
        params.append(Param(openUvThingLongitudeParamTypeId, lon));
        descriptor.setParams(params);
        info->addThingDescriptor(descriptor);
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginOpenUv::setupThing(ThingSetupInfo *info)
{
    if (m_apiKey.isEmpty() || m_apiKey == "-") {
        info->finish(Thing::ThingErrorSetupFailed, tr("API key is missing, add your OpenUV API key in the plug-in configs"));
        return;
    }

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60 * 60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginOpenUv::onPluginTimer);
    }
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginOpenUv::postSetupThing(Thing *thing)
{
    getUvIndex(thing);
}


void IntegrationPluginOpenUv::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing);

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginOpenUv::getUvIndex(Thing *thing)
{
    qCDebug(dcOpenUv()) << "Refresh data for" << thing->name();
    QUrl url("https://api.openuv.io/api/v1/uv");
    QUrlQuery query;
    query.addQueryItem("lat", thing->paramValue(openUvThingLatitudeParamTypeId).toString());
    query.addQueryItem("lng", thing->paramValue(openUvThingLongitudeParamTypeId).toString());
    url.setQuery(query);
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("x-access-token", m_apiKey);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, thing] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                thing->setStateValue(openUvConnectedStateTypeId, false);
            }
            qCWarning(dcOpenUv()) << "Request error:" << status << reply->errorString();
            return;
        }
        thing->setStateValue(openUvConnectedStateTypeId, true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcOpenUv()) << "Recieved invalide JSON object";
            return;
        }
        QVariantMap result = data.toVariant().toMap().value("result").toMap();
        thing->setStateValue(openUvUvStateTypeId, result["uv"].toDouble());
        thing->setStateValue(openUvUvMaxStateTypeId, result["uv_max"].toDouble());
        thing->setStateValue(openUvOzoneStateTypeId, result["ozone"].toDouble());

        thing->setStateValue(openUvUvTimeStateTypeId, QDateTime::fromString(result["uv_time"].toString(),Qt::DateFormat::ISODate).toMSecsSinceEpoch() / 1000);
        thing->setStateValue(openUvOzoneTimeStateTypeId, QDateTime::fromString(result["ozone_time"].toString(),Qt::DateFormat::ISODate).toMSecsSinceEpoch() / 1000);
        thing->setStateValue(openUvUvMaxTimeStateTypeId, QDateTime::fromString(result["uv_max_time"].toString(),Qt::DateFormat::ISODate).toMSecsSinceEpoch() / 100);

        QVariantMap safeExposureTimes = result["safe_exposure_time"].toMap();
        thing->setStateValue(openUvSafeExposureTimeSt1StateTypeId, safeExposureTimes["st1"].toInt());
        thing->setStateValue(openUvSafeExposureTimeSt2StateTypeId, safeExposureTimes["st2"].toInt());
        thing->setStateValue(openUvSafeExposureTimeSt3StateTypeId, safeExposureTimes["st3"].toInt());
        thing->setStateValue(openUvSafeExposureTimeSt4StateTypeId, safeExposureTimes["st4"].toInt());
        thing->setStateValue(openUvSafeExposureTimeSt5StateTypeId, safeExposureTimes["st5"].toInt());
        thing->setStateValue(openUvSafeExposureTimeSt6StateTypeId, safeExposureTimes["st6"].toInt());
    });
}

void IntegrationPluginOpenUv::onPluginTimer()
{
    foreach (Thing *thing, myThings()) {
        getUvIndex(thing);
    }
}

