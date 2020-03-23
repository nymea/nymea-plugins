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

#include "integrationpluginaqi.h"
#include "plugininfo.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

IntegrationPluginAqi::IntegrationPluginAqi()
{

}

void IntegrationPluginAqi::setupThing(ThingSetupInfo *info)
{
    if (info->thing()->thingClassId() == airQualityIndexThingClassId) {

        if (!myThings().filterByThingClassId(info->thing()->thingClassId()).isEmpty()) {
            if (!myThings().findById(info->thing()->id())) {
                info->finish(Thing::ThingErrorSetupFailed, tr("Service is already in use."));
                return;
            }
        }

        QUrl url;
        url.setUrl(m_baseUrl);
        url.setPath("/feed/here/");
        url.setQuery("token="+configValue(airQualityIndexPluginApiKeyParamTypeId).toString());
        QNetworkRequest request;
        request.setUrl(url);
        request.setRawHeader("User-Agent", "nymea 1.0");

        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, this, [reply, info, this] {
            reply->deleteLater();
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            // Check HTTP status code
            if (status != 200 || reply->error() != QNetworkReply::NoError) {
                if (status == 400 || status == 401) {

                }
                qCWarning(dcAirQualityIndex()) << "Request error:" << status << reply->errorString();
                return info->finish(Thing::ThingErrorSetupFailed, reply->errorString());
            }
            return info->finish(Thing::ThingErrorNoError);
        });
    }
}


void IntegrationPluginAqi::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing);
    getDataByIp();

    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginAqi::onPluginTimer);
    }
}


void IntegrationPluginAqi::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing);

    if (myThings().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}


void IntegrationPluginAqi::getDataByIp()
{
    QUrl url;
    url.setUrl(m_baseUrl);
    url.setPath("/feed/here/");
    url.setQuery("token="+configValue(airQualityIndexPluginApiKeyParamTypeId).toString());
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "nymea");

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            foreach (Thing *thing, myThings().filterByThingClassId(airQualityIndexThingClassId)) {
                thing->setStateValue(airQualityIndexConnectedStateTypeId, true);
            }
            qCWarning(dcAirQualityIndex()) << "Request error:" << status << reply->errorString();
            return;
        }
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcAirQualityIndex()) << "Received invalide JSON object";
            return;
        }
        QVariantMap city = data.toVariant().toMap().value("data").toMap().value("city").toMap();
        //double lat = city["geo"].toList().takeAt(0).toDouble();
        //double lng = city["geo"].toList().takeAt(1).toDouble();
        QString name = city["name"].toString();
        QString url = city["url"].toString();

        QVariantMap iaqi = data.toVariant().toMap().value("data").toMap().value("iaqi").toMap();
        double humidity = iaqi["h"].toMap().value("v").toDouble();
        double pressure = iaqi["p"].toMap().value("v").toDouble();
        int pm25 = iaqi["pm25"].toMap().value("v").toInt();
        int pm10 = iaqi["pm10"].toMap().value("v").toInt();
        double so2 = iaqi["so2"].toMap().value("v").toDouble();
        double no2 = iaqi["no2"].toMap().value("v").toDouble();
        double o3 = iaqi["o3"].toMap().value("v").toDouble();
        double co = iaqi["co"].toMap().value("v").toDouble();
        double temperature = iaqi["t"].toMap().value("v").toDouble();
        double windSpeed = iaqi["w"].toMap().value("v").toDouble();

        foreach (Thing *thing, myThings().filterByThingClassId(airQualityIndexThingClassId)) {
            thing->setStateValue(airQualityIndexStationNameStateTypeId, name);
            thing->setStateValue(airQualityIndexConnectedStateTypeId, true);
            thing->setStateValue(airQualityIndexCoStateTypeId, co);
            thing->setStateValue(airQualityIndexHumidityStateTypeId, humidity);
            thing->setStateValue(airQualityIndexTemperatureStateTypeId, temperature);
            thing->setStateValue(airQualityIndexPressureStateTypeId, pressure);
            thing->setStateValue(airQualityIndexO3StateTypeId, o3);
            thing->setStateValue(airQualityIndexNo2StateTypeId, no2);
            thing->setStateValue(airQualityIndexSo2StateTypeId, so2);
            thing->setStateValue(airQualityIndexPm10StateTypeId, pm10);
            thing->setStateValue(airQualityIndexPm25StateTypeId, pm25);
            thing->setStateValue(airQualityIndexWindSpeedStateTypeId, windSpeed);

            if (pm25 <= 50.00) {
                thing->setStateValue(airQualityIndexAirQualityStateTypeId, "Good");
                thing->setStateValue(airQualityIndexCautionaryStatementStateTypeId, tr("None"));
            } else if ((pm25 > 50.00) && (pm25 <= 100.00)) {
                thing->setStateValue(airQualityIndexAirQualityStateTypeId, "Moderate");
                thing->setStateValue(airQualityIndexCautionaryStatementStateTypeId, tr("Active children and adults, and people with respiratory disease, such as asthma, should limit prolonged outdoor exertion."));
            } else if ((pm25 > 100.00) && (pm25 <= 150.00)) {
                thing->setStateValue(airQualityIndexAirQualityStateTypeId, "Unhealthy for Sensitive Groups");
                thing->setStateValue(airQualityIndexCautionaryStatementStateTypeId, tr("Active children and adults, and people with respiratory disease, such as asthma, should limit prolonged outdoor exertion."));
            }  else if ((pm25 > 150.00) && (pm25 <= 200.00)) {
                thing->setStateValue(airQualityIndexAirQualityStateTypeId, "Unhealthy");
                thing->setStateValue(airQualityIndexCautionaryStatementStateTypeId, tr("Active children and adults, and people with respiratory disease, such as asthma, should avoid prolonged outdoor exertion; everyone else, especially children, should limit prolonged outdoor exertion"));
            } else if ((pm25 > 200.00) && (pm25 <= 300.00)) {
                thing->setStateValue(airQualityIndexAirQualityStateTypeId, "Very Unhealthy");
                thing->setStateValue(airQualityIndexCautionaryStatementStateTypeId, tr("Active children and adults, and people with respiratory disease, such as asthma, should avoid all outdoor exertion; everyone else, especially children, should limit outdoor exertion."));
            } else {
                thing->setStateValue(airQualityIndexAirQualityStateTypeId, "Hazardous");
                thing->setStateValue(airQualityIndexCautionaryStatementStateTypeId, tr("Everyone should avoid all outdoor exertion"));
            }
        }
    });
}

void IntegrationPluginAqi::onPluginTimer()
{
    getDataByIp();
}



