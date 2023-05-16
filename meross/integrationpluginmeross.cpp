/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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


#include "integrationpluginmeross.h"
#include "plugininfo.h"

#include <plugintimer.h>
#include <network/networkdevicediscovery.h>
#include <network/networkaccessmanager.h>
#include <network/networkdevicediscoveryreply.h>

#include <QNetworkReply>
#include <QAuthenticator>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QMetaEnum>

IntegrationPluginMeross::IntegrationPluginMeross()
{

}

IntegrationPluginMeross::~IntegrationPluginMeross()
{
}

void IntegrationPluginMeross::discoverThings(ThingDiscoveryInfo *info)
{
    NetworkDeviceDiscoveryReply *reply = hardwareManager()->networkDeviceDiscovery()->discover();
    connect(reply, &NetworkDeviceDiscoveryReply::finished, reply, &NetworkDeviceDiscoveryReply::deleteLater);
        connect(reply, &NetworkDeviceDiscoveryReply::finished, info, [info, reply, this](){
        foreach (const NetworkDeviceInfo &deviceInfo, reply->networkDeviceInfos()) {
            qCDebug(dcMeross) << "Discovery result" << deviceInfo;
            if (deviceInfo.hostName().toLower().startsWith("meross_smart_plug") || deviceInfo.macAddressManufacturer().toLower().contains("meross")) {
                ThingDescriptor descriptor(plugThingClassId, "Meross Smart Plug", deviceInfo.macAddress());
                descriptor.setParams({Param(plugThingMacAddressParamTypeId, deviceInfo.macAddress())});

                Thing *existingThing = myThings().findByParams(descriptor.params());
                if (existingThing) {
                    qCInfo(dcMeross) << "Existing smart plug discovered";
                    descriptor.setThingId(existingThing->id());
                } else {
                    qCInfo(dcMeross) << "New smart plug discovered";
                }

                info->addThingDescriptor(descriptor);
            }
        }
        qCInfo(dcMeross) << "Discovery finished." << info->thingDescriptors().count() << "results.";
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginMeross::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter your Meross login credentials."));
}

void IntegrationPluginMeross::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    QNetworkRequest request(QUrl("https://iot.meross.com/v1/Auth/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QVariantMap params;
    params.insert("email", username);
    params.insert("password", secret);
    QByteArray encodedParams = QJsonDocument::fromVariant(params).toJson(QJsonDocument::Compact).toBase64();

    QByteArray nonce = QUuid::createUuid().toString().remove(QRegExp("[{}-]")).left(16).toUtf8();
    QByteArray initKey = "23x17ahWarFH6w29";
    QByteArray timestamp = QByteArray::number(QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);
    QByteArray signature = initKey + timestamp +  nonce + encodedParams;

    signature = QCryptographicHash::hash(signature, QCryptographicHash::Md5).toHex();

    QUrlQuery query;
    query.addQueryItem("params", encodedParams);
    query.addQueryItem("sign", signature);
    query.addQueryItem("timestamp", timestamp);
    query.addQueryItem("nonce", nonce);

    qCDebug(dcMeross) << "Requesting" << request.url() << query.toString();

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, query.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [=](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcMeross()) << "Error retrieving device key from cloud:" << reply->error() << reply->errorString();
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Failed to retrieve the device key from the Meross cloud."));
            return;
        }

        QByteArray payload = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcMeross) << "Failed to parse JSON from Meross cloud:" << error.error << error.errorString() << payload;
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Failed to retrieve the device key from the Meross cloud."));
            return;
        }

        QVariantMap params = jsonDoc.toVariant().toMap();

        if (params.value("apiStatus").toInt() != 0 || params.value("sysStatus").toInt() != 0) {
            qCWarning(dcMeross()) << "Error retrieving device key from cloud:" << reply->error() << reply->errorString();
            info->finish(Thing::ThingErrorAuthenticationFailure, params.value("info").toString());
            return;
        }

        QVariantMap data = params.value("data").toMap();

        qCDebug(dcMeross) << "key data:" << qUtf8Printable(jsonDoc.toJson());
        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("key", data.value("key").toString());
        pluginStorage()->endGroup();

        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginMeross::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    pluginStorage()->beginGroup(thing->id().toString());
    m_keys.insert(thing, pluginStorage()->value("key").toByteArray());
    pluginStorage()->endGroup();

    NetworkDeviceMonitor *monitor = m_deviceMonitors.take(thing);
    if (monitor) {
        hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(monitor);
    }

    monitor = hardwareManager()->networkDeviceDiscovery()->registerMonitor(MacAddress(thing->paramValue(plugThingMacAddressParamTypeId).toString()));
    m_deviceMonitors.insert(thing, monitor);

    pollDevice5s(thing);
    pollDevice60s(thing);

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginMeross::postSetupThing(Thing */*thing*/)
{
    if (!m_timer5s) {
        m_timer5s = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_timer5s, &PluginTimer::timeout, this, [=](){
            foreach (Thing *thing, myThings()) {
                if (m_deviceMonitors.value(thing)->reachable()) {
                    pollDevice5s(thing);
                }
            }
        });
    }
    if (!m_timer60s) {
        m_timer5s = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_timer5s, &PluginTimer::timeout, this, [=](){
            foreach (Thing *thing, myThings()) {
                if (m_deviceMonitors.value(thing)->reachable()) {
                    pollDevice60s(thing);
                }
            }
        });
    }
}

