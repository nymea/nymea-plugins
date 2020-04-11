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

#include "integrationplugindweetio.h"
#include "plugininfo.h"
#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"

#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

IntegrationPluginDweetio::IntegrationPluginDweetio()
{

}

IntegrationPluginDweetio::~IntegrationPluginDweetio()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void IntegrationPluginDweetio::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginDweetio::onPluginTimer);
}

void IntegrationPluginDweetio::setupThing(ThingSetupInfo *info)
{
    if (info->thing()->thingClassId() == postThingClassId) {
        QString thing = info->thing()->paramValue(postThingThingParamTypeId).toString();
        if (thing.isEmpty()){
            qDebug(dcDweetio) << "No thing name given, creating one";
            thing = QUuid::createUuid().toString();
        }
        return info->finish(Thing::ThingErrorNoError);
    } else if (info->thing()->thingClassId() == getThingClassId) {
        return info->finish(Thing::ThingErrorNoError);
    }

    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginDweetio::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == getThingClassId){
        getRequest(thing);
    }
}

void IntegrationPluginDweetio::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == getThingClassId) {
        foreach(QNetworkReply *reply, m_getReplies.keys()){
            if (m_getReplies.value(reply) == thing) {
                m_getReplies.remove(reply);
            }
        }
    }

    if (thing->thingClassId() == postThingClassId) {
        foreach(QNetworkReply *reply, m_postReplies.keys()){
            if (m_postReplies.value(reply) == thing) {
                m_postReplies.remove(reply);
            }
        }
    }
}

void IntegrationPluginDweetio::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();
    qCDebug(dcDweetio) << "Execute action" << thing->id() << action.params();

    if (thing->thingClassId() == postThingClassId) {

        if (action.actionTypeId() == postContentDataActionTypeId) {

            QString content = action.param(postContentDataActionContentDataAreaParamTypeId).value().toString();
            postContent(content, thing, info);
            return;
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }
    return info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginDweetio::postContent(const QString &content, Thing *thing, ThingActionInfo *info)
{
    QUrl url = QString("https://dweet.io:443/dweet/for/") + thing->paramValue(postThingThingParamTypeId).toString();
    QNetworkRequest request(url);
    request.setRawHeader("Content-Type", QString("application/json").toLocal8Bit());
    request.setRawHeader("Accept", QString("application/json").toLocal8Bit());
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());

    QString key = thing->paramValue(postThingKeyParamTypeId).toString();
    if (!(key.isEmpty()) || !(key == "none")) {
        QUrlQuery query;
        query.addQueryItem("key", key);
        url.setQuery(query);
    }

    QVariantMap contentMap;
    contentMap.insert(thing->paramValue(postThingContentNameParamTypeId).toString(), content);

    QByteArray data = QJsonDocument::fromVariant(contentMap).toJson(QJsonDocument::Compact);
    qDebug(dcDweetio) << "Dweet: " << data << "Url: " << url;

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, data);
    m_asyncActions.insert(reply, info);
    // In case the action is cancelled before the reply returns
    connect(info, &ThingActionInfo::destroyed, this, [this, reply](){ m_asyncActions.remove(reply); });
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginDweetio::onNetworkReplyFinished);
    m_postReplies.insert(reply, thing);

}

void IntegrationPluginDweetio::setConnectionStatus(bool status, Thing *thing)
{
    // Set connection status
    thing->setStateValue(postConnectedStateTypeId, status);
}

void IntegrationPluginDweetio::processPostReply(const QVariantMap &data, Thing *thing)
{
    QString message = data.value("this").toString();
    qDebug(dcDweetio) << "Access Reply: " << message << "Device: " << thing->name();
}

void IntegrationPluginDweetio::processGetReply(const QVariantMap &data, Thing *thing)
{
    QVariantList withList = data.value("with").toList();
    QVariantMap contentMap = withList.first().toMap().value("content").toMap();
    QString content = contentMap.value(thing->paramValue(getThingContentNameParamTypeId).toString()).toString();
    thing->setStateValue(getContentStateTypeId, content);

    qDebug(dcDweetio) << "Data: " << data << "Device: " << thing->name();
}

void IntegrationPluginDweetio::onNetworkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (m_postReplies.contains(reply)) {
        Thing *thing = m_postReplies.value(reply);
        m_postReplies.remove(reply);
        ThingActionInfo *info = m_asyncActions.take(reply);

        // check HTTP status code
        if ((status != 200) && (status != 204)) {
            qCWarning(dcDweetio) << "Update reply HTTP error:" << status << reply->errorString() << reply->readAll();
            info->finish(Thing::ThingErrorMissingParameter);
            setConnectionStatus(false, thing);
            reply->deleteLater();
            return;
        }

        info->finish(Thing::ThingErrorNoError);
        setConnectionStatus(true, thing);
        if (status == 204){
            reply->deleteLater();
            return;
        }

        // check JSON file
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcDweetio) << "Update reply JSON error:" << error.errorString();

            reply->deleteLater();
            return;
        }

        processPostReply(jsonDoc.toVariant().toMap(), thing);

    } else if (m_getReplies.contains(reply)) {

        Thing *thing = m_getReplies.value(reply);
        m_getReplies.remove(reply);

        // check HTTP status code
        if ((status != 200) && (status != 204)) {
            qCWarning(dcDweetio) << "Update reply HTTP error:" << status << reply->errorString() << reply->readAll();
            setConnectionStatus(false, thing);
            reply->deleteLater();
            return;
        }

        setConnectionStatus(true, thing);
        if (status == 204){
            reply->deleteLater();
            return;
        }

        // check JSON file
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcDweetio) << "Update reply JSON error:" << error.errorString();

            reply->deleteLater();
            return;
        }

        processGetReply(jsonDoc.toVariant().toMap(), thing);
    }
}

void IntegrationPluginDweetio::getRequest(Thing *thing)
{
    qCDebug(dcDweetio()) << "Refresh data for" << thing->name();

    QUrl url = QString("https://dweet.io:443/get/latest/dweet/for/") + thing->paramValue(postThingThingParamTypeId).toString();
    QNetworkRequest request(url);
    request.setRawHeader("Content-Type", QString("application/json").toLocal8Bit());
    request.setRawHeader("Accept", QString("application/json").toLocal8Bit());
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());

    QString key = thing->paramValue(getThingKeyParamTypeId).toString();
    if (!(key.isEmpty()) || !(key == "none")) {
        QUrlQuery query;
        query.addQueryItem("key", key);
        url.setQuery(query);
    }

    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginDweetio::onNetworkReplyFinished);
    m_getReplies.insert(reply, thing);
}


void IntegrationPluginDweetio::onPluginTimer()
{
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() == getThingClassId) {
            getRequest(thing);
        }
    }
}
