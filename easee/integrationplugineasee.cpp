/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Tim Stöcker <t.stoecker@consolinno.de>                 *
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

#include "plugininfo.h"
#include "integrationplugineasee.h"


#include <network/networkaccessmanager.h>
#include <plugintimer.h>
#include <QtNetwork>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QTimer>
#include <QNetworkAccessManager>

IntegrationPluginEasee::IntegrationPluginEasee()
{

}

void IntegrationPluginEasee::init()
{
    // Initialisation can be done here.
    qCDebug(dcEasee()) << "Plugin initialized.";
}

void IntegrationPluginEasee::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter your login credentials for the Easee Wallbox."));
}

void IntegrationPluginEasee::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    QString chargerId = info->params().paramValue(wallboxThingChargerIdParamTypeId).toString();
    QNetworkRequest header = composeApiKeyRequest();
    QJsonObject param;
    param.insert("username", QJsonValue::fromVariant(username));
    param.insert("password", QJsonValue::fromVariant(secret));
    pluginStorage()->beginGroup(chargerId);
    pluginStorage()->setValue("username", username);
    pluginStorage()->setValue("password", secret);
    pluginStorage()->endGroup();
    QNetworkReply *reply = hardwareManager()->networkManager()->post(header,QJsonDocument(param).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, info, [=](){
        info->finish(Thing::ThingErrorNoError);
        QByteArray response_data = reply->readAll();
        QJsonDocument json = QJsonDocument::fromJson(response_data);
        accessKey = json.object().value("accessToken").toString();

        if (QString::compare (accessKey,"") != 0){
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorAuthenticationFailure);
        }
        reply->deleteLater();
    });



}


void IntegrationPluginEasee::setupThing(ThingSetupInfo *info)
{
    // A thing is being set up. Use info->thing() to get details of the thing, do
    // the required setup (e.g. connect to the device) and call info->finish() when done.

    qCDebug(dcEasee()) << "Setup thing" << info->thing();
    if (QString::compare (accessKey,"") != 0){
        info->thing()->setStateValue(wallboxConnectedStateTypeId, true);

    }
    else {
        refreshAccessToken(info->thing());
    }
    info->finish(Thing::ThingErrorNoError);


}
void IntegrationPluginEasee::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing)

    if (!m_timer) {
        m_timer = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_timer, &PluginTimer::timeout, this, [this](){
            foreach (Thing *thing, myThings()) {

                if (QString::compare (accessKey,"") != 0 && (siteId == 0 || circuitId == 0 ) ){
                    getSiteAndCircuit(thing);
                } else if (QString::compare (accessKey,"") == 0) {
                    refreshAccessToken(thing);
                }
                else {
                    qCDebug(dcEasee()) << "Credentials Retrieved from the Cloud" <<siteId;
                    getCurrentPower(thing);
                    writeCurrentLimit(thing);
                }
            }
        });
    }

    if (!access_timer){
        access_timer = hardwareManager()->pluginTimerManager()->registerTimer(8600);
        connect(access_timer, &PluginTimer::timeout, this, [this](){
            foreach (Thing *thing, myThings()) {
                refreshAccessToken(thing);
            }
        });
    }
}

void IntegrationPluginEasee::refreshAccessToken(Thing *thing)
{
    QString chargerId = thing->paramValue(wallboxThingChargerIdParamTypeId).toString();
    pluginStorage()->beginGroup(chargerId);
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();
    QNetworkRequest header = composeApiKeyRequest();
    QJsonObject param;
    param.insert("username", QJsonValue::fromVariant(username));
    param.insert("password", QJsonValue::fromVariant(password));
    QNetworkReply *reply = hardwareManager()->networkManager()->post(header,QJsonDocument(param).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, thing, [=](){

        QByteArray response_data = reply->readAll();
        QJsonDocument json = QJsonDocument::fromJson(response_data);
        accessKey = json.object().value("accessToken").toString();
        if (QString::compare (accessKey,"") != 0){
            thing->setStateValue(wallboxConnectedStateTypeId, true);

        }
        else {
            thing->setStateValue(wallboxConnectedStateTypeId, false);
        }
        reply->deleteLater();
    });


}
void IntegrationPluginEasee::getSiteAndCircuit(Thing *thing){
    QString chargerId = thing->paramValue(wallboxThingChargerIdParamTypeId).toString();
    QNetworkRequest header = composeSiteAndCircuitRequest(chargerId);
    QNetworkReply *reply = hardwareManager()->networkManager()->get(header);
    connect(reply, &QNetworkReply::finished, thing, [=](){
        QByteArray response_data = reply->readAll();
        QJsonDocument json = QJsonDocument::fromJson(response_data);
        QJsonArray subJson = json.object().value("circuits").toArray();
        QJsonDocument siteJson = QJsonDocument::fromVariant(subJson.at(0).toVariant());
        double site = siteJson.object().value("siteId").toDouble();
        double circuit =siteJson.object().value("id").toDouble();
        siteId = site;
        circuitId = circuit;

    });
    reply->deleteLater();

}

