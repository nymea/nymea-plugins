/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2025 devendragajjar <devendragajjar@gmail.com>                 *
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

#include "v2xeamberelectric.h"
#include "plugininfo.h"
#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QSslConfiguration>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>




const QString constSite = "01J1XBQFGX57137EH0C6AG040D";
const QString noOfDataNeed = "5";
const QString siteHttpLink = "https://api.amber.com.au/v1/sites";
const QString priceHttpLinkFront  = "https://api.amber.com.au/v1/sites/";
const QString priceHttpLinkBack =  "/prices/current?next="+ noOfDataNeed +"&previous=" + noOfDataNeed;
const QString authToken = "f5b2c823de5fed2f4c6bdb7c1b0db4f5";


V2xeAmberElectric::V2xeAmberElectric()
{
    mConsumerKey = authToken;
}

V2xeAmberElectric::~V2xeAmberElectric()
{

}

void V2xeAmberElectric::init()
{
    // Initialisation can be done here.
    qCDebug(dcAmberElectric()) << "Plugin initialized.";
}



void V2xeAmberElectric::setupThing(ThingSetupInfo *info)
{
    // A thing is being set up. Use info->thing() to get details of the thing, do
    // the required setup (e.g. connect to the device) and call info->finish() when done.

    qCDebug(dcAmberElectric()) << "Setup thing" << info->thing()->name() << info->thing()->params();
    QString key = info->thing()->paramValue(V2XeAmberElectricThingAuthTokenParamTypeId).toString();
    if (!key.isEmpty()) {
        mConsumerKey = key;
        qCDebug(dcAmberElectric()) << "mConsumerKey set " << mConsumerKey << "key " << key;
    }

    QString site = info->thing()->paramValue(V2XeAmberElectricThingCurrentSiteParamTypeId).toString();
    if (!site.isEmpty()) {
        mCurrentSite = site;
        qCDebug(dcAmberElectric()) << "mCurrentSite set " << mCurrentSite ;
    }
    else{
        mCurrentSite = constSite;
    }

    // use default key for now
    qCDebug(dcAmberElectric()) << "No API key set.";
    qCDebug(dcAmberElectric()) << "Either install an API key pacakge (nymea-apikeysprovider-plugin-*) or provide a key in the plugin settings.";

    if (!mPluginTimer) {
        mPluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(1800);//TODO make 30 minutes after test for 
        connect(mPluginTimer, &PluginTimer::timeout, this, &V2xeAmberElectric::onPluginTimer);
    }
    info->finish(Thing::ThingErrorNoError);
}

void V2xeAmberElectric::onPluginTimer()
{
    foreach (Thing *thing, myThings()) {

        requestSiteData(thing);
        requestSitePriceData(thing);

    }
}

void V2xeAmberElectric::executeAction(ThingActionInfo *info)
{
    // An action is being executed. Use info->action() to get details about the action,
    // do the required operations (e.g. send a command to the network) and call info->finish() when done.

    qCDebug(dcAmberElectric()) << "Executing action for thing" << info->thing() << info->action().actionTypeId().toString() << info->action().params();

    info->finish(Thing::ThingErrorNoError);
}

void V2xeAmberElectric::thingRemoved(Thing *thing)
{
    // A thing is being removed from the system. Do the required cleanup
    // (e.g. disconnect from the device) here.

    qCDebug(dcAmberElectric()) << "Remove thing" << thing;
}

void V2xeAmberElectric::requestSitePriceData(Thing* thing, ThingSetupInfo* setup) {

    QString price_http_link = priceHttpLinkFront + mCurrentSite + priceHttpLinkBack;

    mServerUrls[V2XeAmberElectricThingClassId] = price_http_link;

    QNetworkRequest request;
    request.setUrl(QUrl(mServerUrls[V2XeAmberElectricThingClassId]));

    // Add the Authorization header with the token
    QString token = thing->paramValue(mConsumerKey).toString();
    token = mConsumerKey;
    qCDebug(dcAmberElectric) << " using mConsumerKey " <<mConsumerKey;
    request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());
    request.setRawHeader("accept", "application/json");

    QNetworkReply* reply =hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, thing, setup, reply]() {
        reply->deleteLater();

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
            qCWarning(dcAmberElectric) << "Update reply HTTP error:" << status << reply->errorString();
            if (setup) {
                setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error getting data from server."));
            } else {
                thing->setStateValue(V2XeAmberElectricConnectedStateTypeId, false);
            }
            return;
        }
        onSitePriceDataReceived(thing, setup, reply);
    });
}

