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

#include "deviceplugintuya.h"
#include "plugininfo.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QColor>

#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"

#include "plugintimer.h"

// API info:
// Python project: https://github.com/PaulAnnekov/tuyaha
// JS project: https://github.com/unparagoned/cloudtuya

DevicePluginTuya::DevicePluginTuya(QObject *parent): DevicePlugin(parent)
{

}

DevicePluginTuya::~DevicePluginTuya()
{

}

void DevicePluginTuya::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    if (device->deviceClassId() == tuyaCloudDeviceClassId) {
        QTimer *tokenRefreshTimer = m_tokenExpiryTimers.value(device->id());
        if (!tokenRefreshTimer) {
            tokenRefreshTimer = new QTimer(device);
            tokenRefreshTimer->setSingleShot(true);
            m_tokenExpiryTimers.insert(device->id(), tokenRefreshTimer);
        }

        connect(tokenRefreshTimer, &QTimer::timeout, device, [this, device](){
            qCDebug(dcTuya()) << "Timer refresh token";
            refreshAccessToken(device);
        });

        // If token refresh timer is already running, we just passed the login...
        if (tokenRefreshTimer->isActive()) {
            qCDebug(dcTuya()) << "Device already set up during pairing.";
            device->setStateValue(tuyaCloudConnectedStateTypeId, true);
            device->setStateValue(tuyaCloudLoggedInStateTypeId, true);
            pluginStorage()->beginGroup(device->id().toString());
            QString username = pluginStorage()->value("username").toString();
            pluginStorage()->endGroup();
            device->setStateValue(tuyaCloudUserDisplayNameStateTypeId, username);
            return info->finish(Device::DeviceErrorNoError);
        }

        // Else, let's refresh the token now
        qCDebug(dcTuya()) << "Setup refresh token";
        refreshAccessToken(device);

        connect(this, &DevicePluginTuya::tokenRefreshed, info, [info](Device *device, bool success){
            if (device == info->device()) {
                if (!success) {
                    info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("Error authenticating to Tuya device."));
                } else {
                    info->finish(Device::DeviceErrorNoError);
                }
            }
        });

        return ;
    }

    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginTuya::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == tuyaCloudDeviceClassId) {
        updateChildDevices(device);

        if (!m_pluginTimer) {
            m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(5);
            connect(m_pluginTimer, &PluginTimer::timeout, this, [this](){
                foreach (Device *d, myDevices().filterByDeviceClassId(tuyaCloudDeviceClassId)) {
                    updateChildDevices(d);
                }
            });
        }
    }
}

void DevicePluginTuya::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == tuyaCloudDeviceClassId) {
        m_tokenExpiryTimers.take(device->id())->deleteLater();
    }

    if (myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void DevicePluginTuya::startPairing(DevicePairingInfo *info)
{
    info->finish(Device::DeviceErrorNoError, QT_TR_NOOP("Please enter username and password for your Tuya (Smart Life) account."));
}

void DevicePluginTuya::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret)
{
    QUrl url(QString("http://px1.tuyaeu.com/homeassistant/auth.do"));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery query;
    query.addQueryItem("userName", username);
    query.addQueryItem("password", secret);
    query.addQueryItem("countryCode", "44");
    query.addQueryItem("bizType", "smart_life");
    query.addQueryItem("from", "tuya");


    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, query.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);

    qCDebug(dcTuya()) << "Pairing Tuya device";
    connect(reply, &QNetworkReply::finished, info, [this, reply, info, username](){
        reply->deleteLater();
        QByteArray data = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcTuya()) << "Server error:" << reply->errorString();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error communicating with Tuya server."));
            return;
        }

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcTuya()) << "Json parse error:" << error.errorString();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error communicating with Tuya server."));
            return;
        }

        qCDebug(dcTuya()) << "Response from tuya api:" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));

        QVariantMap result = jsonDoc.toVariant().toMap();


        if (result.value("responseStatus") == "error") {
            qCDebug(dcTuya()) << "Error response from service.";
            info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("Wrong username or password."));
            return;
        }

        pluginStorage()->beginGroup(info->deviceId().toString());
        pluginStorage()->setValue("accessToken", result.value("access_token").toString());
        pluginStorage()->setValue("refreshToken", result.value("refresh_token").toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->endGroup();

        int timeout = result.value("expires_in").toInt();

        QTimer *t = new QTimer(this);
        t->setSingleShot(true);
        t->start(timeout * 1000);

        m_tokenExpiryTimers.insert(info->deviceId(), t);

        qCDebug(dcTuya()) << "Tuya device paired. Token expires in" << timeout;

        info->finish(Device::DeviceErrorNoError);
    });
}

