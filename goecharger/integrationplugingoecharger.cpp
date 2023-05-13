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

#include "plugininfo.h"
#include "integrationplugingoecharger.h"
#include "network/networkdevicediscovery.h"

#include <QUrlQuery>
#include <QHostAddress>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonParseError>

#include "goediscovery.h"

// API documentation:
//     V1: https://github.com/goecharger/go-eCharger-API-v1
//     V2: https://github.com/goecharger/go-eCharger-API-v2

IntegrationPluginGoECharger::IntegrationPluginGoECharger()
{

}

void IntegrationPluginGoECharger::discoverThings(ThingDiscoveryInfo *info)
{
    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
        qCWarning(dcGoECharger()) << "The network discovery is not available on this platform.";
        info->finish(Thing::ThingErrorUnsupportedFeature, QT_TR_NOOP("The network device discovery is not available."));
        return;
    }

    GoeDiscovery *discovery = new GoeDiscovery(hardwareManager()->networkManager(), hardwareManager()->networkDeviceDiscovery(), this);
    connect(discovery, &GoeDiscovery::discoveryFinished, discovery, &GoeDiscovery::deleteLater);
    connect(discovery, &GoeDiscovery::discoveryFinished, info, [=](){
        foreach (const GoeDiscovery::Result &result, discovery->discoveryResults()) {
            QString title;
            if (!result.product.isEmpty() && !result.friendlyName.isEmpty() && result.friendlyName != result.product) {
                // We use the friendly name for the title, since the user seems to have given a name
                title = result.friendlyName;
            } else {
                title = result.product;
            }

            // There might be an other OEM than go-e, let's use this information since the user might not look for go-e (whitelabel)
            if (!result.manufacturer.isEmpty()) {
                title += " (" + result.manufacturer + ")";
            }

            QString description = "Serial: " + result.serialNumber + ", V: " + result.firmwareVersion + " - " + result.networkDeviceInfo.address().toString();
            qCDebug(dcGoECharger()) << "-->" << title << description;
            ThingDescriptor descriptor(goeHomeThingClassId, title, description);
            ParamList params;
            params << Param(goeHomeThingMacAddressParamTypeId, result.networkDeviceInfo.macAddress());
            params << Param(goeHomeThingSerialNumberParamTypeId, result.serialNumber);
            params << Param(goeHomeThingApiVersionParamTypeId, result.apiAvailableV2 ? 2 : 1); // always use v2 if available...
            descriptor.setParams(params);

            // Check if we already have set up this device
            Things existingThings = myThings().filterByParam(goeHomeThingMacAddressParamTypeId, result.networkDeviceInfo.macAddress());
            if (existingThings.count() == 1) {
                qCDebug(dcGoECharger()) << "This wallbox already exists in the system!" << result.networkDeviceInfo;
                descriptor.setThingId(existingThings.first()->id());
            }

            info->addThingDescriptor(descriptor);
        }

        info->finish(Thing::ThingErrorNoError);
    });

    // Start the discovery process
    discovery->startDiscovery();
}

void IntegrationPluginGoECharger::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcGoECharger()) << "Setting up" << thing << thing->params();

    MacAddress macAddress = MacAddress(thing->paramValue(goeHomeThingMacAddressParamTypeId).toString());
    if (!macAddress.isValid()) {
        qCWarning(dcGoECharger()) << "The configured mac address is not valid" << thing->params();
        info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("The MAC address is not known. Please reconfigure the thing."));
        return;
    }

    // Handle reconfigure
    if (m_monitors.contains(thing))
        hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));

    // Create the monitor
    NetworkDeviceMonitor *monitor = hardwareManager()->networkDeviceDiscovery()->registerMonitor(macAddress);
    m_monitors.insert(thing, monitor);
    QHostAddress address = getHostAddress(thing);
    if (address.isNull()) {
        qCWarning(dcGoECharger()) << "Cannot set up go-eCharger. The host address is not known yet. Maybe it will be available in the next run...";
        hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The host address is not known yet. Trying later again."));
        return;
    }

    // Clean up in case the setup gets aborted
    connect(info, &ThingSetupInfo::aborted, monitor, [=](){
        if (m_monitors.contains(thing)) {
            qCDebug(dcGoECharger()) << "Unregister monitor because setup has been aborted.";
            hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
        }
    });

    connect(monitor, &NetworkDeviceMonitor::reachableChanged, thing, [=](bool reachable){
        qCDebug(dcGoECharger()) << "Network device monitor reachable changed for" << thing->name() << reachable;
        if (reachable && thing->setupComplete() && !thing->stateValue("connected").toBool()) {

            // The device is reachable again and we have already set it up.
            // Update data and optionally reconfigure the mqtt channel

            QNetworkReply *reply = hardwareManager()->networkManager()->get(buildStatusRequest(thing));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, thing, [=](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcGoECharger()) << "HTTP status reply returned error:" << reply->errorString() << reply->readAll();
                    return;
                }

                QByteArray data = reply->readAll();
                QJsonParseError error;
                QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                if (error.error != QJsonParseError::NoError) {
                    qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(data) << error.errorString();
                    return;
                }

                qCDebug(dcGoECharger()) << "Initial status map" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Compact));
                QVariantMap statusMap = jsonDoc.toVariant().toMap();

                ApiVersion apiVersion = getApiVersion(thing);
                switch (apiVersion) {
                case ApiVersion1:
                    if (thing->paramValue(goeHomeThingUseMqttParamTypeId).toBool()) {
                        // Verify mqtt client and set it up
                        qCDebug(dcGoECharger()) << "Setup using MQTT connection for" << thing;
                        reconfigureMqttChannelV1(thing, statusMap);
                    } else {
                        // Since we are not using mqtt, we are done with the setup, the refresh timer will be configured in post setup
                        qCDebug(dcGoECharger()) << "Setup using HTTP finished successfully";
                        updateV1(thing, statusMap);
                    }
                    break;
                case ApiVersion2:
                    if (thing->paramValue(goeHomeThingUseMqttParamTypeId).toBool()) {
                        // Verify mqtt client and set it up
                        qCDebug(dcGoECharger()) << "Setup using MQTT connection for" << thing;
                        reconfigureMqttChannelV2(thing);
                    } else {
                        // Since we are not using mqtt, we are done with the setup, the refresh timer will be configured in post setup
                        qCDebug(dcGoECharger()) << "Setup using HTTP finished successfully";
                        updateV2(thing, statusMap);
                    }
                    break;
                }
            });
        }
    });

    // Wait for the monitor to be ready
    if (monitor->reachable()) {
        // Thing already reachable...let's finish the setup
        setupGoeHome(info);
    } else {
        qCDebug(dcGoECharger()) << "Wait for the network monitor to get reachable";
        connect(monitor, &NetworkDeviceMonitor::reachableChanged, info, [=](bool reachable){
            if (reachable) {
                setupGoeHome(info);
            }
        });
    }

    Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
}

void IntegrationPluginGoECharger::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == goeHomeThingClassId) {
        // Set up refresh timer if needed and if we are not using mqtt
        if (!thing->paramValue(goeHomeThingUseMqttParamTypeId).toBool() && !m_refreshTimer) {
            qCDebug(dcGoECharger()) << "Enabling HTTP refresh timer...";
            m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(4);
            connect(m_refreshTimer, &PluginTimer::timeout, this, &IntegrationPluginGoECharger::refreshHttp);
            m_refreshTimer->start();
        }

        if (thing->paramValue(goeHomeThingUseMqttParamTypeId).toBool()) {
            // make sure the connected state has been set properly
            switch (getApiVersion(thing)) {
            case ApiVersion1:
                if (m_mqttChannelsV1.contains(thing)) {
                    thing->setStateValue("connected", m_mqttChannelsV1.value(thing)->isConnected());
                    if (!m_mqttChannelsV1.value(thing)->isConnected()) {
                        markAsDisconnected(thing);
                    }
                }
                break;
            case ApiVersion2:
                if (m_mqttChannelsV2.contains(thing)) {
                    thing->setStateValue("connected", m_mqttChannelsV2.value(thing)->isConnected());
                    if (!m_mqttChannelsV2.value(thing)->isConnected()) {
                        markAsDisconnected(thing);
                    }
                }
                break;
            }
        }
    }
}

