/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2024 devendragajjar <devendragajjar@gmail.com>                 *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation;                  *
 *  version 3 of the License.                                              *
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

#include "v2xe_amber_electric.h"
#include "plugininfo.h"
#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QSslConfiguration>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>




const QString const_site = "01J1XBQFGX57137EH0C6AG040D";
const QString no_of_data_need = "5";
const QString site_http_link = "https://api.amber.com.au/v1/sites";
const QString price_http_link_front  = "https://api.amber.com.au/v1/sites/";
const QString price_http_link_back =  "/prices/current?next="+ no_of_data_need +"&previous=" + no_of_data_need;
        //01J1XBQFGX57137EH0C6AG040D/prices/current?next=5&previous=5";
const QString auth_token = "f5b2c823de5fed2f4c6bdb7c1b0db4f5";
const QString renewble_http_link  = "https://api.amber.com.au/v1/state/vic/renewables/current?next=48&previous=5&resolution=5";


v2xe_amber_electric::v2xe_amber_electric()
{
    m_connectedStateTypeIds[V2Xe_Amber_ElectricThingClassId] = V2Xe_Amber_ElectricConnectedStateTypeId;
    m_forecastPriceStateTypeIds[V2Xe_Amber_ElectricThingClassId] = V2Xe_Amber_ElectricForecastpriceStateTypeId;
    m_currentPriceStateTypeIds[V2Xe_Amber_ElectricThingClassId] = V2Xe_Amber_ElectricCurrentpriceStateTypeId;
    m_currentSiteTypeIds[V2Xe_Amber_ElectricThingClassId] = V2Xe_Amber_ElectricSitesStateTypeId;
    m_currentfeddinStateTypeIds[V2Xe_Amber_ElectricThingClassId] = V2Xe_Amber_ElectricCurrentfeedinStateTypeId;
    m_fururefeedinStateTypeIds[V2Xe_Amber_ElectricThingClassId] = V2Xe_Amber_ElectricFuturefeedinStateTypeId;
    m_consumerKey = auth_token;
}

v2xe_amber_electric::~v2xe_amber_electric()
{

}

void v2xe_amber_electric::init()
{
    // Initialisation can be done here.
    qCDebug(dcAmber_Electric_plgn()) << "Plugin initialized.";
}



void v2xe_amber_electric::setupThing(ThingSetupInfo *info)
{
    // A thing is being set up. Use info->thing() to get details of the thing, do
    // the required setup (e.g. connect to the device) and call info->finish() when done.

    qCDebug(dcAmber_Electric_plgn()) << "Setup thing" << info->thing()->name() << info->thing()->params();
    QString key = info->thing()->paramValue(V2Xe_Amber_ElectricThingAuth_tokenParamTypeId).toString();
    if (!key.isEmpty()) {
        m_consumerKey = key;
        qCCritical(dcAmber_Electric_plgn()) << "m_consumerKey set " << m_consumerKey << "key " << key;
    }

    QString site = info->thing()->paramValue(V2Xe_Amber_ElectricThingCrnt_siteParamTypeId).toString();
    if (!site.isEmpty()) {
        m_current_site = site;
        qCCritical(dcAmber_Electric_plgn()) << "m_current_site set " << m_current_site ;
    }
    else{
        m_current_site = const_site;
    }

    // use defult key for now
    qCCritical(dcAmber_Electric_plgn()) << "No API key set.";
    qCCritical(dcAmber_Electric_plgn()) << "Either install an API key pacakge (nymea-apikeysprovider-plugin-*) or provide a key in the plugin settings.";

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);//TODO make 30 minutes after test
        connect(m_pluginTimer, &PluginTimer::timeout, this, &v2xe_amber_electric::onPluginTimer);
    }
    info->finish(Thing::ThingErrorNoError);
}

void v2xe_amber_electric::onPluginTimer()
{
    foreach (Thing *thing, myThings()) {

        //requestPriceData(thing); // TODO
#if ENABLE_RENEWEBLE_API
        requestRenewablesData(thing); //TODO
#endif
        requestSiteData(thing);
        requestSitePriceData(thing);

    }
}