void IntegrationPluginMeross::thingRemoved(Thing *thing)
{
    hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_deviceMonitors.take(thing));

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_timer5s);
        m_timer5s = nullptr;
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_timer60s);
        m_timer60s = nullptr;
    }
}

void IntegrationPluginMeross::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == plugPowerActionTypeId) {
        QVariantMap payload;
        QVariantMap togglex;
        togglex.insert("channel", 0);
        togglex.insert("onoff", info->action().paramValue(plugPowerActionPowerParamTypeId).toBool() ? 1 : 0);
        payload.insert("togglex", togglex);
        QNetworkReply *reply = request(info->thing(), "Appliance.Control.ToggleX", SET, payload);
        connect(reply, &QNetworkReply::finished, info, [=](){
            qCDebug(dcMeross) << "reply" << reply->error() << reply->errorString() << reply->readAll();
            info->finish(Thing::ThingErrorNoError);
        });
    }
}

void IntegrationPluginMeross::pollDevice5s(Thing *thing)
{
    QNetworkReply *systemReply = request(thing, "Appliance.System.All");
    connect(systemReply, &QNetworkReply::finished, thing, [=](){
        if (systemReply->error() != QNetworkReply::NoError) {
            qCWarning(dcMeross) << "Error polling" << thing->name() << ":" << systemReply->error() << systemReply->errorString();
            thing->setStateValue(plugConnectedStateTypeId, false);
            return;
        }
        QByteArray data = systemReply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcMeross) << "Error parsing JSON reply from" << thing->name() << ":" << error.error << error.errorString();
            thing->setStateValue(plugConnectedStateTypeId, false);
            return;
        }

        qCDebug(dcMeross) << "System:" << qUtf8Printable(jsonDoc.toJson());

        QVariantMap payload = jsonDoc.toVariant().toMap().value("payload").toMap().value("all").toMap();

        QVariantMap digest = payload.value("digest").toMap();
        if (digest.value("togglex").toList().count() != 1) {
            qCWarning(dcMeross) << "Unexpected reply payload. Expected 1 togglex entry, got:" << qUtf8Printable(QJsonDocument::fromVariant(digest.value("togglex")).toJson());
            thing->setStateValue(plugConnectedStateTypeId, false);
            return;
        }
        thing->setStateValue(plugConnectedStateTypeId, true);

        thing->setStateValue(plugPowerStateTypeId, digest.value("togglex").toList().at(0).toMap().value("onoff").toInt() == 1);

    });

    QNetworkReply *electricityReply = request(thing, "Appliance.Control.Electricity");
    connect(electricityReply, &QNetworkReply::finished, thing, [electricityReply, thing](){
        if (electricityReply->error() != QNetworkReply::NoError) {
            return;
        }
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(electricityReply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            return;
        }
        qCDebug(dcMeross()) << "Electricity:" << qUtf8Printable(jsonDoc.toJson());

        QVariantMap electricityMap = jsonDoc.toVariant().toMap().value("payload").toMap().value("electricity").toMap();
        double power = electricityMap.value("power").toDouble() / 1000;
        thing->setStateValue(plugCurrentPowerStateTypeId, power);
    });
}