void IntegrationPluginGoECharger::thingRemoved(Thing *thing)
{
    // Cleanup mqtt channels if set up
    switch (getApiVersion(thing)) {
    case ApiVersion1:
        if (m_mqttChannelsV1.contains(thing)) {
            hardwareManager()->mqttProvider()->releaseChannel(m_mqttChannelsV1.take(thing));
        }
        break;
    case ApiVersion2:
        if (m_mqttChannelsV2.contains(thing)) {
            hardwareManager()->mqttProvider()->releaseChannel(m_mqttChannelsV2.take(thing));
        }
        break;
    }

    // Cleanup possible pending replies
    if (m_pendingReplies.contains(thing) && m_pendingReplies.value(thing)) {
        m_pendingReplies.take(thing)->abort();
    }

    // Clean up refresh timer if set up
    if (m_refreshTimer && myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
        m_refreshTimer = nullptr;
    }
}

void IntegrationPluginGoECharger::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();
    ApiVersion apiVersion = getApiVersion(thing);
    QHostAddress address = getHostAddress(thing);

    if (thing->thingClassId() != goeHomeThingClassId) {
        info->finish(Thing::ThingErrorThingClassNotFound);
        return;
    }

    if (!thing->stateValue("connected").toBool()) {
        qCWarning(dcGoECharger()) << thing << "failed to execute action. The device seems not to be connected.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    switch (apiVersion) {
    case ApiVersion1:
        if (action.actionTypeId() == goeHomePowerActionTypeId) {
            bool power = action.paramValue(goeHomePowerActionPowerParamTypeId).toBool();
            qCDebug(dcGoECharger()) << "Setting charging allowed to" << power;
            // Set the allow value
            QString configuration = QString("alw=%1").arg(power ? 1: 0);
            sendActionRequestV1(thing, info, configuration, QVariant(power));
            return;
        } else if (action.actionTypeId() == goeHomeMaxChargingCurrentActionTypeId) {
            uint maxChargingCurrent = action.paramValue(goeHomeMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt();
            qCDebug(dcGoECharger()) << "Setting max charging current to" << maxChargingCurrent << "A";
            // FIXME: check if we can use amx since it is better for pv charging, not all version seen implement amx
            // Maybe check if the user sets it or a automatism
            QString configuration = QString("amp=%1").arg(maxChargingCurrent);
            sendActionRequestV1(thing, info, configuration, QVariant(maxChargingCurrent));
            return;
        } else {
            info->finish(Thing::ThingErrorActionTypeNotFound);
        }
        break;
    case ApiVersion2:
        if (action.actionTypeId() == goeHomePowerActionTypeId) {
            bool power = action.paramValue(goeHomePowerActionPowerParamTypeId).toBool();
            qCDebug(dcGoECharger()) << "Setting charging allowed to" << power;
            // Warning: using QUrlQuery not always works here due to standard mixing from go-e:
            // The url query has to be JSON encoded, i.e. <url>/set?fna="mein charger"
            QUrlQuery configuration;
            // 0 neutral (prefere on), 1 off, 2 on force
            configuration.addQueryItem("frc", (power ? "0": "1"));
            QNetworkRequest request = buildConfigurationRequestV2(address, configuration);
            QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
            connect(info, &ThingActionInfo::aborted, reply, &QNetworkReply::abort);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [=](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcGoECharger()) << "Execute action failed for" << thing->name() << "HTTP error:" << reply->errorString() << reply->readAll() << "Request was:" << request.url().toString();
                    info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The wallbox does not seem to be reachable."));
                    return;
                }

                QByteArray data = reply->readAll();
                QJsonParseError error;
                QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                if (error.error != QJsonParseError::NoError) {
                    qCWarning(dcGoECharger()) << "Execute action failed for" << thing->name() << "Parsing data failed:" << qUtf8Printable(data) << error.errorString() << "Request was:" << request.url().toString();
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The wallbox returned invalid data."));
                    return;
                }

                QVariantMap responseCode = jsonDoc.toVariant().toMap();
                if (responseCode.value("frc", false).toBool()) {
                    qCDebug(dcGoECharger()) << "Execute action finished successfully. Power" << power;
                    thing->setStateValue("power", power);
                    info->finish(Thing::ThingErrorNoError);
                } else {
                    qCWarning(dcGoECharger()) << "Action finished with error:" << responseCode.value("frc").toString();
                    info->finish(Thing::ThingErrorHardwareFailure);
                }
            });

            return;
        } else if (action.actionTypeId() == goeHomeMaxChargingCurrentActionTypeId) {
            uint ampere = action.paramValue(goeHomeMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt();
            qCDebug(dcGoECharger()) << "Setting max charging current to" << ampere << "A";
            // Warning: using QUrlQuery not always works here due to standard mixing from go-e:
            // The url query has to be JSON encoded, i.e. <url>/set?fna="mein charger"
            QUrlQuery configuration;
            configuration.addQueryItem("amp", QString::number(ampere));
            QNetworkRequest request = buildConfigurationRequestV2(address, configuration);
            QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
            connect(info, &ThingActionInfo::aborted, reply, &QNetworkReply::abort);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [=](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcGoECharger()) << "Execute action failed for" << thing->name() << "HTTP error:" << reply->errorString() << reply->readAll() << "Request was:" << request.url().toString();
                    info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The wallbox does not seem to be reachable."));
                    return;
                }

                QByteArray data = reply->readAll();
                QJsonParseError error;
                QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                if (error.error != QJsonParseError::NoError) {
                    qCWarning(dcGoECharger()) << "Execute action failed for" << thing->name() << "Failed to parse data" << qUtf8Printable(data) << error.errorString() << "Request was:" << request.url().toString();
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The wallbox returned invalid data."));
                    return;
                }

                QVariantMap responseCode = jsonDoc.toVariant().toMap();
                if (responseCode.value("amp", false).toBool()) {
                    qCDebug(dcGoECharger()) << "Execute action finished successfully. Charging current" << ampere;
                    thing->setStateValue("maxChargingCurrent", ampere);
                    info->finish(Thing::ThingErrorNoError);
                } else {
                    qCWarning(dcGoECharger()) << "Action finished with error:" << responseCode.value("amp").toString();
                    info->finish(Thing::ThingErrorHardwareFailure);
                }
            });
            return;
        } else if (action.actionTypeId() == goeHomeDesiredPhaseCountActionTypeId) {
            uint desiredPhases = action.paramValue(goeHomeDesiredPhaseCountActionDesiredPhaseCountParamTypeId).toUInt();
            QString value = desiredPhases == 1 ? "1" : "2"; // 1: 1-Phase, 2: 3-Phases (0: Auto)
            qCDebug(dcGoECharger()) << "Setting phaseSwitchMode to" << value;
            // Warning: using QUrlQuery not always works here due to standard mixing from go-e:
            // The url query has to be JSON encoded, i.e. <url>/set?fna="mein charger"
            QUrlQuery configuration;
            configuration.addQueryItem("psm", value);
            QNetworkRequest request = buildConfigurationRequestV2(address, configuration);
            QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
            connect(info, &ThingActionInfo::aborted, reply, &QNetworkReply::abort);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [=](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcGoECharger()) << "Execute action failed for" << thing->name() << "HTTP error:" << reply->errorString() << reply->readAll() << "Request was:" << request.url().toString();
                    info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The wallbox does not seem to be reachable."));
                    return;
                }

                QByteArray data = reply->readAll();
                QJsonParseError error;
                QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                if (error.error != QJsonParseError::NoError) {
                    qCWarning(dcGoECharger()) << "Execute action failed for" << thing->name() << "Failed to parse data" << qUtf8Printable(data) << error.errorString() << "Request was:" << request.url().toString();
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The wallbox returned invalid data."));
                    return;
                }

                QVariantMap responseCode = jsonDoc.toVariant().toMap();
                if (responseCode.value("psm", false).toBool()) {
                    qCDebug(dcGoECharger()) << "Execute action finished successfully. phaseSwitchMode" << value << "desired phases:" << desiredPhases;
                    thing->setStateValue(goeHomeDesiredPhaseCountStateTypeId, desiredPhases);
                    info->finish(Thing::ThingErrorNoError);
                } else {
                    qCWarning(dcGoECharger()) << "Action finished with error:" << responseCode.value("psm").toString();
                    info->finish(Thing::ThingErrorHardwareFailure);
                }
            });
            return;
        } else {
            info->finish(Thing::ThingErrorActionTypeNotFound);
        }
        break;
    }
}


