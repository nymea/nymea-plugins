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

#include "integrationplugintuya.h"
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

// API limits seem to be 120 for query and 600 for discovery. Adding some seconds to be on the save side.
const int queryInterval = 130;
const int discoveryInterval = 610;

IntegrationPluginTuya::IntegrationPluginTuya(QObject *parent): IntegrationPlugin(parent)
{
    m_devIdParamTypeIdsMap[tuyaClosableThingClassId] = tuyaClosableThingIdParamTypeId;
    m_devIdParamTypeIdsMap[tuyaSwitchThingClassId] = tuyaSwitchThingIdParamTypeId;

    m_connectedStateTypeIdsMap[tuyaClosableThingClassId] = tuyaClosableConnectedStateTypeId;
    m_connectedStateTypeIdsMap[tuyaSwitchThingClassId] = tuyaSwitchConnectedStateTypeId;

    m_powerStateTypeIdsMap[tuyaSwitchThingClassId] = tuyaSwitchPowerStateTypeId;
}

IntegrationPluginTuya::~IntegrationPluginTuya()
{

}

void IntegrationPluginTuya::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == tuyaCloudThingClassId) {
        QTimer *tokenRefreshTimer = m_tokenExpiryTimers.value(thing->id());
        if (!tokenRefreshTimer) {
            tokenRefreshTimer = new QTimer(thing);
            tokenRefreshTimer->setSingleShot(true);
            m_tokenExpiryTimers.insert(thing->id(), tokenRefreshTimer);
        }

        connect(tokenRefreshTimer, &QTimer::timeout, thing, [this, thing](){
            qCDebug(dcTuya()) << "Timer refresh token";
            refreshAccessToken(thing);
        });

        // If token refresh timer is already running, we just passed the login...
        if (tokenRefreshTimer->isActive()) {
            qCDebug(dcTuya()) << "Device already set up during pairing.";
            thing->setStateValue(tuyaCloudConnectedStateTypeId, true);
            thing->setStateValue(tuyaCloudLoggedInStateTypeId, true);
            pluginStorage()->beginGroup(thing->id().toString());
            QString username = pluginStorage()->value("username").toString();
            pluginStorage()->endGroup();
            thing->setStateValue(tuyaCloudUserDisplayNameStateTypeId, username);
            return info->finish(Thing::ThingErrorNoError);
        }

        // Else, let's refresh the token now
        qCDebug(dcTuya()) << "Setup refresh token";
        refreshAccessToken(thing);

        connect(this, &IntegrationPluginTuya::tokenRefreshed, info, [info](Thing *thing, bool success){
            if (thing == info->thing()) {
                if (!success) {
                    info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Error authenticating to Tuya thing."));
                } else {
                    info->finish(Thing::ThingErrorNoError);
                }
            }
        });

        return ;
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginTuya::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == tuyaCloudThingClassId) {
        updateChildDevices(thing);        
    } else {
        queryDevice(thing);
    }


    if (!m_pluginTimerQuery) {
        m_pluginTimerQuery = hardwareManager()->pluginTimerManager()->registerTimer(queryInterval);
        connect(m_pluginTimerQuery, &PluginTimer::timeout, this, [this](){
            foreach (Thing *d, myThings().filterByThingClassId(tuyaCloudThingClassId)) {
                if (m_pollQueue.value(d).isEmpty()) {
                    m_pollQueue[d] = myThings().filterByParentId(d->id());
                }
                if (m_pollQueue[d].count() > 0) {
                    queryDevice(m_pollQueue[d].takeFirst());
                }
            }
        });
    }

    if (!m_pluginTimerDiscovery) {
        m_pluginTimerDiscovery = hardwareManager()->pluginTimerManager()->registerTimer(discoveryInterval);
        connect(m_pluginTimerDiscovery, &PluginTimer::timeout, this, [this](){
            foreach (Thing *d, myThings().filterByThingClassId(tuyaCloudThingClassId)) {
                updateChildDevices(d);
            }
        });
    }
}