void DevicePluginTuya::executeAction(DeviceActionInfo *info)
{
    if (info->action().actionTypeId() == tuyaSwitchPowerActionTypeId) {
        bool on = info->action().param(tuyaSwitchPowerActionPowerParamTypeId).value().toBool();
        controlTuyaSwitch("turnOnOff", on ? "1" : "0", info);
        connect(info, &DeviceActionInfo::finished, [info, on](){
            info->device()->setStateValue(tuyaSwitchPowerStateTypeId, on);
        });
        return;
    }
    if (info->action().actionTypeId() == tuyaClosableOpenActionTypeId) {
        controlTuyaSwitch("turnOnOff", "1", info);
        return;
    }
    if (info->action().actionTypeId() == tuyaClosableCloseActionTypeId) {
        controlTuyaSwitch("turnOnOff", "0", info);
        return;
    }
    if (info->action().actionTypeId() == tuyaClosableStopActionTypeId) {
        controlTuyaSwitch("startStop", "0", info);
        return;
    }
    Q_ASSERT_X(false, "tuyaplugin", "Unhandled action type " + info->action().actionTypeId().toByteArray());
}

void DevicePluginTuya::refreshAccessToken(Device *device)
{
    qCDebug(dcTuya()) << device->name() << "Refreshing access token for" << device->name();

    pluginStorage()->beginGroup(device->id().toString());
    QString refreshToken = pluginStorage()->value("refreshToken").toString();
    pluginStorage()->endGroup();

    QUrl url("http://px1.tuyaeu.com/homeassistant/access.do");

    QUrlQuery query;
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", refreshToken);
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, [reply](){ reply->deleteLater(); });

    connect(reply, &QNetworkReply::finished, device, [this, reply, device](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcTuya()) << "Error refreshing access token";
            device->setStateValue(tuyaCloudConnectedStateTypeId, false);
            device->setStateValue(tuyaCloudLoggedInStateTypeId, false);
            emit tokenRefreshed(device, false);
            return;
        }
        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcTuya()) << "Failed to parse json reply when refreshing access token" << error.errorString();
            device->setStateValue(tuyaCloudConnectedStateTypeId, false);
            device->setStateValue(tuyaCloudLoggedInStateTypeId, false);
            emit tokenRefreshed(device, false);
            return;
        }

        if (jsonDoc.toVariant().toMap().isEmpty()) {
            qCWarning(dcTuya()) << "Empty response from Tuya server";
            device->setStateValue(tuyaCloudConnectedStateTypeId, false);
            device->setStateValue(tuyaCloudLoggedInStateTypeId, false);
            return;
        }

        pluginStorage()->beginGroup(device->id().toString());
        pluginStorage()->setValue("accessToken", jsonDoc.toVariant().toMap().value("access_token").toString());
        pluginStorage()->setValue("refreshToken", jsonDoc.toVariant().toMap().value("refresh_token").toString());
        pluginStorage()->endGroup();
        int tokenExpiry = jsonDoc.toVariant().toMap().value("expires_in").toInt();

        qCDebug(dcTuya()) << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
        qCDebug(dcTuya()) << "Access token for" << device->name() << "refreshed. Expires in" << tokenExpiry;

        QTimer *t = m_tokenExpiryTimers.value(device->id());
        t->start(tokenExpiry);
        device->setStateValue(tuyaCloudConnectedStateTypeId, true);
        device->setStateValue(tuyaCloudLoggedInStateTypeId, true);

        pluginStorage()->beginGroup(device->id().toString());
        QString username = pluginStorage()->value("username").toString();
        pluginStorage()->endGroup();
        device->setStateValue(tuyaCloudUserDisplayNameStateTypeId, username);

        emit tokenRefreshed(device, true);
    });
}