void IntegrationPluginEasee::getCurrentPower(Thing *thing){
    QString chargerId = thing->paramValue(wallboxThingChargerIdParamTypeId).toString();
    QNetworkRequest header = composeCurrentPowerRequest(chargerId);
    QNetworkReply *reply = hardwareManager()->networkManager()->get(header);
    connect(reply, &QNetworkReply::finished, thing, [=](){
        QByteArray response_data = reply->readAll();
        QJsonDocument json = QJsonDocument::fromJson(response_data);
        double totalPower = json.object().value("totalPower").toDouble();
        double phaseMode = json.object().value("outputPhase").toDouble();
        if (phaseMode > 10) {thing->setStateValue(wallboxPhaseCountStateTypeId,3);}
        else {thing->setStateValue(wallboxPhaseCountStateTypeId,1);}
        thing->setStateValue(wallboxCurrentPowerStateTypeId,totalPower);
        qCDebug(dcEasee()) << json.object().value("dynamicChargerCurrent").toDouble();
        qCDebug(dcEasee()) << phaseMode;
        if (totalPower > 10){
            thing->setStateValue(wallboxPluggedInStateTypeId,true);
            thing->setStateValue(wallboxChargingStateTypeId,true);

        } else {
            thing->setStateValue(wallboxPluggedInStateTypeId,false);
            thing->setStateValue(wallboxChargingStateTypeId,false);
        }
        if (reply->error() != QNetworkReply::NetworkError::NoError){
            thing->setStateValue(wallboxConnectedStateTypeId, false);
        }
        reply->deleteLater();
    });

}

void IntegrationPluginEasee::writeCurrentLimit(Thing *thing)
{
    QNetworkRequest header = composeCurrentLimitRequest();
    int limit = thing->state("maxChargingCurrent").value().toInt();
    QJsonObject param;
    param.insert("phase1", QJsonValue::fromVariant(limit));
    param.insert("phase2", QJsonValue::fromVariant(limit));
    param.insert("phase3", QJsonValue::fromVariant(limit));
    param.insert("timeToLive", QJsonValue::fromVariant(1));
    QNetworkReply *reply = hardwareManager()->networkManager()->post(header,QJsonDocument(param).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, thing, [=](){
        if (reply->error() != QNetworkReply::NetworkError::NoError){
            thing->setStateValue(wallboxConnectedStateTypeId, false);
        }
        reply->deleteLater();
    });


}

void IntegrationPluginEasee::executeAction(ThingActionInfo *info)
{
    // An action is being executed. Use info->action() to get details about the action,
    // do the required operations (e.g. send a command to the network) and call info->finish() when done.

    qCDebug(dcEasee()) << "Executing action for thing" << info->thing() << info->action().actionTypeId().toString() << info->action().params();

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginEasee::thingRemoved(Thing *thing)
{
    // A thing is being removed from the system. Do the required cleanup
    // (e.g. disconnect from the device) here.
    delete m_timer;
    delete access_timer;
    qCDebug(dcEasee()) << "Remove thing" << thing;
}
QNetworkRequest IntegrationPluginEasee::composeApiKeyRequest()
{
    QUrl url("https://api.easee.cloud/api/accounts/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/*+json");
    return request;
}

QNetworkRequest IntegrationPluginEasee::composeSiteAndCircuitRequest(const QString &chargerId)
{
    QUrl url("https://api.easee.cloud/api/chargers/"+ chargerId +"/site");
    QNetworkRequest request(url);
    QString headerData = "Bearer " + accessKey;
    request.setRawHeader("Authorization", headerData.toLocal8Bit());
    return request;
}

QNetworkRequest IntegrationPluginEasee::composeCurrentPowerRequest(const QString &chargerId)
{
    QUrl url("https://api.easee.cloud/api/chargers/"+ chargerId +"/state");
    QNetworkRequest request(url);
    QString headerData = "Bearer " + accessKey;
    request.setRawHeader("Authorization", headerData.toLocal8Bit());
    return request;
}
QNetworkRequest IntegrationPluginEasee::composeCurrentLimitRequest()
{
    QUrl url("https://api.easee.cloud/api/sites/"+ QString::number((int)siteId,10) +"/circuits/" +  QString::number((int)circuitId,10) +"/dynamicCurrent");
    QNetworkRequest request(url);
    qCDebug(dcEasee()) << url;
    QString headerData = "Bearer " + accessKey;
    request.setRawHeader("Authorization", headerData.toLocal8Bit());
    return request;
}