void v2xe_amber_electric::executeAction(ThingActionInfo *info)
{
    // An action is being executed. Use info->action() to get details about the action,
    // do the required operations (e.g. send a command to the network) and call info->finish() when done.

    qCDebug(dcAmber_Electric_plgn()) << "Executing action for thing" << info->thing() << info->action().actionTypeId().toString() << info->action().params();

    info->finish(Thing::ThingErrorNoError);
}

void v2xe_amber_electric::thingRemoved(Thing *thing)
{
    // A thing is being removed from the system. Do the required cleanup
    // (e.g. disconnect from the device) here.

    qCDebug(dcAmber_Electric_plgn()) << "Remove thing" << thing;
}

void v2xe_amber_electric::requestSitePriceData(Thing* thing, ThingSetupInfo* setup) {

    QString price_http_link = price_http_link_front + m_current_site + price_http_link_back;

    m_serverUrls[V2Xe_Amber_ElectricThingClassId] = price_http_link;

    QNetworkRequest request;
    request.setUrl(QUrl(m_serverUrls[V2Xe_Amber_ElectricThingClassId]));

    // Add the Authorization header with the token
    QString token = thing->paramValue(m_consumerKey).toString();
    token = m_consumerKey;
    qCCritical(dcAmber_Electric_plgn) << " using m_consumerKey " <<m_consumerKey;
    request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());
    request.setRawHeader("accept", "application/json");

    QNetworkReply* reply =hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, thing, setup, reply]() {
        reply->deleteLater();

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
            qCWarning(dcAmber_Electric_plgn) << "Update reply HTTP error:" << status << reply->errorString();
            if (setup) {
                setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error getting data from server."));
            } else {
                //thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), false);//TODO remove comment
            }
            return;
        }
        onSitePriceDataReceived(thing, setup, reply);
    });
}

void v2xe_amber_electric::onSitePriceDataReceived(Thing* thing, ThingSetupInfo* setup, QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(dcAmber_Electric_plgn) << "Network error:" << reply->errorString();
        if (setup) {
            setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Network error."));
        }
        return;
    }

    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcAmber_Electric_plgn) << "JSON parse error:" << error.errorString();
        if (setup) {
            setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("JSON parse error."));
        }
        return;
    }

    if (jsonDoc.isArray()) {
        QString channelType;
        QJsonArray jsonArray = jsonDoc.array();
        for (const QJsonValue &value : jsonArray) {
            if (value.isObject()) {
                QJsonObject jsonObject = value.toObject();
                QString type = jsonObject["type"].toString();  //Get type
                channelType = jsonObject["channelType"].toString();

                       //Current Price
                       if((type == "CurrentInterval") && channelType == "general"){

                         if (jsonObject.contains("perKwh") && jsonObject["perKwh"].isDouble()) {  // Get perKwh

                             double perkwh = jsonObject["perKwh"].toDouble();

                             thing->setStateValue(m_currentPriceStateTypeIds.value(thing->thingClassId()), qAbs(perkwh));
                         }
                        }

                       //Future Price
                       if((type == "ForecastInterval") && channelType == "general"){

                         if (jsonObject.contains("perKwh") && jsonObject["perKwh"].isDouble()) {  // Get perKwh

                             double perkwh = jsonObject["perKwh"].toDouble();

                             thing->setStateValue(m_forecastPriceStateTypeIds.value(thing->thingClassId()), qAbs(perkwh));
                           }
                        }


                       // Current Feedin //TODO what is feed in price its dummy data
                       if((type == "CurrentInterval") && (channelType == "feedIn")){

                         if (jsonObject.contains("perKwh") && jsonObject["perKwh"].isDouble()) {  // Get SportPerKwh
                            double spotperkwh = jsonObject["perKwh"].toDouble();

                            thing->setStateValue(m_currentfeddinStateTypeIds.value(thing->thingClassId()), qAbs(spotperkwh));
                           }
                       }

                       //Future Feedin
                       if((type == "ForecastInterval") && (channelType == "feedIn")){

                         if (jsonObject.contains("perKwh") && jsonObject["perKwh"].isDouble()) {  // Get SportPerKwh
                            double spotperkwh = jsonObject["perKwh"].toDouble();

                            thing->setStateValue(m_fururefeedinStateTypeIds.value(thing->thingClassId()), qAbs(spotperkwh));
                           }
                       }

            }
        }
    }

    if (setup) {
        setup->finish(Thing::ThingErrorNoError);
    }
    thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), true);
}