void IntegrationPluginGoECharger::setupGoeHome(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QNetworkReply *reply = hardwareManager()->networkManager()->get(buildStatusRequest(thing));
    connect(info, &ThingSetupInfo::aborted, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [=](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcGoECharger()) << "HTTP status reply returned error:" << reply->errorString() << reply->readAll();
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The wallbox does not seem to be reachable."));
            hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(data) << error.errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The wallbox returned invalid data."));
            hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
            return;
        }

        QHostAddress address = getHostAddress(thing);
        ApiVersion apiVersion = getApiVersion(thing);

        switch (apiVersion) {
        case ApiVersion1: {
            // Handle reconfigure
            if (m_mqttChannelsV1.contains(thing))
                hardwareManager()->mqttProvider()->releaseChannel(m_mqttChannelsV1.take(thing));

            if (m_pendingReplies.contains(thing))
                m_pendingReplies.take(thing)->abort();

            qCDebug(dcGoECharger()) << "Initial status map" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Compact));
            QVariantMap statusMap = jsonDoc.toVariant().toMap();
            if (thing->paramValue(goeHomeThingUseMqttParamTypeId).toBool()) {
                // Verify mqtt client and set it up
                qCDebug(dcGoECharger()) << "Setting up using MQTT connection for" << thing;
                setupMqttChannelV1(info, address, statusMap);
            } else {
                // Since we are not using mqtt, we are done with the setup, the refresh timer will be configured in post setup
                info->finish(Thing::ThingErrorNoError);

                qCDebug(dcGoECharger()) << "Setup using HTTP finished successfully";
                thing->setStateValue("connected", true);
                updateV1(thing, statusMap);
            }
            break;
        }
        case ApiVersion2:
            // Handle reconfigure
            if (m_mqttChannelsV2.contains(thing))
                hardwareManager()->mqttProvider()->releaseChannel(m_mqttChannelsV2.take(thing));

            if (m_pendingReplies.contains(thing))
                m_pendingReplies.take(thing)->abort();

            qCDebug(dcGoECharger()) << "Initial status map" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Compact));
            QVariantMap statusMap = jsonDoc.toVariant().toMap();
            if (thing->paramValue(goeHomeThingUseMqttParamTypeId).toBool()) {
                // Verify mqtt client and set it up
                qCDebug(dcGoECharger()) << "Setting up using MQTT connection for" << thing;
                setupMqttChannelV2(info, address, statusMap);
            } else {
                // Since we are not using mqtt, we are done with the setup, the refresh timer will be configured in post setup
                info->finish(Thing::ThingErrorNoError);

                qCDebug(dcGoECharger()) << "Setup using HTTP finished successfully";
                thing->setStateValue("connected", true);
                updateV2(thing, statusMap);
            }
            break;
        }
    });
}

QNetworkRequest IntegrationPluginGoECharger::buildStatusRequest(Thing *thing)
{
    QHostAddress address = getHostAddress(thing);
    ApiVersion apiVersion = getApiVersion(thing);

    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(address.toString());

    switch (apiVersion) {
    case ApiVersion1:
        requestUrl.setPath("/status");
        break;
    case ApiVersion2:
        requestUrl.setPath("/api/status");
        break;
    }

    return QNetworkRequest(requestUrl);
}

QHostAddress IntegrationPluginGoECharger::getHostAddress(Thing *thing)
{
    if (m_monitors.contains(thing))
        return m_monitors.value(thing)->networkDeviceInfo().address();

    return QHostAddress();
}

IntegrationPluginGoECharger::ApiVersion IntegrationPluginGoECharger::getApiVersion(Thing *thing)
{
    return static_cast<ApiVersion>(thing->paramValue(goeHomeThingApiVersionParamTypeId).toUInt());
}

void IntegrationPluginGoECharger::updateV1(Thing *thing, const QVariantMap &statusMap)
{
    // Parse status map and update states...
    thing->setStateValue(goeHomePowerStateTypeId, (statusMap.value("alw").toUInt() == 0 ? false : true));

    CarState carState = static_cast<CarState>(statusMap.value("car").toUInt());
    switch (carState) {
    case CarStateReadyNoCar:
        thing->setStateValue(goeHomeCarStatusStateTypeId, "Ready but no vehicle connected");
        thing->setStateValue(goeHomePluggedInStateTypeId, false);
        break;
    case CarStateCharging:
        thing->setStateValue(goeHomeCarStatusStateTypeId, "Vehicle loads");
        thing->setStateValue(goeHomePluggedInStateTypeId, true);
        break;
    case CarStateWaitForCar:
        thing->setStateValue(goeHomeCarStatusStateTypeId, "Waiting for vehicle");
        thing->setStateValue(goeHomePluggedInStateTypeId, false);
        break;
    case CarStateChargedCarConnected:
        thing->setStateValue(goeHomeCarStatusStateTypeId, "Charging finished and vehicle still connected");
        thing->setStateValue(goeHomePluggedInStateTypeId, true);
        break;
    default:
        thing->setStateValue(goeHomeCarStatusStateTypeId, "Unknown");
        thing->setStateValue(goeHomePluggedInStateTypeId, false);
        break;
    }

    thing->setStateValue(goeHomeChargingStateTypeId, carState == CarStateCharging && thing->stateValue(goeHomePowerStateTypeId).toBool() == true);

    Access accessStatus = static_cast<Access>(statusMap.value("ast").toUInt());
    switch (accessStatus) {
    case AccessOpen:
        thing->setStateValue(goeHomeAccessStateTypeId, "Open");
        break;
    case AccessRfid:
        thing->setStateValue(goeHomeAccessStateTypeId, "RFID");
        break;
    case AccessAuto:
        thing->setStateValue(goeHomeAccessStateTypeId, "Automatic");
        break;
    }

    QVariantList temperatureSensorList = statusMap.value("tma").toList();
    if (temperatureSensorList.count() >= 1)
        thing->setStateValue(goeHomeTemperatureSensor1StateTypeId, temperatureSensorList.at(0).toDouble());

    if (temperatureSensorList.count() >= 2)
        thing->setStateValue(goeHomeTemperatureSensor2StateTypeId, temperatureSensorList.at(1).toDouble());

    if (temperatureSensorList.count() >= 3)
        thing->setStateValue(goeHomeTemperatureSensor3StateTypeId, temperatureSensorList.at(2).toDouble());

    if (temperatureSensorList.count() >= 4)
        thing->setStateValue(goeHomeTemperatureSensor4StateTypeId, temperatureSensorList.at(3).toDouble());

    thing->setStateValue(goeHomeTotalEnergyConsumedStateTypeId, statusMap.value("eto").toUInt() / 10.0);
    thing->setStateValue(goeHomeSessionEnergyStateTypeId, statusMap.value("dws").toUInt() / 360000.0);
    thing->setStateValue(goeHomeUpdateAvailableStateTypeId, (statusMap.value("upd").toUInt() == 0 ? false : true));
    thing->setStateValue(goeHomeFirmwareVersionStateTypeId, statusMap.value("fwv").toString());
    // FIXME: check if we can use amx since it is better for pv charging, not all version seen implement this
    thing->setStateValue(goeHomeMaxChargingCurrentStateTypeId, statusMap.value("amp").toUInt());
    thing->setStateValue(goeHomeAdapterConnectedStateTypeId, (statusMap.value("adi").toUInt() == 0 ? false : true));
    thing->setStateValue(goeHomeDesiredPhaseCountStateTypeId, statusMap.value("psm").toUInt()  == 1 ? 1 : 3);

    uint amaLimit = statusMap.value("ama").toUInt();
    uint cableLimit = statusMap.value("cbl").toUInt();

    thing->setStateValue(goeHomeAbsoluteMaxAmpereStateTypeId, amaLimit);
    thing->setStateValue(goeHomeCableType2AmpereStateTypeId, cableLimit);

    // Set the limit for the max charging amps
    if (cableLimit != 0) {
        thing->setStateMaxValue(goeHomeMaxChargingCurrentStateTypeId, qMin(amaLimit, cableLimit));
    } else {
        thing->setStateMaxValue(goeHomeMaxChargingCurrentStateTypeId, amaLimit);
    }

    // Parse nrg array
    uint voltagePhaseA = 0; uint voltagePhaseB = 0; uint voltagePhaseC = 0;
    double amperePhaseA = 0; double amperePhaseB = 0; double amperePhaseC = 0;
    double currentPower = 0; double powerPhaseA = 0; double powerPhaseB = 0; double powerPhaseC = 0;

    QVariantList measurementList = statusMap.value("nrg").toList();
    if (measurementList.count() >= 1)
        voltagePhaseA = measurementList.at(0).toUInt();

    if (measurementList.count() >= 2)
        voltagePhaseB = measurementList.at(1).toUInt();

    if (measurementList.count() >= 3)
        voltagePhaseC = measurementList.at(2).toUInt();

    if (measurementList.count() >= 5)
        amperePhaseA = measurementList.at(4).toUInt() / 10.0; // 0,1 A value 123 -> 12,3 A

    if (measurementList.count() >= 6)
        amperePhaseB = measurementList.at(5).toUInt() / 10.0; // 0,1 A value 123 -> 12,3 A

    if (measurementList.count() >= 7)
        amperePhaseC = measurementList.at(6).toUInt() / 10.0; // 0,1 A value 123 -> 12,3 A

    if (measurementList.count() >= 8)
        powerPhaseA = measurementList.at(7).toUInt() * 100.0; // 0.1kW -> W

    if (measurementList.count() >= 9)
        powerPhaseB = measurementList.at(8).toUInt() * 100.0; // 0.1kW -> W

    if (measurementList.count() >= 10)
        powerPhaseC = measurementList.at(9).toUInt() * 100.0; // 0.1kW -> W

    if (measurementList.count() >= 12)
        currentPower = measurementList.at(11).toUInt() * 10.0; // 0.01kW -> W

    // Update all states
    thing->setStateValue(goeHomeVoltagePhaseAStateTypeId, voltagePhaseA);
    thing->setStateValue(goeHomeVoltagePhaseBStateTypeId, voltagePhaseB);
    thing->setStateValue(goeHomeVoltagePhaseCStateTypeId, voltagePhaseC);
    thing->setStateValue(goeHomeCurrentPhaseAStateTypeId, amperePhaseA);
    thing->setStateValue(goeHomeCurrentPhaseBStateTypeId, amperePhaseB);
    thing->setStateValue(goeHomeCurrentPhaseCStateTypeId, amperePhaseC);
    thing->setStateValue(goeHomeCurrentPowerPhaseAStateTypeId, powerPhaseA);
    thing->setStateValue(goeHomeCurrentPowerPhaseBStateTypeId, powerPhaseB);
    thing->setStateValue(goeHomeCurrentPowerPhaseCStateTypeId, powerPhaseC);

    thing->setStateValue(goeHomeCurrentPowerStateTypeId, currentPower);

    // Check how many phases are actually charging, and update the phase count only if something happens on the phases (current or power)
    if (amperePhaseA != 0 || amperePhaseB != 0 || amperePhaseC != 0) {
        uint phaseCount = 0;
        if (amperePhaseA != 0)
            phaseCount += 1;

        if (amperePhaseB != 0)
            phaseCount += 1;

        if (amperePhaseC != 0)
            phaseCount += 1;

        thing->setStateValue(goeHomePhaseCountStateTypeId, phaseCount);
    }
}

