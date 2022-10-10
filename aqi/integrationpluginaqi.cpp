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
#include "nymeasettings.h"

#include <QNetworkAccessManager>

// The API reports American AQI index values, but nymea interfaces require actual values
QList<QPair<int,double>> pm10AQI = {
    {55,54},
    {100,154},
    {150,254},
    {200,354},
    {300,424},
    {400,504},
    {500,604}
};

QList<QPair<int,double>> pm25AQI = {
    {50,12},
    {100,35.4},
    {150,55.4},
    {200,150.4},
    {300,250.4},
    {400,350.4},
    {500,500}
};

QList<QPair<int,double>> so2AQI = {
    {50,35},
    {100,75},
    {150,185},
    {200,304},
    {300,604},
    {400,804},
    {500,1004}
};

QList<QPair<int,double>> no2AQI = {
    {50,53},
    {100,100},
    {150,360},
    {200,649},
    {300,1294},
    {400,1649},
    {500,2049}
};

QList<QPair<int,double>> o3AQI = {
    {50,54},
    {100,70},
    {150,85},
    {200,105},
    {300,200},
    {400,504},
    {500,604}
};

QList<QPair<int,double>> coAQI = {
    {50,4.4},
    {100,9.4},
    {150,12.4},
    {200,15.4},
    {300,30.4},
    {400,40.4},
    {500,50.4}
};

IntegrationPluginAqi::IntegrationPluginAqi()
{
    connect(this, &IntegrationPluginAqi::configValueChanged, this, [this] (const ParamTypeId &paramTypeId, const QVariant &value) {

        if (paramTypeId == airQualityIndexPluginApiKeyParamTypeId && m_aqiConnection) {
            if (!value.toString().isEmpty()) {
                qCDebug(dcAirQualityIndex()) << "Custom API key updated";
                m_aqiConnection->setApiKey(value.toString());
            } else {
                qCDebug(dcAirQualityIndex()) << "Custom API key has been deleted";
                QString apiKey = apiKeyStorage()->requestKey("aqi").data("apiKey");
                if (apiKey.isEmpty()) {
                    qCWarning(dcApiKeys()) << "No API Key is available, keeping the AQI connection as it is";
                } else {
                    m_aqiConnection->setApiKey(apiKey);
                }
            }
        }
    });
}

void IntegrationPluginAqi::discoverThings(ThingDiscoveryInfo *info)
{
    if (!m_aqiConnection) {
        if(!createAqiConnection()) {
            return info->finish(Thing::ThingErrorHardwareNotAvailable,  QT_TR_NOOP("API key is not available."));
        }
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
    connect(info, &ThingDiscoveryInfo::aborted, [=] {m_asyncDiscovery.remove(requestId);});
}

void IntegrationPluginAqi::setupThing(ThingSetupInfo *info)
{
    if (info->thing()->thingClassId() == airQualityIndexThingClassId) {
        if (!m_aqiConnection) {
            if(!createAqiConnection()) {
                return info->finish(Thing::ThingErrorHardwareNotAvailable,  QT_TR_NOOP("API key is not available."));
            }
            double longitude = info->thing()->paramValue(airQualityIndexThingLongitudeParamTypeId).toDouble();
            double latitude = info->thing()->paramValue(airQualityIndexThingLatitudeParamTypeId).toDouble();
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
            // An AQI connection might be setup because of an discovery request
            // or because there is already another thing using the connection
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

        if (!m_aqiConnection) {
            qCWarning(dcAirQualityIndex()) << "Air quality connection not initialized";
            return;
        }

        double longitude = thing->paramValue(airQualityIndexThingLongitudeParamTypeId).toDouble();
        double latitude = thing->paramValue(airQualityIndexThingLatitudeParamTypeId).toDouble();
        QUuid requestId = m_aqiConnection->getDataByGeolocation(latitude, longitude);
        m_asyncRequests.insert(requestId, thing->id());
    }

    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginAqi::onPluginTimer);
    }
}

bool IntegrationPluginAqi::createAqiConnection()
{
    QString apiKey = configValue(airQualityIndexPluginApiKeyParamTypeId).toString();

    if (apiKey.isEmpty()) {
        apiKey = apiKeyStorage()->requestKey("aqi").data("apiKey");
    }
    if (apiKey.isEmpty()) {
        qCWarning(dcAirQualityIndex()) << "Could not find any API key for AQI";
        return false;
    }
    m_aqiConnection = new AirQualityIndex(hardwareManager()->networkManager(), apiKey, this);
    connect(m_aqiConnection, &AirQualityIndex::requestExecuted, this, &IntegrationPluginAqi::onRequestExecuted);
    connect(m_aqiConnection, &AirQualityIndex::dataReceived, this, &IntegrationPluginAqi::onAirQualityDataReceived);
    connect(m_aqiConnection, &AirQualityIndex::stationsReceived, this, &IntegrationPluginAqi::onAirQualityStationsReceived);
    return true;
}