void DevicePluginTuya::updateChildDevices(Device *device)
{
    qCDebug(dcTuya()) << device->name() << "Updating child devices";
    pluginStorage()->beginGroup(device->id().toString());
    QString accesToken = pluginStorage()->value("accessToken").toString();
    pluginStorage()->endGroup();

    QUrl url("http://px1.tuyaeu.com/homeassistant/skill");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QVariantMap header;
    header.insert("name", "Discovery");
    header.insert("namespace", "discovery");
    header.insert("payloadVersion", 1);

    QVariantMap payload;
    payload.insert("accessToken", accesToken);

    QVariantMap data;
    data.insert("header", header);
    data.insert("payload", payload);

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(data);

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, jsonDoc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, [reply](){reply->deleteLater();});
    connect(reply, &QNetworkReply::finished, device, [this, device, reply](){
        if (reply->error() !=  QNetworkReply::NoError) {
            qCWarning(dcTuya()) << "Error fetching devices from Tuya cloud" << reply->error();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcTuya()) << "Json parser error updating child devices" << error.errorString();
            return;
        }

        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        if (!dataMap.contains("payload") || !dataMap.value("payload").toMap().contains("devices")) {
            qCWarning(dcTuya()) << "Invalid data from Tuya cloud:" << jsonDoc.toJson();
            return;
        }
        QVariantList devices = dataMap.value("payload").toMap().value("devices").toList();

        qCDebug(dcTuya())  << "Devices fetched";

        QList<DeviceDescriptor> unknownDevices;

        foreach (const QVariant &deviceVariant, devices) {
            QVariantMap deviceMap = deviceVariant.toMap();
            QString devType = deviceMap.value("dev_type").toString();
            QString id = deviceMap.value("id").toString();
            QString name = deviceMap.value("name").toString();

            if (devType == "switch") {
                bool online = deviceMap.value("data").toMap().value("online").toBool();
                bool state = deviceMap.value("data").toMap().value("state").toBool();

                Device *d = myDevices().findByParams(ParamList() << Param(tuyaSwitchDeviceIdParamTypeId, id));
                if (d) {
                    qCDebug(dcTuya()) << "Found existing Tuya switch" << d->name() << id << name << (online ? "online:" : "offline") << (state ? "on": "off");
                    d->setStateValue(tuyaSwitchConnectedStateTypeId, online);
                    d->setStateValue(tuyaSwitchPowerStateTypeId, state);
                } else {
                    qCDebug(dcTuya()) << "Found new Tuya switch" << id << name;
                    DeviceDescriptor descriptor(tuyaSwitchDeviceClassId, name, QString(), device->id());
                    descriptor.setParams(ParamList() << Param(tuyaSwitchDeviceIdParamTypeId, id));
                    unknownDevices.append(descriptor);
                }
            } else if (devType == "cover") {
                bool online = deviceMap.value("data").toMap().value("online").toBool();

                Device *d = myDevices().findByParams(ParamList() << Param(tuyaClosableDeviceIdParamTypeId, id));
                if (d) {
                    qCDebug(dcTuya()) << "Found existing Tuya cover" << d->name() << id << name << (online ? "online" : "offline");
                    d->setStateValue(tuyaClosableConnectedStateTypeId, online);
                } else {
                    qCDebug(dcTuya()) << "Found new Tuya cover" << id << name;
                    DeviceDescriptor descriptor(tuyaClosableDeviceClassId, name, QString(), device->id());
                    descriptor.setParams(ParamList() << Param(tuyaClosableDeviceIdParamTypeId, id));
                    unknownDevices.append(descriptor);
                }
            } else {
                qCWarning(dcTuya()) << "Skipping unsupported device type:" << devType;
                qCWarning(dcTuya()) << "Please report this including the following data:\n" << qUtf8Printable(QJsonDocument::fromVariant(deviceVariant).toJson());
                continue;
            }
        }

        if (!unknownDevices.isEmpty()) {
            emit autoDevicesAppeared(unknownDevices);
        }
    });

}

void DevicePluginTuya::controlTuyaSwitch(const QString &command, const QString &value, DeviceActionInfo *info)
{
    Device *device = info->device();
    Device *parentDevice = myDevices().findById(device->parentId());

    qCDebug(dcTuya()) << device->name() << "Controlling Tuya switch. Parent:" << parentDevice->name() << "command:" << command << "value:" << value;

    pluginStorage()->beginGroup(parentDevice->id().toString());
    QString accesToken = pluginStorage()->value("accessToken").toString();
    pluginStorage()->endGroup();

    QUrl url("http://px1.tuyaeu.com/homeassistant/skill");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QVariantMap header;
    header.insert("name", command);
    header.insert("namespace", "control");
    header.insert("payloadVersion", 1);

    QVariantMap payload;
    payload.insert("accessToken", accesToken);
    payload.insert("devId", device->paramValue(tuyaSwitchDeviceIdParamTypeId).toString());
    payload.insert("value", value);

    QVariantMap data;
    data.insert("header", header);
    data.insert("payload", payload);

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(data);

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, jsonDoc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, [reply](){reply->deleteLater();});
    connect(reply, &QNetworkReply::finished, info, [info, reply](){
        if (reply->error() !=  QNetworkReply::NoError) {
            qCWarning(dcTuya()) << "Error setting switch state" << reply->error();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error connecting to Tuya switch."));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcTuya()) << "Json parser error in control switch reply" << error.errorString() << data;
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Received an unexpected reply from the Tuya switch."));
            return;
        }

        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        bool success = dataMap.value("header").toMap().value("code").toString() == "SUCCESS";
        if (!success) {
            qCWarning(dcTuya()) << "Tuya response indicates an issue...";
            info->finish(Device::DeviceErrorHardwareFailure);
            return;
        }

        qCDebug(dcTuya())  << "Device controlled";
        info->finish(Device::DeviceErrorNoError);
    });
}

