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

IntegrationPluginAqi::IntegrationPluginAqi()
{

}

void IntegrationPluginAqi::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter your API token for Air Quality Index"));
}

void IntegrationPluginAqi::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)

    QNetworkRequest request(QUrl("https://api.waqi.info/feed/here/?token="+secret));
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, info, [this, reply, info, secret](){
        reply->deleteLater();

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // check HTTP status code
        if (status != 200) {
            //: Error setting up device with invalid token
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("This token is not valid."));
            return;
        }

        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("apiKey", secret);
        pluginStorage()->endGroup();
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginAqi::discoverThings(ThingDiscoveryInfo *info)
{
    if (!m_aqiConnection) {
        QString apiKey = "74d31bb5ad9bcdeaed48097418b55188cb56d450"; //temporary key for discovery
        m_aqiConnection = new AirQualityIndex(hardwareManager()->networkManager(), apiKey, this);
        connect(m_aqiConnection, &AirQualityIndex::requestExecuted, this, &IntegrationPluginAqi::onRequestExecuted);
        connect(m_aqiConnection, &AirQualityIndex::dataReceived, this, &IntegrationPluginAqi::onAirQualityDataReceived);
        connect(m_aqiConnection, &AirQualityIndex::stationsReceived, this, &IntegrationPluginAqi::onAirQualityStationsReceived);

        connect(info, &ThingDiscoveryInfo::aborted, [this] {
            if (myThings().filterByThingClassId(airQualityIndexThingClassId).isEmpty()) {
                m_aqiConnection->deleteLater();
                m_aqiConnection = nullptr;
            }
        });
    } else {
        qCDebug(dcAirQualityIndex()) << "AQI connection alread created";
    }
    QUuid requestId = m_aqiConnection->getDataByIp();
    m_asyncDiscovery.insert(requestId, info);
}

void IntegrationPluginAqi::setupThing(ThingSetupInfo *info)
{
    if (info->thing()->thingClassId() == airQualityIndexThingClassId) {
        if (!m_aqiConnection) {
            pluginStorage()->beginGroup(info->thing()->id().toString());
            QString apiKey = pluginStorage()->value("apiKey").toString();
            pluginStorage()->endGroup();
            m_aqiConnection = new AirQualityIndex(hardwareManager()->networkManager(), apiKey, this);
            connect(m_aqiConnection, &AirQualityIndex::requestExecuted, this, &IntegrationPluginAqi::onRequestExecuted);
            connect(m_aqiConnection, &AirQualityIndex::dataReceived, this, &IntegrationPluginAqi::onAirQualityDataReceived);
            connect(m_aqiConnection, &AirQualityIndex::stationsReceived, this, &IntegrationPluginAqi::onAirQualityStationsReceived);

            QString longitude = info->thing()->paramValue(airQualityIndexThingLongitudeParamTypeId).toString();
            QString latitude = info->thing()->paramValue(airQualityIndexThingLatitudeParamTypeId).toString();
            QUuid requestId = m_aqiConnection->getDataByGeolocation(latitude, longitude);
            m_asyncSetups.insert(requestId, info);

            connect(info, &ThingSetupInfo::aborted, [requestId, this] {
                m_asyncSetups.remove(requestId);
                if (myThings().filterByThingClassId(airQualityIndexThingClassId).isEmpty()) {
                    m_aqiConnection->deleteLater();
                    m_aqiConnection = nullptr;
                }
            });
        } else {
            info->finish(Thing::ThingErrorNoError);
        }
    } else {
        qCWarning(dcAirQualityIndex()) << "setupThing - thing class id not found" << info->thing()->thingClassId();
        info->finish(Thing::ThingErrorSetupFailed);
    }
}



void IntegrationPluginAqi::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == airQualityIndexThingClassId) {

        if (!m_aqiConnection)
            return;

        QString longitude = thing->paramValue(airQualityIndexThingLongitudeParamTypeId).toString();
        QString latitude = thing->paramValue(airQualityIndexThingLatitudeParamTypeId).toString();
        QUuid requestId = m_aqiConnection->getDataByGeolocation(latitude, longitude);
        m_asyncRequests.insert(requestId, thing->id());
    }

    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginAqi::onPluginTimer);
    }
}


void IntegrationPluginAqi::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == airQualityIndexThingClassId) {

    }

    if (myThings().empty()) {
        if (!m_pluginTimer) {
            hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
            m_pluginTimer = nullptr;
        }
        if (!m_aqiConnection) {
            m_aqiConnection->deleteLater();
            m_aqiConnection = nullptr;
        }
    }
}