QNetworkRequest IntegrationPluginGoECharger::buildConfigurationRequestV1(const QHostAddress &address, const QString &configuration)
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(address.toString());
    requestUrl.setPath("/mqtt");
    QUrlQuery query;
    query.addQueryItem("payload", configuration);
    requestUrl.setQuery(query);
    return QNetworkRequest(requestUrl);
}

void IntegrationPluginGoECharger::sendActionRequestV1(Thing *thing, ThingActionInfo *info, const QString &configuration, const QVariant &value)
{
    // Lets use rest here since we get a reply on the rest request.
    // For using MQTT publish to topic "go-eCharger/<serialnumber>/cmd/req"
    QHostAddress address = getHostAddress(thing);
    QNetworkRequest request = buildConfigurationRequestV1(address, configuration);
    QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
    connect(info, &ThingActionInfo::aborted, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [=](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcGoECharger()) << "HTTP status reply returned error:" << reply->errorString();
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The wallbox does not seem to be reachable."));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(data) << error.errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The wallbox returned invalid data."));
            return;
        }

        thing->setStateValue(info->action().actionTypeId(), value);
        info->finish(Thing::ThingErrorNoError);
        updateV1(thing, jsonDoc.toVariant().toMap());
    });
}

void IntegrationPluginGoECharger::setupMqttChannelV1(ThingSetupInfo *info, const QHostAddress &address, const QVariantMap &statusMap)
{
    Thing *thing = info->thing();

    QString serialNumber = statusMap.value("sse").toString();
    QString clientId = QString("go-eCharger:%1:%2").arg(serialNumber).arg(statusMap.value("rbc").toInt());
    QString statusTopic = QString("go-eCharger/%1/status").arg(serialNumber);
    QString commandTopic = QString("go-eCharger/%1/cmd/req").arg(serialNumber);
    qCDebug(dcGoECharger()) << "Setting up mqtt channel for" << thing << address.toString() << statusTopic << commandTopic;

    MqttChannel *channel = hardwareManager()->mqttProvider()->createChannel(clientId, address, {statusTopic, commandTopic});
    if (!channel) {
        qCWarning(dcGoECharger()) << "Failed to create MQTT channel for" << thing;
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error creating MQTT channel. Please check MQTT server settings."));
        return;
    }

    m_mqttChannelsV1.insert(thing, channel);
    connect(channel, &MqttChannel::clientConnected, this, &IntegrationPluginGoECharger::onMqttClientV1Connected);
    connect(channel, &MqttChannel::clientDisconnected, this, &IntegrationPluginGoECharger::onMqttClientV1Disconnected);
    connect(channel, &MqttChannel::publishReceived, this, &IntegrationPluginGoECharger::onMqttPublishV1Received);

    // Configure the mqtt server on the go-e
    QNetworkRequest request = buildConfigurationRequestV1(address, QString("mcs=%1").arg(channel->serverAddress().toString()));
    qCDebug(dcGoECharger()) << "Configure nymea mqtt server address on" << thing << request.url().toString();
    QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [=](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcGoECharger()) << "HTTP status reply returned error:" << reply->errorString();
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The wallbox does not seem to be reachable."));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(data) << error.errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The wallbox returned invalid data."));
            return;
        }

        // Verify response matches the requsted value
        if (jsonDoc.toVariant().toMap().value("mcs").toString() != channel->serverAddress().toString()) {
            qCWarning(dcGoECharger()) << "Configured MQTT server but the response does not match with requested server address" << channel->serverAddress().toString();
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Error while configuring MQTT settings on the wallbox."));
            return;
        } else {
            qCDebug(dcGoECharger()) << "Configured successfully MQTT server" << thing << channel->serverAddress().toString();
        }

        // Configure the port
        QNetworkRequest request = buildConfigurationRequestV1(address, QString("mcp=%1").arg(channel->serverPort()));
        qCDebug(dcGoECharger()) << "Configure nymea mqtt server port on" << thing << request.url().toString();
        QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [=](){
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcGoECharger()) << "HTTP status reply returned error:" << reply->errorString();
                info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The wallbox does not seem to be reachable."));
                return;
            }

            QByteArray data = reply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(data) << error.errorString();
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The wallbox returned invalid data."));
                return;
            }

            // Verify response matches the requsted value
            if (jsonDoc.toVariant().toMap().value("mcp").toUInt() != channel->serverPort()) {
                qCWarning(dcGoECharger()) << "Configured MQTT server but the response does not match with requested server port" << channel->serverPort();
                info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Error while configuring MQTT settings on the wallbox."));
                return;
            } else {
                qCDebug(dcGoECharger()) << "Configured successfully MQTT server" << thing << channel->serverPort();
            }

            // Username
            QNetworkRequest request = buildConfigurationRequestV1(address, QString("mcu=%1").arg(channel->username()));
            qCDebug(dcGoECharger()) << "Configure nymea mqtt server user name on" << thing << request.url().toString();
            QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [=](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcGoECharger()) << "HTTP status reply returned error:" << reply->errorString();
                    info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The wallbox does not seem to be reachable."));
                    return;
                }

                QByteArray data = reply->readAll();
                QJsonParseError error;
                QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                if (error.error != QJsonParseError::NoError) {
                    qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(data) << error.errorString();
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The wallbox returned invalid data."));
                    return;
                }

                // Verify response matches the requsted value
                if (jsonDoc.toVariant().toMap().value("mcu").toString() != channel->username()) {
                    qCWarning(dcGoECharger()) << "Configured MQTT server but the response does not match with requested server username" << channel->username();
                    info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Error while configuring MQTT settings on the wallbox."));
                    return;
                } else {
                    qCDebug(dcGoECharger()) << "Configured successfully MQTT server" << thing << channel->username();
                }

                // Password
                QNetworkRequest request = buildConfigurationRequestV1(address, QString("mck=%1").arg(channel->password()));
                qCDebug(dcGoECharger()) << "Configure nymea mqtt server password on" << thing << request.url().toString();
                QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
                connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                connect(reply, &QNetworkReply::finished, info, [=](){
                    if (reply->error() != QNetworkReply::NoError) {
                        qCWarning(dcGoECharger()) << "HTTP status reply returned error:" << reply->errorString();
                        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The wallbox does not seem to be reachable."));
                        return;
                    }

                    QByteArray data = reply->readAll();
                    QJsonParseError error;
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                    if (error.error != QJsonParseError::NoError) {
                        qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(data) << error.errorString();
                        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The wallbox returned invalid data."));
                        return;
                    }

                    // Verify response matches the requsted value
                    if (jsonDoc.toVariant().toMap().value("mck").toString() != channel->password()) {
                        qCWarning(dcGoECharger()) << "Configured MQTT server but the response does not match with requested server password" << channel->password();
                        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Error while configuring MQTT settings on the wallbox."));
                        return;
                    } else {
                        qCDebug(dcGoECharger()) << "Configured successfully MQTT server" << thing << channel->password();
                    }

                    // Enable MQTT
                    QNetworkRequest request = buildConfigurationRequestV1(address, QString("mce=1"));
                    qCDebug(dcGoECharger()) << "Enable custom mqtt server on" << thing << request.url().toString();
                    QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
                    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                    connect(reply, &QNetworkReply::finished, info, [=](){
                        if (reply->error() != QNetworkReply::NoError) {
                            qCWarning(dcGoECharger()) << "HTTP status reply returned error:" << reply->errorString();
                            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The wallbox does not seem to be reachable."));
                            return;
                        }

                        QByteArray data = reply->readAll();
                        QJsonParseError error;
                        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                        if (error.error != QJsonParseError::NoError) {
                            qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(data) << error.errorString();
                            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The wallbox returned invalid data."));
                            return;
                        }

                        // Verify response matches the requsted value
                        QVariantMap statusMap = jsonDoc.toVariant().toMap();
                        if (statusMap.value("mce").toInt() != 1) {
                            qCWarning(dcGoECharger()) << "Configured MQTT server but the response does not match with requested value 1";
                            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Error while configuring MQTT settings on the wallbox."));
                            return;
                        } else {
                            qCDebug(dcGoECharger()) << "Configured successfully MQTT server enabled" << thing;
                        }

                        info->finish(Thing::ThingErrorNoError);
                        qCDebug(dcGoECharger()) << "Configuration of MQTT for" << thing << "finished successfully";
                        // Update states...
                        updateV1(thing, statusMap);
                    });
                });
            });
        });
    });
}

