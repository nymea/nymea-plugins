/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
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



#include "deviceplugindweetio.h"
#include "plugininfo.h"
#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"

#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

DevicePluginDweetio::DevicePluginDweetio()
{

}

DevicePluginDweetio::~DevicePluginDweetio()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void DevicePluginDweetio::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginDweetio::onPluginTimer);
}

Device::DeviceSetupStatus DevicePluginDweetio::setupDevice(Device *device)
{

    if (device->deviceClassId() == postDeviceClassId) {

        QString thing = device->paramValue(postDeviceThingParamTypeId).toString();
        if (thing.isEmpty()){
            qDebug(dcDweetio) << "No thing name given, creating one";
            thing = QUuid::createUuid().toString();
        }
        return Device::DeviceSetupStatusSuccess;
    } else if (device->deviceClassId() == getDeviceClassId) {

        return Device::DeviceSetupStatusSuccess;
    }
    return Device::DeviceSetupStatusFailure;
}

void DevicePluginDweetio::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == getDeviceClassId){
        getRequest(device);
    }
}

void DevicePluginDweetio::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == getDeviceClassId) {
        foreach(QNetworkReply *reply, m_getReplies.keys()){
            if (m_getReplies.value(reply) == device) {
                m_getReplies.remove(reply);
            }
        }
    }

    if (device->deviceClassId() == postDeviceClassId) {
        foreach(QNetworkReply *reply, m_postReplies.keys()){
            if (m_postReplies.value(reply) == device) {
                m_postReplies.remove(reply);
            }
        }
    }
}

Device::DeviceError DevicePluginDweetio::executeAction(Device *device, const Action &action)
{
    qCDebug(dcDweetio) << "Execute action" << device->id() << action.id() << action.params();

    if (device->deviceClassId() == postDeviceClassId) {

        if (action.actionTypeId() == postContentDataActionTypeId) {

            QString content = action.param(postContentDataActionContentDataAreaParamTypeId).value().toString();
            postContent(content, device, action);

            return Device::DeviceErrorAsync;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginDweetio::postContent(const QString &content, Device *device, const Action &action)
{
    QUrl url = QString("https://dweet.io:443/dweet/for/") + device->paramValue(postDeviceThingParamTypeId).toString();
    QNetworkRequest request(url);
    request.setRawHeader("Content-Type", QString("application/json").toLocal8Bit());
    request.setRawHeader("Accept", QString("application/json").toLocal8Bit());
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());

    QString key = device->paramValue(postDeviceKeyParamTypeId).toString();
    if (!(key.isEmpty()) || !(key == "none")) {
        QUrlQuery query;
        query.addQueryItem("key", key);
        url.setQuery(query);
    }

    QVariantMap contentMap;
    contentMap.insert(device->paramValue(postDeviceContentNameParamTypeId).toString(), content);

    QByteArray data = QJsonDocument::fromVariant(contentMap).toJson(QJsonDocument::Compact);
    qDebug(dcDweetio) << "Dweet: " << data << "Url: " << url;

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, data);
    m_asyncActions.insert(reply, action.id());
    connect(reply, &QNetworkReply::finished, this, &DevicePluginDweetio::onNetworkReplyFinished);
    m_postReplies.insert(reply, device);

}

void DevicePluginDweetio::setConnectionStatus(bool status, Device *device)
{
    // Set connection status
    device->setStateValue(postConnectedStateTypeId, status);
}

void DevicePluginDweetio::processPostReply(const QVariantMap &data, Device *device)
{
    QString message = data.value("this").toString();
    qDebug(dcDweetio) << "Access Reply: " << message << "Device: " << device->name();
}

void DevicePluginDweetio::processGetReply(const QVariantMap &data, Device *device)
{
    QVariantList withList = data.value("with").toList();
    QVariantMap contentMap = withList.first().toMap().value("content").toMap();
    QString content = contentMap.value(device->paramValue(getDeviceContentNameParamTypeId).toString()).toString();
    device->setStateValue(getContentStateTypeId, content);

    qDebug(dcDweetio) << "Data: " << data << "Device: " << device->name();
}

void DevicePluginDweetio::onNetworkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (m_postReplies.contains(reply)) {
        Device *device = m_postReplies.value(reply);
        m_postReplies.remove(reply);
        ActionId actionId = m_asyncActions.value(reply);

        // check HTTP status code
        if ((status != 200) && (status != 204)) {
            qCWarning(dcDweetio) << "Update reply HTTP error:" << status << reply->errorString() << reply->readAll();
            emit actionExecutionFinished(actionId, Device::DeviceErrorMissingParameter);
            setConnectionStatus(false, device);
            reply->deleteLater();
            return;
        }

        emit actionExecutionFinished(actionId, Device::DeviceErrorNoError);
        setConnectionStatus(true, device);
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

        processPostReply(jsonDoc.toVariant().toMap(), device);

    } else if (m_getReplies.contains(reply)) {

        Device *device = m_getReplies.value(reply);
        m_getReplies.remove(reply);

        // check HTTP status code
        if ((status != 200) && (status != 204)) {
            qCWarning(dcDweetio) << "Update reply HTTP error:" << status << reply->errorString() << reply->readAll();
            setConnectionStatus(false, device);
            reply->deleteLater();
            return;
        }

        setConnectionStatus(true, device);
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

        processGetReply(jsonDoc.toVariant().toMap(), device);
    }
}

void DevicePluginDweetio::getRequest(Device *device)
{
    qCDebug(dcDweetio()) << "Refresh data for" << device->name();

    QUrl url = QString("https://dweet.io:443/get/latest/dweet/for/") + device->paramValue(postDeviceThingParamTypeId).toString();
    QNetworkRequest request(url);
    request.setRawHeader("Content-Type", QString("application/json").toLocal8Bit());
    request.setRawHeader("Accept", QString("application/json").toLocal8Bit());
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());

    QString key = device->paramValue(getDeviceKeyParamTypeId).toString();
    if (!(key.isEmpty()) || !(key == "none")) {
        QUrlQuery query;
        query.addQueryItem("key", key);
        url.setQuery(query);
    }

    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &DevicePluginDweetio::onNetworkReplyFinished);
    m_getReplies.insert(reply, device);
}


void DevicePluginDweetio::onPluginTimer()
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == getDeviceClassId) {
            getRequest(device);
        }
    }
}