void IntegrationPluginAqi::onAirQualityDataReceived(QUuid requestId, AirQualityIndex::AirQualityData data)
{
    if (m_asyncSetups.contains(requestId)) {
        ThingSetupInfo *info = m_asyncSetups.take(requestId);
        return info->finish(Thing::ThingErrorNoError);
    }

    if (m_asyncRequests.contains(requestId)) {
        Thing * thing = myThings().findById(m_asyncRequests.take(requestId));
        if (!thing)
            return;

        thing->setStateValue(airQualityIndexConnectedStateTypeId, true);
        thing->setStateValue(airQualityIndexCoStateTypeId, data.co);
        thing->setStateValue(airQualityIndexHumidityStateTypeId, data.humidity);
        thing->setStateValue(airQualityIndexTemperatureStateTypeId, data.temperature);
        thing->setStateValue(airQualityIndexPressureStateTypeId, data.pressure);
        thing->setStateValue(airQualityIndexO3StateTypeId, data.o3);
        thing->setStateValue(airQualityIndexNo2StateTypeId, data.no2);
        thing->setStateValue(airQualityIndexSo2StateTypeId, data.so2);
        thing->setStateValue(airQualityIndexPm10StateTypeId, data.pm10);
        thing->setStateValue(airQualityIndexPm25StateTypeId, data.pm25);
        thing->setStateValue(airQualityIndexWindSpeedStateTypeId, data.windSpeed);

        if (data.pm25 <= 50.00) {
            thing->setStateValue(airQualityIndexAirQualityStateTypeId, "Good");
            thing->setStateValue(airQualityIndexCautionaryStatementStateTypeId, tr("None"));
        } else if ((data.pm25 > 50.00) && (data.pm25 <= 100.00)) {
            thing->setStateValue(airQualityIndexAirQualityStateTypeId, "Moderate");
            thing->setStateValue(airQualityIndexCautionaryStatementStateTypeId, tr("Active children and adults, and people with respiratory disease, such as asthma, should limit prolonged outdoor exertion."));
        } else if ((data.pm25 > 100.00) && (data.pm25 <= 150.00)) {
            thing->setStateValue(airQualityIndexAirQualityStateTypeId, "Unhealthy for Sensitive Groups");
            thing->setStateValue(airQualityIndexCautionaryStatementStateTypeId, tr("Active children and adults, and people with respiratory disease, such as asthma, should limit prolonged outdoor exertion."));
        }  else if ((data.pm25 > 150.00) && (data.pm25 <= 200.00)) {
            thing->setStateValue(airQualityIndexAirQualityStateTypeId, "Unhealthy");
            thing->setStateValue(airQualityIndexCautionaryStatementStateTypeId, tr("Active children and adults, and people with respiratory disease, such as asthma, should avoid prolonged outdoor exertion; everyone else, especially children, should limit prolonged outdoor exertion"));
        } else if ((data.pm25 > 200.00) && (data.pm25 <= 300.00)) {
            thing->setStateValue(airQualityIndexAirQualityStateTypeId, "Very Unhealthy");
            thing->setStateValue(airQualityIndexCautionaryStatementStateTypeId, tr("Active children and adults, and people with respiratory disease, such as asthma, should avoid all outdoor exertion; everyone else, especially children, should limit outdoor exertion."));
        } else {
            thing->setStateValue(airQualityIndexAirQualityStateTypeId, "Hazardous");
            thing->setStateValue(airQualityIndexCautionaryStatementStateTypeId, tr("Everyone should avoid all outdoor exertion"));
        }
    }
}

void IntegrationPluginAqi::onAirQualityStationsReceived(QUuid requestId, QList<AirQualityIndex::Station> stations)
{
    if (m_asyncDiscovery.contains(requestId)) {
        ThingDiscoveryInfo *info = m_asyncDiscovery.take(requestId);
        foreach(AirQualityIndex::Station station, stations) {
            ThingDescriptor descriptor(airQualityIndexThingClassId, station.name, "Air Quality Index Station");
            ParamList params;
            params << Param(airQualityIndexThingLatitudeParamTypeId, station.location.latitude);
            params << Param(airQualityIndexThingLongitudeParamTypeId, station.location.longitude);
            descriptor.setParams(params);
            info->addThingDescriptor(descriptor);
        }
        info->finish(Thing::ThingErrorNoError);
    }


    if (m_asyncRequests.contains(requestId)) {
        Thing * thing = myThings().findById(m_asyncRequests.take(requestId));
        if (!thing)
            return;
        if (stations.length() != 0) {
            thing->setStateValue(airQualityIndexStationNameStateTypeId, stations.first().name);
        }
    }
}

void IntegrationPluginAqi::onPluginTimer()
{
    if (!m_aqiConnection)
        return;

    foreach (Thing *thing, myThings().filterByThingClassId(airQualityIndexThingClassId)) {

        QString longitude = thing->paramValue(airQualityIndexThingLongitudeParamTypeId).toString();
        QString latitude = thing->paramValue(airQualityIndexThingLatitudeParamTypeId).toString();
        QUuid requestId = m_aqiConnection->getDataByGeolocation(latitude, longitude);
        m_asyncRequests.insert(requestId, thing->id());
    }
}

void IntegrationPluginAqi::onRequestExecuted(QUuid requestId, bool success)
{
    if (m_asyncRequests.contains(requestId)) {

        Thing *thing = myThings().findById(m_asyncRequests.value(requestId));
        thing->setStateValue(airQualityIndexConnectedStateTypeId, success);
        if (!success)
            m_asyncRequests.remove(requestId);
    }
}