void IntegrationPluginMeross::pollDevice60s(Thing *thing)
{
    // Signal strength
    QNetworkReply *runtimeReply = request(thing, "Appliance.System.Runtime");
    connect(runtimeReply, &QNetworkReply::finished, thing, [runtimeReply, thing](){
        if (runtimeReply->error() != QNetworkReply::NoError) {
            return;
        }
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(runtimeReply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            return;
        }
        qCDebug(dcMeross()) << "Runtime:" << qUtf8Printable(jsonDoc.toJson());

        thing->setStateValue(plugSignalStrengthStateTypeId, jsonDoc.toVariant().toMap().value("payload").toMap().value("runtime").toMap().value("signal").toInt());
    });

    QNetworkReply *consumptionReply = request(thing, "Appliance.Control.ConsumptionX");
    connect(consumptionReply, &QNetworkReply::finished, thing, [consumptionReply, thing, this](){
        if (consumptionReply->error() != QNetworkReply::NoError) {
            return;
        }
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(consumptionReply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            return;
        }
        qCDebug(dcMeross()) << "Consumption:" << qUtf8Printable(jsonDoc.toJson());

        // We get a list of (max 10 or so) daily consumption totals but we're only interested in the grand total
        // So we're keeping a copy of the list and and add up changes in that list to the total
        double total = thing->stateValue(plugTotalEnergyConsumedStateTypeId).toDouble();

        QStringList timestamps;

        pluginStorage()->beginGroup(thing->id().toString());
        pluginStorage()->beginGroup("consumptionLogs");
        foreach (const QVariant &entry, jsonDoc.toVariant().toMap().value("payload").toMap().value("consumptionx").toList()) {
            QVariantMap entryMap = entry.toMap();
            QString timestamp = entryMap.value("date").toString();
            int value = entryMap.value("value").toInt();
            int loggedValue = pluginStorage()->value(timestamp).toInt();

//            qCDebug(dcMeross) << "entry:" << timestamp << "value" << value << loggedValue;

            if (loggedValue != value) {
                total -= 1.0 * loggedValue / 1000;
                total += 1.0 * value / 1000;
                pluginStorage()->setValue(timestamp, value);
            }

            timestamps.append(timestamp);
        }

        // Clean up old timestamps from pluginstorage
        foreach (const QString &childKey, pluginStorage()->childKeys()) {
            if (!timestamps.contains(childKey)) {
                pluginStorage()->remove(childKey);
            }
        }
        pluginStorage()->endGroup();
        pluginStorage()->endGroup();

        thing->setStateValue(plugTotalEnergyConsumedStateTypeId, total);
    });
}

QNetworkReply* IntegrationPluginMeross::request(Thing *thing, const QString &nameSpace, Method method, const QVariantMap &payload)
{
    QByteArray key = m_keys.value(thing);

    QString messageId = QUuid::createUuid().toString().remove(QRegExp("[{}-]"));
    qulonglong timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    quint16 timestampMs = timestamp % 1000;
    timestamp = timestamp / 1000;
    QByteArray signature = QCryptographicHash::hash(QString(messageId + key + QString::number(timestamp)).toUtf8(), QCryptographicHash::Md5).toHex();

    QVariantMap header;
    header.insert("from", "Meross");
    header.insert("messageId", messageId);
    header.insert("method", QMetaEnum::fromType<IntegrationPluginMeross::Method>().valueToKey(method));
    header.insert("namespace", nameSpace);
    header.insert("payloadVersion", 1);
    header.insert("timestamp", timestamp);
    header.insert("timestampMs", timestampMs);
    header.insert("sign", signature);

    QVariantMap data;
    data.insert("header", header);
    data.insert("payload", payload);

    QUrl url;
    url.setScheme("http");
    url.setHost(m_deviceMonitors.value(thing)->networkDeviceInfo().address().toString());
    url.setPath("/config");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    qCDebug(dcMeross) << "Requesting with key" << key << qUtf8Printable(QJsonDocument::fromVariant(data).toJson());

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    return reply;
}