void IntegrationPluginTuya::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == tuyaCloudThingClassId) {
        m_pollQueue.remove(thing);
        m_tokenExpiryTimers.take(thing->id())->deleteLater();
    }

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimerDiscovery);
        m_pluginTimerDiscovery = nullptr;
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimerQuery);
        m_pluginTimerQuery = nullptr;
    }
}

void IntegrationPluginTuya::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter username and password for your Tuya (Smart Life) account."));
}

void IntegrationPluginTuya::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
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

    qCDebug(dcTuya()) << "Pairing Tuya service";
    connect(reply, &QNetworkReply::finished, info, [this, reply, info, username](){
        reply->deleteLater();
        QByteArray data = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcTuya()) << "Server error:" << reply->errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error communicating with Tuya server."));
            return;
        }

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcTuya()) << "Json parse error:" << error.errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error communicating with Tuya server."));
            return;
        }

        qCDebug(dcTuya()) << "Response from tuya api:" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));

        QVariantMap result = jsonDoc.toVariant().toMap();


        if (result.value("responseStatus") == "error") {
            qCDebug(dcTuya()) << "Error response from service.";
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Wrong username or password."));
            return;
        }

        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("accessToken", result.value("access_token").toString());
        pluginStorage()->setValue("refreshToken", result.value("refresh_token").toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->endGroup();

        int timeout = result.value("expires_in").toInt();

        QTimer *t = new QTimer(this);
        t->setSingleShot(true);
        t->start(timeout * 1000);

        m_tokenExpiryTimers.insert(info->thingId(), t);

        qCDebug(dcTuya()) << "Tuya thing paired. Token expires in" << timeout;

        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginTuya::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == tuyaSwitchPowerActionTypeId) {
        bool on = info->action().param(tuyaSwitchPowerActionPowerParamTypeId).value().toBool();
        QString devId = info->thing()->paramValue(tuyaSwitchThingIdParamTypeId).toString();
        controlTuyaSwitch(devId, "turnOnOff", on ? "1" : "0", info);
        connect(info, &ThingActionInfo::finished, [info, on](){
            if (info->status() == Thing::ThingErrorNoError) {
                info->thing()->setStateValue(tuyaSwitchPowerStateTypeId, on);
            }
        });
        return;
    }
    if (info->thing()->thingClassId() == tuyaClosableThingClassId) {
        QString devId = info->thing()->paramValue(tuyaClosableThingIdParamTypeId).toString();
        if (info->action().actionTypeId() == tuyaClosableOpenActionTypeId) {
            controlTuyaSwitch(devId, "turnOnOff", "1", info);
            return;
        }
        if (info->action().actionTypeId() == tuyaClosableCloseActionTypeId) {
            controlTuyaSwitch(devId, "turnOnOff", "0", info);
            return;
        }
        if (info->action().actionTypeId() == tuyaClosableStopActionTypeId) {
            controlTuyaSwitch(devId, "startStop", "0", info);
            return;
        }

    }
    Q_ASSERT_X(false, "tuyaplugin", "Unhandled action type " + info->action().actionTypeId().toByteArray());
}