void V2xeAmberElectric::onSitePriceDataReceived(Thing* thing, ThingSetupInfo* setup, QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(dcAmberElectric) << "Network error:" << reply->errorString();
        if (setup) {
            setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Network error."));
        }
        return;
    }

    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcAmberElectric) << "JSON parse error:" << error.errorString();
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
                        thing->setStateValue(V2XeAmberElectricCurrentpriceStateTypeId, qAbs(perkwh));
                    }
                }
                //Future Price
                if((type == "ForecastInterval") && channelType == "general"){
                    if (jsonObject.contains("perKwh") && jsonObject["perKwh"].isDouble()) {  // Get perKwh
                        double perkwh = jsonObject["perKwh"].toDouble();
                        thing->setStateValue(V2XeAmberElectricForecastpriceStateTypeId, qAbs(perkwh));
                    }
                }
                // Current Feedin //TODO what is feed in price its dummy data
                if((type == "CurrentInterval") && (channelType == "feedIn")){
                    if (jsonObject.contains("perKwh") && jsonObject["perKwh"].isDouble()) {  // Get SportPerKwh
                        double spotperkwh = jsonObject["perKwh"].toDouble();
                        thing->setStateValue(V2XeAmberElectricCurrentfeedinStateTypeId, qAbs(spotperkwh));
                    }
                }
                //Future Feedin
                if((type == "ForecastInterval") && (channelType == "feedIn")){
                    if (jsonObject.contains("perKwh") && jsonObject["perKwh"].isDouble()) {  // Get SportPerKwh
                        double spotperkwh = jsonObject["perKwh"].toDouble();
                        thing->setStateValue(V2XeAmberElectricFuturefeedinStateTypeId, qAbs(spotperkwh));
                    }
                }
            }
        }
    }

    if (setup) {
        setup->finish(Thing::ThingErrorNoError);
    }
    thing->setStateValue(V2XeAmberElectricConnectedStateTypeId, true);
}


void V2xeAmberElectric::requestSiteData(Thing* thing, ThingSetupInfo* setup) {

    mServerUrls[V2XeAmberElectricThingClassId] = siteHttpLink;


    QNetworkRequest request;
    request.setUrl(QUrl(mServerUrls[V2XeAmberElectricThingClassId]));

    // Add the Authorization header with the token
    QString token = thing->paramValue(mConsumerKey).toString();
    token = mConsumerKey;
    qCDebug(dcAmberElectric) << " using mConsumerKey " <<mConsumerKey;
    request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());
    request.setRawHeader("accept", "application/json");

    QNetworkReply* reply =hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, thing, setup, reply]() {
        reply->deleteLater();

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
            qCWarning(dcAmberElectric) << "Update reply HTTP error:" << status << reply->errorString();
            if (setup) {
                setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error getting data from server."));
            } else {
                thing->setStateValue(V2XeAmberElectricConnectedStateTypeId, false);
            }
            return;
        }

        onSiteDataReceived(thing, setup, reply);
    });
}

void V2xeAmberElectric::onSiteDataReceived(Thing* thing, ThingSetupInfo* setup, QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(dcAmberElectric) << "Network error:" << reply->errorString();
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
        qCWarning(dcAmberElectric) << "JSON parse error:" << error.errorString();
        if (setup) {
            setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("JSON parse error."));
        }
        return;
    }
    qCDebug(dcAmberElectric) << "onSiteDataReceived " ;
    if (jsonDoc.isArray()) {
        QJsonArray jsonArray = jsonDoc.array();
        for (const QJsonValue &value : jsonArray) {
            if (value.isObject()) {
                QJsonObject jsonObject = value.toObject();
                QString type = jsonObject["id"].toString();  //Get type

                    if(!type.isEmpty()){ //update
                         stringList.append(type);
                         qCDebug(dcAmberElectric) << "type :" << type;
                         l_site = type;
                    }
            }
        }
    }
    thing->setStateValue(V2XeAmberElectricSitesStateTypeId, l_site);
    if (setup) {
        setup->finish(Thing::ThingErrorNoError);
    }
    thing->setStateValue(V2XeAmberElectricConnectedStateTypeId, true);
}