void v2xe_amber_electric::requestSiteData(Thing* thing, ThingSetupInfo* setup) {

    m_serverUrls[V2Xe_Amber_ElectricThingClassId] = site_http_link;


    QNetworkRequest request;
    request.setUrl(QUrl(m_serverUrls[V2Xe_Amber_ElectricThingClassId]));

    // Add the Authorization header with the token
    QString token = thing->paramValue(m_consumerKey).toString();
    token = m_consumerKey;
    qCCritical(dcAmber_Electric_plgn) << " using m_consumerKey " <<m_consumerKey;
    request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());
    request.setRawHeader("accept", "application/json");

    QNetworkReply* reply =hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, thing, setup, reply]() {
        reply->deleteLater();

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
            qCWarning(dcAmber_Electric_plgn) << "Update reply HTTP error:" << status << reply->errorString();
            if (setup) {
                setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error getting data from server."));
            } else {
                thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), false);
            }
            return;
        }

        onSiteDataReceived(thing, setup, reply);
    });
}

void v2xe_amber_electric::onSiteDataReceived(Thing* thing, ThingSetupInfo* setup, QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(dcAmber_Electric_plgn) << "Network error:" << reply->errorString();
        if (setup) {
            setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Network error."));
        }
        return;
    }
    QVariantList stringList = {};
    QString l_site;
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcAmber_Electric_plgn) << "JSON parse error:" << error.errorString();
        if (setup) {
            setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("JSON parse error."));
        }
        return;
    }
    qCCritical(dcAmber_Electric_plgn) << "onSiteDataReceived " ;
    if (jsonDoc.isArray()) {
        QJsonArray jsonArray = jsonDoc.array();
        for (const QJsonValue &value : jsonArray) {
            if (value.isObject()) {
                QJsonObject jsonObject = value.toObject();
                QString type = jsonObject["id"].toString();  //Get type

                    if(!type.isEmpty()){ //update
                         stringList.append(type);
                         qCCritical(dcAmber_Electric_plgn) << "type :" << type;
                         l_site = type;
                    }
            }
        }
    }
    thing->setStateValue(m_currentSiteTypeIds.value(thing->thingClassId()), l_site);
    if (setup) {
        setup->finish(Thing::ThingErrorNoError);
    }
    thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), true);
}



#if ENABLE_RENEWEBLE_API
void v2xe_amber_electric::requestRenewablesData(Thing* thing, ThingSetupInfo* setup) {

    m_serverUrls[V2Xe_Amber_ElectricThingClassId] = renewble_http_link;

    QNetworkRequest request;
    request.setUrl(QUrl(m_serverUrls[V2Xe_Amber_ElectricThingClassId]));

    // Add the Authorization header with the token
    QString token = thing->paramValue(m_consumerKey).toString();
    request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());
    request.setRawHeader("accept", "application/json");

    QNetworkReply* reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, thing, setup, reply]() {
        reply->deleteLater();

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
            qCWarning(dcAmber_Electric_plgn) << "Update reply HTTP error:" << status << reply->errorString();
            if (setup) {
                setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error getting data from server."));
            } else {
                //thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), false);//TODO remove comment
            }
            return;
        }
        onRenewablesDataReceived(thing, setup, reply);
    });
}


void v2xe_amber_electric::onRenewablesDataReceived(Thing* thing, ThingSetupInfo* setup, QNetworkReply* reply) {

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(dcAmber_Electric_plgn) << "Network error:" << reply->errorString();
        if (setup) {
            setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Network error."));
        }
        return;
    }

    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcAmber_Electric_plgn) << "JSON parse error:" << error.errorString();
        if (setup) {
            setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("JSON parse error."));
        }
        return;
    }

    if (jsonDoc.isArray()) {
        QJsonArray jsonArray = jsonDoc.array();
        for (const QJsonValue &value : jsonArray) {
            if (value.isObject()) {
                QJsonObject jsonObject = value.toObject();
                QString type = jsonObject["type"].toString();
                if (type == "ActualRenewable") {
                    //double renewables = jsonObject["renewables"].toDouble();
                }
            }
        }
    }

    if (setup) {
        setup->finish(Thing::ThingErrorNoError);
    }
    thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), true);
}

#endif