void IntegrationPluginTuya::refreshAccessToken(Thing *thing)
{
    qCDebug(dcTuya()) << thing->name() << "Refreshing access token for" << thing->name();

    pluginStorage()->beginGroup(thing->id().toString());
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

    connect(reply, &QNetworkReply::finished, thing, [this, reply, thing](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcTuya()) << "Error refreshing access token";
            thing->setStateValue(tuyaCloudConnectedStateTypeId, false);
            thing->setStateValue(tuyaCloudLoggedInStateTypeId, false);
            emit tokenRefreshed(thing, false);
            return;
        }
        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcTuya()) << "Failed to parse json reply when refreshing access token" << error.errorString();
            thing->setStateValue(tuyaCloudConnectedStateTypeId, false);
            thing->setStateValue(tuyaCloudLoggedInStateTypeId, false);
            emit tokenRefreshed(thing, false);
            return;
        }

        if (jsonDoc.toVariant().toMap().isEmpty()) {
            qCWarning(dcTuya()) << "Empty response from Tuya server";
            thing->setStateValue(tuyaCloudConnectedStateTypeId, false);
            thing->setStateValue(tuyaCloudLoggedInStateTypeId, false);
            return;
        }

        pluginStorage()->beginGroup(thing->id().toString());
        pluginStorage()->setValue("accessToken", jsonDoc.toVariant().toMap().value("access_token").toString());
        pluginStorage()->setValue("refreshToken", jsonDoc.toVariant().toMap().value("refresh_token").toString());
        pluginStorage()->endGroup();
        int tokenExpiry = jsonDoc.toVariant().toMap().value("expires_in").toInt();

        qCDebug(dcTuya()) << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
        qCDebug(dcTuya()) << "Access token for" << thing->name() << "refreshed. Expires in" << tokenExpiry;

        QTimer *t = m_tokenExpiryTimers.value(thing->id());
        t->start(tokenExpiry);
        thing->setStateValue(tuyaCloudConnectedStateTypeId, true);
        thing->setStateValue(tuyaCloudLoggedInStateTypeId, true);

        pluginStorage()->beginGroup(thing->id().toString());
        QString username = pluginStorage()->value("username").toString();
        pluginStorage()->endGroup();
        thing->setStateValue(tuyaCloudUserDisplayNameStateTypeId, username);

        emit tokenRefreshed(thing, true);
    });
}

void IntegrationPluginTuya::updateChildDevices(Thing *thing)
{
    qCDebug(dcTuya()) << thing->name() << "Updating child devices";
    pluginStorage()->beginGroup(thing->id().toString());
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

    qCDebug(dcTuya()) << "Requesting:" << url.toString() << qUtf8Printable(jsonDoc.toJson());

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, jsonDoc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, [reply](){reply->deleteLater();});
    connect(reply, &QNetworkReply::finished, thing, [this, thing, reply](){
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
            qCWarning(dcTuya()) << "Invalid data from Tuya cloud:" << qUtf8Printable(jsonDoc.toJson());
            return;
        }

        qCDebug(dcTuya()) << "Discovery result:" << qUtf8Printable(jsonDoc.toJson());

        // OK. We have good data. let's cache it
        pluginStorage()->beginGroup(thing->id().toString());
        pluginStorage()->setValue("DiscoveryCache", data);
        pluginStorage()->endGroup();

        QVariantList devices = dataMap.value("payload").toMap().value("devices").toList();

        qCDebug(dcTuya())  << "Devices fetched";

        QList<ThingDescriptor> unknownDevices;

        foreach (const QVariant &deviceVariant, devices) {
            QVariantMap deviceMap = deviceVariant.toMap();
            QString devType = deviceMap.value("dev_type").toString();
            QString id = deviceMap.value("id").toString();
            QString name = deviceMap.value("name").toString();

            if (devType == "switch") {
                bool online = deviceMap.value("data").toMap().value("online").toBool();
                bool state = deviceMap.value("data").toMap().value("state").toBool();

                Thing *d = myThings().findByParams(ParamList() << Param(tuyaSwitchThingIdParamTypeId, id));
                if (d) {
                    qCDebug(dcTuya()) << "Found existing Tuya switch" << d->name() << id << name << (online ? "online:" : "offline") << (state ? "on": "off");
                    d->setStateValue(tuyaSwitchConnectedStateTypeId, online);
                    d->setStateValue(tuyaSwitchPowerStateTypeId, state);
                } else {
                    qCDebug(dcTuya()) << "Found new Tuya switch" << id << name;
                    ThingDescriptor descriptor(tuyaSwitchThingClassId, name, QString(), thing->id());
                    descriptor.setParams(ParamList() << Param(tuyaSwitchThingIdParamTypeId, id));
                    unknownDevices.append(descriptor);
                }
            } else if (devType == "cover") {
                bool online = deviceMap.value("data").toMap().value("online").toBool();

                Thing *d = myThings().findByParams(ParamList() << Param(tuyaClosableThingIdParamTypeId, id));
                if (d) {
                    qCDebug(dcTuya()) << "Found existing Tuya cover" << d->name() << id << name << (online ? "online" : "offline");
                    d->setStateValue(tuyaClosableConnectedStateTypeId, online);
                } else {
                    qCDebug(dcTuya()) << "Found new Tuya cover" << id << name;
                    ThingDescriptor descriptor(tuyaClosableThingClassId, name, QString(), thing->id());
                    descriptor.setParams(ParamList() << Param(tuyaClosableThingIdParamTypeId, id));
                    unknownDevices.append(descriptor);
                }
            } else {
                qCWarning(dcTuya()) << "Skipping unsupported thing type:" << devType;
                qCWarning(dcTuya()) << "Please report this including the following data:\n" << qUtf8Printable(QJsonDocument::fromVariant(deviceVariant).toJson());
                continue;
            }
        }

        if (!unknownDevices.isEmpty()) {
            emit autoThingsAppeared(unknownDevices);
        }
    });

}