void IntegrationPluginGoECharger::reconfigureMqttChannelV1(Thing *thing, const QVariantMap &statusMap)
{
    QString serialNumber = thing->paramValue(goeHomeThingSerialNumberParamTypeId).toString();
    QHostAddress address = getHostAddress(thing);
    qCDebug(dcGoECharger()) << "Reconfigure mqtt channel for" << thing;

    // At least in version 30.1
    QString clientId = QString("go-eCharger:%1:%2").arg(serialNumber).arg(statusMap.value("rbc").toInt());
    QString statusTopic = QString("/go-eCharger/%1/status").arg(serialNumber);
    QString commandTopic = QString("/go-eCharger/%1/cmd/req").arg(serialNumber);

    qCDebug(dcGoECharger()) << "Setting up mqtt channel for" << thing << address.toString() << statusTopic << commandTopic;

    MqttChannel *channel = hardwareManager()->mqttProvider()->createChannel(clientId, address, {statusTopic, commandTopic});
    if (!channel) {
        qCWarning(dcGoECharger()) << "Failed to create MQTT channel for" << thing;
        return;
    }

    m_mqttChannelsV1.insert(thing, channel);
    connect(channel, &MqttChannel::clientConnected, this, &IntegrationPluginGoECharger::onMqttClientV1Connected);
    connect(channel, &MqttChannel::clientDisconnected, this, &IntegrationPluginGoECharger::onMqttClientV1Disconnected);
    connect(channel, &MqttChannel::publishReceived, this, &IntegrationPluginGoECharger::onMqttPublishV1Received);

    // Configure the mqtt server on the go-e
    QNetworkRequest request = buildConfigurationRequestV1(address, QString("mcs=%1").arg(channel->serverAddress().toString()));
    qCDebug(dcGoECharger()) << "Configure nymea mqtt server address on" << thing << request.url().toString();
    QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [=](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcGoECharger()) << "HTTP status reply returned error:" << reply->errorString();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(data) << error.errorString();
            return;
        }

        // Verify response matches the requsted value
        if (jsonDoc.toVariant().toMap().value("mcs").toString() != channel->serverAddress().toString()) {
            qCWarning(dcGoECharger()) << "Configured MQTT server but the response does not match with requested server address" << channel->serverAddress().toString();
            return;
        } else {
            qCDebug(dcGoECharger()) << "Configured successfully MQTT server" << thing << channel->serverAddress().toString();
        }

        // Configure the port
        QNetworkRequest request = buildConfigurationRequestV1(address, QString("mcp=%1").arg(channel->serverPort()));
        qCDebug(dcGoECharger()) << "Configure nymea mqtt server port on" << thing << request.url().toString();
        QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, thing, [=](){
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcGoECharger()) << "HTTP status reply returned error:" << reply->errorString();
                return;
            }

            QByteArray data = reply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(data) << error.errorString();
                return;
            }

            // Verify response matches the requsted value
            if (jsonDoc.toVariant().toMap().value("mcp").toUInt() != channel->serverPort()) {
                qCWarning(dcGoECharger()) << "Configured MQTT server but the response does not match with requested server port" << channel->serverPort();
                return;
            } else {
                qCDebug(dcGoECharger()) << "Configured successfully MQTT server" << thing << channel->serverPort();
            }

            // Username
            QNetworkRequest request = buildConfigurationRequestV1(address, QString("mcu=%1").arg(channel->username()));
            qCDebug(dcGoECharger()) << "Configure nymea mqtt server user name on" << thing << request.url().toString();
            QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, thing, [=](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcGoECharger()) << "HTTP status reply returned error:" << reply->errorString();
                    return;
                }

                QByteArray data = reply->readAll();
                QJsonParseError error;
                QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                if (error.error != QJsonParseError::NoError) {
                    qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(data) << error.errorString();
                    return;
                }

                // Verify response matches the requsted value
                if (jsonDoc.toVariant().toMap().value("mcu").toString() != channel->username()) {
                    qCWarning(dcGoECharger()) << "Configured MQTT server but the response does not match with requested server username" << channel->username();
                    return;
                } else {
                    qCDebug(dcGoECharger()) << "Configured successfully MQTT server" << thing << channel->username();
                }

                // Password
                QNetworkRequest request = buildConfigurationRequestV1(address, QString("mck=%1").arg(channel->password()));
                qCDebug(dcGoECharger()) << "Configure nymea mqtt server password on" << thing << request.url().toString();
                QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
                connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                connect(reply, &QNetworkReply::finished, thing, [=](){
                    if (reply->error() != QNetworkReply::NoError) {
                        qCWarning(dcGoECharger()) << "HTTP status reply returned error:" << reply->errorString();
                        return;
                    }

                    QByteArray data = reply->readAll();
                    QJsonParseError error;
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                    if (error.error != QJsonParseError::NoError) {
                        qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(data) << error.errorString();
                        return;
                    }

                    // Verify response matches the requsted value
                    if (jsonDoc.toVariant().toMap().value("mck").toString() != channel->password()) {
                        qCWarning(dcGoECharger()) << "Configured MQTT server but the response does not match with requested server password" << channel->password();
                        return;
                    } else {
                        qCDebug(dcGoECharger()) << "Configured successfully MQTT server" << thing << channel->password();
                    }

                    // Enable MQTT
                    QNetworkRequest request = buildConfigurationRequestV1(address, QString("mce=1"));
                    qCDebug(dcGoECharger()) << "Enable custom mqtt server on" << thing << request.url().toString();
                    QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
                    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                    connect(reply, &QNetworkReply::finished, thing, [=](){
                        if (reply->error() != QNetworkReply::NoError) {
                            qCWarning(dcGoECharger()) << "HTTP status reply returned error:" << reply->errorString();
                            return;
                        }

                        QByteArray data = reply->readAll();
                        QJsonParseError error;
                        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                        if (error.error != QJsonParseError::NoError) {
                            qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(data) << error.errorString();
                            return;
                        }

                        // Verify response matches the requsted value
                        QVariantMap statusMap = jsonDoc.toVariant().toMap();
                        if (statusMap.value("mce").toInt() != 1) {
                            qCWarning(dcGoECharger()) << "Configured MQTT server but the response does not match with requested value 1";
                            return;
                        } else {
                            qCDebug(dcGoECharger()) << "Configured successfully MQTT server enabled" << thing;
                        }

                        qCDebug(dcGoECharger()) << "Configuration of MQTT for" << thing << "finished successfully";
                        // Update states...
                        updateV1(thing, statusMap);
                    });
                });
            });
        });
    });
}