double IntegrationPluginAqi::convertFromAQI(int aqi, const QList<QPair<int, double>> &map) const
{
    int il = 0, ih = 0;
    double vl = 0, vh = 0;

    int index = 0;
    while (aqi > map.at(index).first && index < map.count()) {
        index++;
    }

    if (index > 0) {
        il = map.at(index - 1).first;
        vl = map.at(index - 1).second;
    }
    ih = map.at(index).first;
    vh = map.at(index).second;
    double value = (aqi - il) * (vh - vl) / (ih - il) + vl;
    return value;
}

void IntegrationPluginAqi::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)
    if (myThings().empty()) {
        if (m_pluginTimer) {
            hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
            m_pluginTimer = nullptr;        }
        if (m_aqiConnection) {
            m_aqiConnection->deleteLater();
            m_aqiConnection = nullptr;
        }
    }
}

void IntegrationPluginAqi::onAirQualityDataReceived(QUuid requestId, AirQualityIndex::AirQualityData data)
{
    qCDebug(dcAirQualityIndex()) << "Air Quality data received, request id:" << requestId << "is an async request:" << m_asyncRequests.contains(requestId);

    if (m_asyncSetups.contains(requestId)) {
        ThingSetupInfo *info = m_asyncSetups.value(requestId);
        return info->finish(Thing::ThingErrorNoError);
    }

    if (m_asyncRequests.contains(requestId)) {
        Thing * thing = myThings().findById(m_asyncRequests.take(requestId));
        if (!thing)
            return;


        thing->setStateValue(airQualityIndexConnectedStateTypeId, true);
        thing->setStateValue(airQualityIndexHumidityStateTypeId, data.humidity);
        thing->setStateValue(airQualityIndexTemperatureStateTypeId, data.temperature);
        thing->setStateValue(airQualityIndexPressureStateTypeId, data.pressure);
        thing->setStateValue(airQualityIndexCoStateTypeId, convertFromAQI(data.co, coAQI));
        thing->setStateValue(airQualityIndexO3StateTypeId, convertFromAQI(data.o3, o3AQI));
        thing->setStateValue(airQualityIndexNo2StateTypeId, convertFromAQI(data.no2, no2AQI));
        thing->setStateValue(airQualityIndexSo2StateTypeId, convertFromAQI(data.so2, so2AQI));
        thing->setStateValue(airQualityIndexPm10StateTypeId, convertFromAQI(data.pm10, pm10AQI));
        thing->setStateValue(airQualityIndexPm25StateTypeId, convertFromAQI(data.pm25, pm25AQI));
        thing->setStateValue(airQualityIndexWindSpeedStateTypeId, data.windSpeed);
    }
}

void IntegrationPluginAqi::onAirQualityStationsReceived(QUuid requestId, QList<AirQualityIndex::Station> stations)
{
    qCDebug(dcAirQualityIndex()) << "Air Quality Stations received, request id:" << requestId << "is an async request:" << m_asyncRequests.contains(requestId);
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
        Thing * thing = myThings().findById(m_asyncRequests.value(requestId));
        if (!thing) {
            qCWarning(dcAirQualityIndex()) << "Can't find thing, associated to this async request";
            return;
        }
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
        double longitude = thing->paramValue(airQualityIndexThingLongitudeParamTypeId).toDouble();
        double latitude = thing->paramValue(airQualityIndexThingLatitudeParamTypeId).toDouble();
        QUuid requestId = m_aqiConnection->getDataByGeolocation(latitude, longitude);
        m_asyncRequests.insert(requestId, thing->id());
    }
}

void IntegrationPluginAqi::onRequestExecuted(QUuid requestId, bool success)
{
    qCDebug(dcAirQualityIndex()) << "Request executed, requestId:" << requestId << "Success:" << success << "is an async request:" << m_asyncRequests.contains(requestId);
    if (m_asyncDiscovery.contains(requestId) && !success) {
        ThingDiscoveryInfo *info = m_asyncDiscovery.take(requestId);
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Air quality index server not available, please check your internet connection."));
        if (myThings().filterByThingClassId(airQualityIndexThingClassId).isEmpty() && m_aqiConnection) {
            m_aqiConnection->deleteLater();
            m_aqiConnection = nullptr;
        }
    }

    if (m_asyncRequests.contains(requestId)) {

        Thing *thing = myThings().findById(m_asyncRequests.value(requestId));
        thing->setStateValue(airQualityIndexConnectedStateTypeId, success);
        if (!success) {
            qCWarning(dcAirQualityIndex()) << "Request failed, removing request from async request list";
        }
        m_asyncRequests.remove(requestId);
    }
}