void IntegrationPluginTuya::queryDevice(Thing *thing)
{
    qCDebug(dcTuya()) << "Updating thing:" << thing;

    QString devId = thing->paramValue(m_devIdParamTypeIdsMap.value(thing->thingClassId())).toString();

    pluginStorage()->beginGroup(thing->parentId().toString());
    QString accesToken = pluginStorage()->value("accessToken").toString();
    pluginStorage()->endGroup();

    QUrl url("http://px1.tuyaeu.com/homeassistant/skill");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QVariantMap header;
    header.insert("name", "QueryDevice");
    header.insert("namespace", "query");
    header.insert("payloadVersion", 1);

    QVariantMap payload;
    payload.insert("accessToken", accesToken);
    payload.insert("devId", devId);

    QVariantMap data;
    data.insert("header", header);
    data.insert("payload", payload);

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(data);

    qCDebug(dcTuya()) << "Requesting:" << url.toString() << qUtf8Printable(jsonDoc.toJson());

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, jsonDoc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, [reply](){reply->deleteLater();});
    connect(reply, &QNetworkReply::finished, thing, [this, thing, reply](){
        if (reply->error() !=  QNetworkReply::NoError) {
            qCWarning(dcTuya()) << "Error fetching devices from Tuya cloud" << reply->error();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcTuya()) << "Json parser error updating child devices" << error.errorString() << data;
            return;
        }

        QVariantMap result = jsonDoc.toVariant().toMap();
        if (result.value("header").toMap().value("code").toString() != "SUCCESS") {
            qCWarning(dcTuya()) << "Error quering tuya device" << thing->name() << qUtf8Printable(jsonDoc.toJson());
            if (result.value("header").toMap().value("code").toString() == "FrequentlyInvoke") {
                // Poll rate limit exceeded... As switching is not affected by this, all we lose is an update
                // from remote... So just do nothing here and hope next polling gets going again...
                return;
            }
            // Fall through to mark thing as offline on any other error.
        }

        bool connected = result.value("payload").toMap().value("data").toMap().value("online").toBool();
        bool state = result.value("payload").toMap().value("data").toMap().value("state").toBool();
        qCDebug(dcTuya()) << "Device" << thing->name() << "is online:" << connected << "on:" << state;

        thing->setStateValue(m_connectedStateTypeIdsMap.value(thing->thingClassId()), connected);

        if (m_powerStateTypeIdsMap.contains(thing->thingClassId())) {
            thing->setStateValue(m_powerStateTypeIdsMap.value(thing->thingClassId()), state);
        }
    });
}

void IntegrationPluginTuya::controlTuyaSwitch(const QString &devId, const QString &command, const QString &value, ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Thing *parentDevice = myThings().findById(thing->parentId());

    qCDebug(dcTuya()) << "Controlling Tuya device" << info->thing() << ". Parent:" << parentDevice->name() << "command:" << command << "value:" << value;

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
    payload.insert("devId", devId);
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
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error connecting to Tuya switch."));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcTuya()) << "Json parser error in control switch reply" << error.errorString() << data;
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Received an unexpected reply from the Tuya switch."));
            return;
        }

        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        bool success = dataMap.value("header").toMap().value("code").toString() == "SUCCESS";
        if (!success) {
            qCWarning(dcTuya()) << "Tuya response indicates an issue...";
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        qCDebug(dcTuya())  << "Device controlled";
        info->finish(Thing::ThingErrorNoError);
    });
}