void IntegrationPluginGoECharger::updateV2(Thing *thing, const QVariantMap &statusMap)
{
    qCDebug(dcGoECharger()) << "Update V2:" << qUtf8Printable(QJsonDocument::fromVariant(statusMap).toJson());
    if (statusMap.contains("alw"))
        thing->setStateValue(goeHomePowerStateTypeId, (statusMap.value("alw").toUInt() == 0 ? false : true));

    if (statusMap.contains("car")) {
        CarState carState = static_cast<CarState>(statusMap.value("car").toUInt());
        switch (carState) {
        case CarStateReadyNoCar:
            thing->setStateValue(goeHomeCarStatusStateTypeId, "Ready but no vehicle connected");
            thing->setStateValue(goeHomePluggedInStateTypeId, false);
            break;
        case CarStateCharging:
            thing->setStateValue(goeHomeCarStatusStateTypeId, "Vehicle loads");
            thing->setStateValue(goeHomePluggedInStateTypeId, true);
            break;
        case CarStateWaitForCar:
            thing->setStateValue(goeHomeCarStatusStateTypeId, "Waiting for vehicle");
            thing->setStateValue(goeHomePluggedInStateTypeId, true);
            break;
        case CarStateChargedCarConnected:
            thing->setStateValue(goeHomeCarStatusStateTypeId, "Charging finished and vehicle still connected");
            thing->setStateValue(goeHomePluggedInStateTypeId, true);
            break;
        default:
            thing->setStateValue(goeHomeCarStatusStateTypeId, "Unknown");
            thing->setStateValue(goeHomePluggedInStateTypeId, false);
            break;
        }

        thing->setStateValue(goeHomeChargingStateTypeId, carState == CarStateCharging && thing->stateValue(goeHomePowerStateTypeId).toBool() == true);
    }

    if (statusMap.contains("ast")) {
        Access accessStatus = static_cast<Access>(statusMap.value("ast").toUInt());
        switch (accessStatus) {
        case AccessOpen:
            thing->setStateValue(goeHomeAccessStateTypeId, "Open");
            break;
        case AccessRfid:
            thing->setStateValue(goeHomeAccessStateTypeId, "RFID");
            break;
        case AccessAuto:
            thing->setStateValue(goeHomeAccessStateTypeId, "Automatic");
            break;
        }
    }

    if (statusMap.contains("tma")) {
        QVariantList temperatureSensorList = statusMap.value("tma").toList();
        if (temperatureSensorList.count() >= 1)
            thing->setStateValue(goeHomeTemperatureSensor1StateTypeId, temperatureSensorList.at(0).toDouble());

        if (temperatureSensorList.count() >= 2)
            thing->setStateValue(goeHomeTemperatureSensor2StateTypeId, temperatureSensorList.at(1).toDouble());

        if (temperatureSensorList.count() >= 3)
            thing->setStateValue(goeHomeTemperatureSensor3StateTypeId, temperatureSensorList.at(2).toDouble());

        if (temperatureSensorList.count() >= 4)
            thing->setStateValue(goeHomeTemperatureSensor4StateTypeId, temperatureSensorList.at(3).toDouble());
    }

    if (statusMap.contains("eto"))
        thing->setStateValue(goeHomeTotalEnergyConsumedStateTypeId, statusMap.value("eto").toUInt() / 1000.0); // Wh -> kWh

    if (statusMap.contains("wh"))
        thing->setStateValue(goeHomeSessionEnergyStateTypeId, statusMap.value("wh").toUInt() / 1000.0); // Wh -> kWh

    if (statusMap.contains("upd"))
        thing->setStateValue(goeHomeUpdateAvailableStateTypeId, (statusMap.value("upd").toUInt() == 0 ? false : true));

    if (statusMap.contains("fwv"))
        thing->setStateValue(goeHomeFirmwareVersionStateTypeId, statusMap.value("fwv").toString());

    // FIXME: check if we can use amx since it is better for pv charging, not all version seen implement this
    if (statusMap.contains("amp"))
        thing->setStateValue(goeHomeMaxChargingCurrentStateTypeId, statusMap.value("amp").toUInt());

    if (statusMap.contains("adi"))
        thing->setStateValue(goeHomeAdapterConnectedStateTypeId, (statusMap.value("adi").toUInt() == 0 ? false : true));

    if (statusMap.contains("fhz"))
        thing->setStateValue(goeHomeFrequencyStateTypeId, statusMap.value("fhz").toDouble());

    if (statusMap.contains("cbl"))
        thing->setStateValue(goeHomeCableType2AmpereStateTypeId, statusMap.value("cbl").toUInt());

    if (statusMap.contains("ama"))
        thing->setStateValue(goeHomeAbsoluteMaxAmpereStateTypeId, statusMap.value("ama").toUInt());

    if (statusMap.contains("var")) {
        uint variant = statusMap.value("var").toUInt();
        uint variantLimit = 16; // 11 kW
        if (variant == 22) // 22 kW
            variantLimit = 32;

        thing->setStateValue(goeHomeModelMaxAmpereStateTypeId, variantLimit);
    }

    if (statusMap.contains("cbl") || statusMap.contains("ama")) {
        // Check charging limits
        uint amaLimit = thing->stateValue(goeHomeAbsoluteMaxAmpereStateTypeId).toUInt();
        uint cableLimit = thing->stateValue(goeHomeCableType2AmpereStateTypeId).toUInt();
        uint modelLimit = thing->stateValue(goeHomeModelMaxAmpereStateTypeId).toUInt();

        uint finalLimit = 0;
        if (cableLimit != 0) {
            finalLimit = qMin(amaLimit, cableLimit);
        } else {
            finalLimit = amaLimit;
        }
        // Check hardware variant: 11 -> 16A and 22 -> 32A
        if (modelLimit != 0)
            finalLimit = qMin(finalLimit, modelLimit);

        thing->setStateMaxValue(goeHomeMaxChargingCurrentStateTypeId, finalLimit);
    }

    if (statusMap.contains("pnp") && statusMap.value("pnp").toUInt() != 0)
        thing->setStateValue(goeHomePhaseCountStateTypeId, statusMap.value("pnp").toUInt());

    if (statusMap.contains("nrg")) {
        // Parse nrg array
        uint voltagePhaseA = 0; uint voltagePhaseB = 0; uint voltagePhaseC = 0;
        double amperePhaseA = 0; double amperePhaseB = 0; double amperePhaseC = 0;
        double currentPower = 0; double powerPhaseA = 0; double powerPhaseB = 0; double powerPhaseC = 0;
        QVariantList measurementList = statusMap.value("nrg").toList();

        if (measurementList.count() >= 1)
            voltagePhaseA = measurementList.at(0).toDouble();

        if (measurementList.count() >= 2)
            voltagePhaseB = measurementList.at(1).toDouble();

        if (measurementList.count() >= 3)
            voltagePhaseC = measurementList.at(2).toDouble();

        if (measurementList.count() >= 5)
            amperePhaseA = measurementList.at(4).toDouble();

        if (measurementList.count() >= 6)
            amperePhaseB = measurementList.at(5).toDouble();

        if (measurementList.count() >= 7)
            amperePhaseC = measurementList.at(6).toDouble();

        if (measurementList.count() >= 8)
            powerPhaseA = measurementList.at(7).toDouble();
        if (measurementList.count() >= 9)
            powerPhaseB = measurementList.at(8).toDouble() ;

        if (measurementList.count() >= 10)
            powerPhaseC = measurementList.at(9).toDouble();

        if (measurementList.count() >= 12)
            currentPower = measurementList.at(11).toDouble();

        // Update all states
        thing->setStateValue(goeHomeVoltagePhaseAStateTypeId, voltagePhaseA);
        thing->setStateValue(goeHomeVoltagePhaseBStateTypeId, voltagePhaseB);
        thing->setStateValue(goeHomeVoltagePhaseCStateTypeId, voltagePhaseC);
        thing->setStateValue(goeHomeCurrentPhaseAStateTypeId, amperePhaseA);
        thing->setStateValue(goeHomeCurrentPhaseBStateTypeId, amperePhaseB);
        thing->setStateValue(goeHomeCurrentPhaseCStateTypeId, amperePhaseC);
        thing->setStateValue(goeHomeCurrentPowerPhaseAStateTypeId, powerPhaseA);
        thing->setStateValue(goeHomeCurrentPowerPhaseBStateTypeId, powerPhaseB);
        thing->setStateValue(goeHomeCurrentPowerPhaseCStateTypeId, powerPhaseC);

        thing->setStateValue(goeHomeCurrentPowerStateTypeId, currentPower);

        // Check how many phases are actually charging, and update the phase count only if something happens on the phases (current or power)
        if (amperePhaseA != 0 || amperePhaseB != 0 || amperePhaseC != 0) {
            uint phaseCount = 0;
            if (amperePhaseA != 0)
                phaseCount += 1;

            if (amperePhaseB != 0)
                phaseCount += 1;

            if (amperePhaseC != 0)
                phaseCount += 1;

            // Use this loginc only if we don't have pnp available
            if (!statusMap.contains("pnp") || statusMap.value("pnp").toUInt() == 0) {
                thing->setStateValue(goeHomePhaseCountStateTypeId, phaseCount);
            }
        }
    }
}

QNetworkRequest IntegrationPluginGoECharger::buildConfigurationRequestV2(const QHostAddress &address, const QUrlQuery &configuration)
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(address.toString());
    requestUrl.setPath("/api/set");
    requestUrl.setQuery(configuration);
    return QNetworkRequest(requestUrl);
}

void IntegrationPluginGoECharger::setupMqttChannelV2(ThingSetupInfo *info, const QHostAddress &address, const QVariantMap &statusMap)
{
    Thing *thing = info->thing();

    QString serialNumber = thing->paramValue(goeHomeThingSerialNumberParamTypeId).toString();

    // At least in version 51.1
    QString clientId = QString("go-echarger_%1").arg(serialNumber);
    QString statusTopic = QString("/go-eCharger/%1/#").arg(serialNumber);
    qCDebug(dcGoECharger()) << "Setting up mqtt channel for" << thing << address.toString() << statusTopic;

    // Somehow limited to 8 characters...
    QString username = QString::fromUtf8(QUuid::createUuid().toByteArray().toHex().left(8));
    QString password = QString::fromUtf8(QUuid::createUuid().toByteArray().toHex().left(8));

    MqttChannel *channel = hardwareManager()->mqttProvider()->createChannel(clientId, username, password, address, {statusTopic});
    if (!channel) {
        qCWarning(dcGoECharger()) << "Failed to create MQTT channel for" << thing;
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error creating MQTT channel. Please check MQTT server settings."));
        return;
    }

    m_mqttChannelsV2.insert(thing, channel);
    connect(channel, &MqttChannel::clientConnected, this, &IntegrationPluginGoECharger::onMqttClientV2Connected);
    connect(channel, &MqttChannel::clientDisconnected, this, &IntegrationPluginGoECharger::onMqttClientV2Disconnected);

    // Build the mqtt url
    QUrl mqttUrl;
    mqttUrl.setScheme("mqtt");
    mqttUrl.setHost(channel->serverAddress().toString());
    mqttUrl.setPort(channel->serverPort());
    mqttUrl.setUserName(channel->username());
    mqttUrl.setPassword(channel->password());

    // The query item must be JSON encoded, meaning: strings need quouts... for whatever reason...
    QUrlQuery query;
    query.addQueryItem("mcu", "\"" + mqttUrl.toString() + "\"");
    query.addQueryItem("mce", "true");

    QNetworkRequest request = buildConfigurationRequestV2(address, query);
    qCDebug(dcGoECharger()) << "Configure nymea mqtt server address on" << thing << request.url().toString();
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [=](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcGoECharger()) << "Failed to set MQTT config for" << thing->name() << reply->errorString() << reply->readAll() << "Request was:" << request.url().toString();
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The wallbox does not seem to be reachable."));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcGoECharger()) << "Failed to parse MQTT config reply from" << thing->name() << qUtf8Printable(data) << error.errorString() << "Request was:" << request.url().toString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The wallbox returned invalid data."));
            return;
        }

        qCDebug(dcGoECharger()) << qUtf8Printable(jsonDoc.toJson());

        info->finish(Thing::ThingErrorNoError);
        qCDebug(dcGoECharger()) << "Configuration of MQTT for" << thing << "finished successfully";
        // Update states...
        updateV2(thing, statusMap);

        // From now on we listen to topics
        connect(channel, &MqttChannel::publishReceived, thing, [=](MqttChannel* channel, const QString &topic, const QByteArray &payload){
            QString propertyKey = topic.split("/").last();
            QString propertyValue = QString::fromUtf8(payload);
            // Well...no better idea for now to keep the APIs / parsing methods
            // compatible trought different APIs and protocols
            QString statusMapJsonString = QString("{\"%1\":%2}").arg(propertyKey).arg(propertyValue);
            QJsonDocument jsonDoc = QJsonDocument::fromJson(statusMapJsonString.toUtf8());

            // Mute the spaming stuff..
            if (propertyKey != "fhz" && propertyKey != "rssi" && propertyKey != "utc" && propertyKey != "loc" && propertyKey != "rbt") {
                qCDebug(dcGoECharger()) << thing->name() << channel->clientId() << "publish received" << topic << payload;
                //qCDebug(dcGoECharger()) << "-->" << jsonDoc.toJson(QJsonDocument::Compact);
            }
            updateV2(thing, jsonDoc.toVariant().toMap());
        });
    });
}

void IntegrationPluginGoECharger::reconfigureMqttChannelV2(Thing *thing)
{
    QString serialNumber = thing->paramValue(goeHomeThingSerialNumberParamTypeId).toString();
    QHostAddress address = getHostAddress(thing);

    qCDebug(dcGoECharger()) << "Reconfigure mqtt channel for" << thing;
    // At least in version 51.1
    QString clientId = QString("go-echarger_%1").arg(serialNumber);
    QString statusTopic = QString("/go-eCharger/%1/#").arg(serialNumber);
    qCDebug(dcGoECharger()) << "Reconfigure mqtt channel for" << thing << address.toString() << statusTopic;

    // Somehow limited to 8 characters...
    QString username = QString::fromUtf8(QUuid::createUuid().toByteArray().toHex().left(8));
    QString password = QString::fromUtf8(QUuid::createUuid().toByteArray().toHex().left(8));

    MqttChannel *channel = hardwareManager()->mqttProvider()->createChannel(clientId, username, password, address, {statusTopic});
    if (!channel) {
        qCWarning(dcGoECharger()) << "Failed to create MQTT channel for" << thing;
        return;
    }

    // Clean up existing channel if there is one
    if (m_mqttChannelsV2.contains(thing)) {
        qCDebug(dcGoECharger()) << "Release old mqtt channel...";
        hardwareManager()->mqttProvider()->releaseChannel(m_mqttChannelsV2.take(thing));
    }

    m_mqttChannelsV2.insert(thing, channel);
    connect(channel, &MqttChannel::clientConnected, this, &IntegrationPluginGoECharger::onMqttClientV2Connected);
    connect(channel, &MqttChannel::clientDisconnected, this, &IntegrationPluginGoECharger::onMqttClientV2Disconnected);

    // Build the mqtt url
    QUrl mqttUrl;
    mqttUrl.setScheme("mqtt");
    mqttUrl.setHost(channel->serverAddress().toString());
    mqttUrl.setPort(channel->serverPort());
    mqttUrl.setUserName(channel->username());
    mqttUrl.setPassword(channel->password());

    // The query item must be JSON encoded, meaning: strings need quouts... for whatever reason...
    QUrlQuery query;
    query.addQueryItem("mcu", "\"" + mqttUrl.toString() + "\"");

    QNetworkRequest request = buildConfigurationRequestV2(address, query);
    qCDebug(dcGoECharger()) << "Configure nymea mqtt server address on" << thing << request.url().toString();
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [=](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcGoECharger()) << "Configuring MQTT for" << thing->name() << "failed:" << reply->errorString() << reply->readAll() << "Request was:" << request.url().toString();
            hardwareManager()->mqttProvider()->releaseChannel(m_mqttChannelsV2.take(thing));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcGoECharger()) << "Failed to parse MQTT config reply for" << thing->name() << qUtf8Printable(data) << error.errorString() << "Request was:" << request.url().toString();
            hardwareManager()->mqttProvider()->releaseChannel(m_mqttChannelsV2.take(thing));
            return;
        }

        QVariantMap responseCode = jsonDoc.toVariant().toMap();
        if (!responseCode.value("mcu", false).toBool()) {
            qCWarning(dcGoECharger()) << "Failed to configure mqtt on" << thing;
            hardwareManager()->mqttProvider()->releaseChannel(m_mqttChannelsV2.take(thing));
            return;
        }

        qCDebug(dcGoECharger()) << "Configuration of MQTT for" << thing << "finished successfully";
        // From now on we listen to topics
        connect(channel, &MqttChannel::publishReceived, thing, [=](MqttChannel* channel, const QString &topic, const QByteArray &payload){
            QString propertyKey = topic.split("/").last();
            QString propertyValue = QString::fromUtf8(payload);
            // Well...no better idea for now to keep the APIs / parsing methods
            // compatible trought different APIs and protocols
            QString statusMapJsonString = QString("{\"%1\":%2}").arg(propertyKey).arg(propertyValue);
            QJsonDocument jsonDoc = QJsonDocument::fromJson(statusMapJsonString.toUtf8());

            // Mute the spaming stuff..
            if (propertyKey != "fhz" && propertyKey != "rssi" && propertyKey != "utc" && propertyKey != "loc" && propertyKey != "rbt") {
                qCDebug(dcGoECharger()) << thing->name() << channel->clientId() << "publish received" << topic << payload;
                //qCDebug(dcGoECharger()) << "-->" << jsonDoc.toJson(QJsonDocument::Compact);
            }
            updateV2(thing, jsonDoc.toVariant().toMap());
        });

    });
}

void IntegrationPluginGoECharger::refreshHttp()
{
    // Update all things which don't use mqtt
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() != goeHomeThingClassId) {
            continue;
        }

        // Poll thing which if not using mqtt
        if (thing->paramValue(goeHomeThingUseMqttParamTypeId).toBool()) {
            continue;
        }

        // Make sure there is not a request pending for this thing, otherwise wait for the next refresh
        if (m_pendingReplies.contains(thing) && m_pendingReplies.value(thing)) {
            continue;
        }

        QNetworkRequest request = buildStatusRequest(thing);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        m_pendingReplies.insert(thing, reply);

        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, thing, [=](){
            // We are done with this thing reply
            m_pendingReplies.remove(thing);

            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcGoECharger()) << "HTTP status reply error for thing" << thing->name() << reply->errorString() << "Request was:" << request.url().toString();
                markAsDisconnected(thing);
                return;
            }

            QByteArray data = reply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcGoECharger()) << "Failed to parse status data for thing" << thing->name() << qUtf8Printable(data) << error.errorString() << "Request was:" << request.url().toString();
                markAsDisconnected(thing);
                return;
            }

            ApiVersion apiVersion = getApiVersion(thing);
            // Valid json data received, connected true
            thing->setStateValue("connected", true);

            //qCDebug(dcGoECharger()) << "Received" << qUtf8Printable(jsonDoc.toJson());
            QVariantMap statusMap = jsonDoc.toVariant().toMap();
            switch (apiVersion) {
            case ApiVersion1:
                updateV1(thing, statusMap);
                break;
            case ApiVersion2:
                updateV2(thing, statusMap);
                break;
            }
        });
    }
}


void IntegrationPluginGoECharger::onMqttClientV1Connected(MqttChannel *channel)
{
    Thing *thing = m_mqttChannelsV1.key(channel);
    if (!thing) {
        qCWarning(dcGoECharger()) << "Received a client connect for an unknown thing. Ignoring the event.";
        return;
    }

    qCDebug(dcGoECharger()) << thing << "connected";
    thing->setStateValue("connected", true);
}

void IntegrationPluginGoECharger::onMqttClientV1Disconnected(MqttChannel *channel)
{
    Thing *thing = m_mqttChannelsV1.key(channel);
    if (!thing) {
        qCWarning(dcGoECharger()) << "Received a client disconnect for an unknown thing. Ignoring the event.";
        return;
    }

    qCDebug(dcGoECharger()) << thing << "connected";
    markAsDisconnected(thing);
}

void IntegrationPluginGoECharger::onMqttPublishV1Received(MqttChannel *channel, const QString &topic, const QByteArray &payload)
{
    Thing *thing = m_mqttChannelsV1.key(channel);
    if (!thing) {
        qCWarning(dcGoECharger()) << "Received a MQTT client publish from an unknown thing. Ignoring the event.";
        return;
    }

    qCDebug(dcGoECharger()) << thing << "publish received" << topic;
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcGoECharger()) << "Failed to parse MQTT status data for thing" << thing->name() << "on topic" << topic << error.errorString() << qUtf8Printable(payload);
        return;
    }

    QString serialNumber = thing->paramValue(goeHomeThingSerialNumberParamTypeId).toString();
    if (topic == QString("go-eCharger/%1/status").arg(serialNumber)) {
        updateV1(thing, jsonDoc.toVariant().toMap());
    } else {
        qCDebug(dcGoECharger()) << "Unhandled MQTT topic publish received for thing" << thing->name() << "on topic" << topic << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Compact));
    }
}

void IntegrationPluginGoECharger::onMqttClientV2Connected(MqttChannel *channel)
{
    Thing *thing = m_mqttChannelsV2.key(channel);
    if (!thing) {
        qCWarning(dcGoECharger()) << "Received a client disconnect for an unknown thing. Ignoring the event.";
        return;
    }

    qCDebug(dcGoECharger()) << thing << "connected";
    thing->setStateValue("connected", true);
}

void IntegrationPluginGoECharger::onMqttClientV2Disconnected(MqttChannel *channel)
{
    Thing *thing = m_mqttChannelsV1.key(channel);
    if (!thing) {
        qCWarning(dcGoECharger()) << "Received a client disconnect for an unknown thing. Ignoring the event.";
        return;
    }

    qCDebug(dcGoECharger()) << thing << "connected";
    markAsDisconnected(thing);
}

void IntegrationPluginGoECharger::markAsDisconnected(Thing *thing)
{
    qCDebug(dcGoECharger()) << "Mark device as disconnected" << thing;
    thing->setStateValue("connected", false);
    thing->setStateValue("currentPower", 0);
    thing->setStateValue("voltagePhaseA", 0);
    thing->setStateValue("voltagePhaseB", 0);
    thing->setStateValue("voltagePhaseC", 0);
    thing->setStateValue("currentPhaseA", 0);
    thing->setStateValue("currentPhaseB", 0);
    thing->setStateValue("currentPhaseC", 0);
    thing->setStateValue("currentPowerPhaseA", 0);
    thing->setStateValue("currentPowerPhaseB", 0);
    thing->setStateValue("currentPowerPhaseC", 0);
    thing->setStateValue("frequency", 0);
}
