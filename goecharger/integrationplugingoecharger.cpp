/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
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

// API documentation: https://github.com/goecharger/go-eCharger-API-v1

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

    // Perform a network device discovery and filter for "go-eCharger" hosts
    NetworkDeviceDiscoveryReply *discoveryReply = hardwareManager()->networkDeviceDiscovery()->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        foreach (const NetworkDevice &networkDevice, discoveryReply->networkDevices()) {

            qCDebug(dcGoECharger()) << "Found" << networkDevice;
            if (!networkDevice.hostName().toLower().contains("go-echarger"))
                continue;

            QString title;
            if (networkDevice.hostName().isEmpty()) {
                title = networkDevice.address().toString();
            } else {
                title = "go-eCharger (" + networkDevice.address().toString() + ")";
            }

            QString description;
            if (networkDevice.macAddressManufacturer().isEmpty()) {
                description = networkDevice.macAddress();
            } else {
                description = networkDevice.macAddress() + " (" + networkDevice.macAddressManufacturer() + ")";
            }

            ThingDescriptor descriptor(goeHomeThingClassId, title, description);
            ParamList params;
            params << Param(goeHomeThingIpAddressParamTypeId, networkDevice.address().toString());
            params << Param(goeHomeThingMacAddressParamTypeId, networkDevice.macAddress());
            descriptor.setParams(params);

            // Check if we already have set up this device
            Things existingThings = myThings().filterByParam(goeHomeThingMacAddressParamTypeId, networkDevice.macAddress());
            if (existingThings.count() == 1) {
                qCDebug(dcGoECharger()) << "This go-eCharger already exists in the system!" << networkDevice;
                descriptor.setThingId(existingThings.first()->id());
            }

            info->addThingDescriptor(descriptor);
        }

        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginGoECharger::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == goeHomeThingClassId) {
        QHostAddress address = QHostAddress(thing->paramValue(goeHomeThingIpAddressParamTypeId).toString());
        QUrl requestUrl;
        requestUrl.setScheme("http");
        requestUrl.setHost(address.toString());
        requestUrl.setPath("/status");

        QNetworkRequest request(requestUrl);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, thing, [=](){
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

            qCDebug(dcGoECharger()) << "Received" << qUtf8Printable(jsonDoc.toJson());
            // Verify mqtt client and set it up
            setupMqttChannel(info, address, jsonDoc.toVariant().toMap());
        });
        return;
    }

    Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
}


void IntegrationPluginGoECharger::thingRemoved(Thing *thing)
{
    if (m_channels.contains(thing)) {
        hardwareManager()->mqttProvider()->releaseChannel(m_channels.take(thing));
    }
}

void IntegrationPluginGoECharger::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() != goeHomeThingClassId) {
        info->finish(Thing::ThingErrorThingClassNotFound);
        return;
    }

    if (thing->stateValue(goeHomeConnectedStateTypeId).toBool()) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    if (thing->stateValue(goeHomeSerialNumberStateTypeId).toString().isEmpty()) {
        qCDebug(dcGoECharger()) << "Could not execute action because the serial number is missing.";
        info->finish(Thing::ThingErrorHardwareFailure);
        return;
    }

    if (action.actionTypeId() == goeHomePowerActionTypeId) {
        bool power = action.paramValue(goeHomePowerActionPowerParamTypeId).toBool();
        qCDebug(dcGoECharger()) << "Setting charging allowed to" << power;
        // Set the allow value
        QString configuration = QString("alw=%1").arg(power ? 1: 0);
        sendActionRequest(thing, info, configuration);
        return;
    } else if (action.actionTypeId() == goeHomeMaxChargingCurrentActionTypeId) {
        int maxChargingCurrent = action.paramValue(goeHomeMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt();
        qCDebug(dcGoECharger()) << "Setting max charging current to" << maxChargingCurrent << "A";
        // Set the allow value
        QString configuration = QString("ama=%1").arg(maxChargingCurrent);
        sendActionRequest(thing, info, configuration);
        return;
    } else if (action.actionTypeId() == goeHomeCloudActionTypeId) {
        bool enabled = action.paramValue(goeHomeCloudActionCloudParamTypeId).toBool();
        qCDebug(dcGoECharger()) << "Set cloud" << (enabled ? "enabled" : "disabled");
        // Set the allow value
        QString configuration = QString("cdi=%1").arg(enabled ? 1: 0);
        sendActionRequest(thing, info, configuration);
        return;
    } else if (action.actionTypeId() == goeHomeLedBrightnessActionTypeId) {
        quint8 brightness = action.paramValue(goeHomeLedBrightnessActionLedBrightnessParamTypeId).toUInt();
        qCDebug(dcGoECharger()) << "Set led brightnss to" << brightness << "/" << 255;
        // Set the allow value
        QString configuration = QString("lbr=%1").arg(brightness);
        sendActionRequest(thing, info, configuration);
        return;
    } else if (action.actionTypeId() == goeHomeLedEnergySaveActionTypeId) {
        bool enabled = action.paramValue(goeHomeLedEnergySaveActionLedEnergySaveParamTypeId).toBool();
        qCDebug(dcGoECharger()) << "Set led energy saving" << (enabled ? "enabled" : "disabled");
        // Set the allow value
        QString configuration = QString("lse=%1").arg(enabled ? 1: 0);
        sendActionRequest(thing, info, configuration);
        return;
    } else {
        info->finish(Thing::ThingErrorActionTypeNotFound);
    }
}

void IntegrationPluginGoECharger::onClientConnected(MqttChannel *channel)
{
    Thing *thing = m_channels.key(channel);
    if (!thing) {
        qCWarning(dcGoECharger()) << "Received a client connect for an unknown thing. Ignoring the event.";
        return;
    }

    qCDebug(dcGoECharger()) << thing << "connected";
    thing->setStateValue(goeHomeConnectedStateTypeId, true);
}

void IntegrationPluginGoECharger::onClientDisconnected(MqttChannel *channel)
{
    Thing *thing = m_channels.key(channel);
    if (!thing) {
        qCWarning(dcGoECharger()) << "Received a client disconnect for an unknown thing. Ignoring the event.";
        return;
    }

    qCDebug(dcGoECharger()) << thing << "connected";
    thing->setStateValue(goeHomeConnectedStateTypeId, false);
}

void IntegrationPluginGoECharger::onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload)
{
    Thing *thing = m_channels.key(channel);
    if (!thing) {
        qCWarning(dcGoECharger()) << "Received a MQTT client publish from an unknown thing. Ignoring the event.";
        return;
    }

    qCDebug(dcGoECharger()) << thing << "publish received" << topic;
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcGoECharger()) << "Failed to parse status data for thing " << thing->name() << qUtf8Printable(payload) << error.errorString();
        return;
    }

    QString serialNumber = thing->stateValue(goeHomeSerialNumberStateTypeId).toString();
    if (topic == QString("go-eCharger/%1/status").arg(serialNumber)) {
        update(thing, jsonDoc.toVariant().toMap());
    }
}

void IntegrationPluginGoECharger::update(Thing *thing, const QVariantMap &statusMap)
{
    if (thing->thingClassId() == goeHomeThingClassId) {
        // Parse status map and update states...
        CarState carState = static_cast<CarState>(statusMap.value("car").toUInt());
        switch (carState) {
        case CarStateReadyNoCar:
            thing->setStateValue(goeHomeCarStatusStateTypeId, "Ready but no vehicle connected");
            break;
        case CarStateCharging:
            thing->setStateValue(goeHomeCarStatusStateTypeId, "Vehicle loads");
            break;
        case CarStateWaitForCar:
            thing->setStateValue(goeHomeCarStatusStateTypeId, "Waiting for vehicle");
            break;
        case CarStateChargedCarConnected:
            thing->setStateValue(goeHomeCarStatusStateTypeId, "Charging finished and vehicle still connected");
            break;
        }

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
        if (temperatureSensorList.count() == 4) {
            thing->setStateValue(goeHomeTemperatureSensor1StateTypeId, temperatureSensorList.at(0).toDouble());
            thing->setStateValue(goeHomeTemperatureSensor2StateTypeId, temperatureSensorList.at(1).toDouble());
            thing->setStateValue(goeHomeTemperatureSensor3StateTypeId, temperatureSensorList.at(2).toDouble());
            thing->setStateValue(goeHomeTemperatureSensor4StateTypeId, temperatureSensorList.at(3).toDouble());
        }

        thing->setStateValue(goeHomeTotalEnergyConsumedStateTypeId, statusMap.value("eto").toUInt() / 10.0);
        thing->setStateValue(goeHomeChargeEnergyStateTypeId, statusMap.value("dws").toUInt() / 360000.0);
        thing->setStateValue(goeHomePowerStateTypeId, (statusMap.value("alw").toUInt() == 0 ? false : true));
        thing->setStateValue(goeHomeUpdateAvailableStateTypeId, (statusMap.value("upd").toUInt() == 0 ? false : true));
        thing->setStateValue(goeHomeAdapterConnectedStateTypeId, (statusMap.value("adi").toUInt() == 0 ? false : true));
        thing->setStateValue(goeHomeCloudStateTypeId, (statusMap.value("cdi").toUInt() == 0 ? false : true));
        thing->setStateValue(goeHomeFirmwareVersionStateTypeId, statusMap.value("fwv").toString());
        thing->setStateValue(goeHomeMaxChargingCurrentStateTypeId, statusMap.value("ama").toUInt());
        thing->setStateValue(goeHomeLedBrightnessStateTypeId, statusMap.value("lbr").toUInt());
        thing->setStateValue(goeHomeLedEnergySaveStateTypeId, statusMap.value("lse").toBool());
        thing->setStateValue(goeHomeSerialNumberStateTypeId, statusMap.value("sse").toString());
    }
}

QNetworkRequest IntegrationPluginGoECharger::buildConfigurationRequest(const QHostAddress &address, const QString &configuration)
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

void IntegrationPluginGoECharger::sendActionRequest(Thing *thing, ThingActionInfo *info, const QString &configuration)
{
    // Lets use rest here since we get a reply on the rest request. For using MQTT publish to topic "go-eCharger/<serialnumber>/cmd/req"
    QNetworkRequest request = buildConfigurationRequest(QHostAddress(thing->paramValue(goeHomeThingIpAddressParamTypeId).toString()), configuration);
    QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
    connect(reply, &QNetworkReply::finished, thing, [=](){
        reply->deleteLater();
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

        qCDebug(dcGoECharger()) << "Action response" << jsonDoc.toJson(QJsonDocument::Compact);
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginGoECharger::setupMqttChannel(ThingSetupInfo *info, const QHostAddress &address, const QVariantMap &statusMap)
{
    Thing *thing = info->thing();
    QString serialNumber = statusMap.value("sse").toString();
    QString clientId = QString("go-eCharger:%1:%2").arg(serialNumber).arg(statusMap.value("rbc").toInt());
    QString statusTopic = QString("go-eCharger/%1/status").arg(serialNumber);
    qCDebug(dcGoECharger()) << "Setting up mqtt channel for" << thing << address.toString() << statusTopic;

    MqttChannel *channel = hardwareManager()->mqttProvider()->createChannel(clientId, address, {statusTopic});
    if (!channel) {
        qCWarning(dcGoECharger()) << "Failed to create MQTT channel for" << thing;
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error creating MQTT channel. Please check MQTT server settings."));
        return;
    }

    m_channels.insert(thing, channel);
    connect(channel, &MqttChannel::clientConnected, this, &IntegrationPluginGoECharger::onClientConnected);
    connect(channel, &MqttChannel::clientDisconnected, this, &IntegrationPluginGoECharger::onClientDisconnected);
    connect(channel, &MqttChannel::publishReceived, this, &IntegrationPluginGoECharger::onPublishReceived);

    // Configure the mqtt server on the go-e
    QNetworkRequest request = buildConfigurationRequest(address, QString("mcs=%1").arg(channel->serverAddress().toString()));
    qCDebug(dcGoECharger()) << "Configure nymea mqtt server address on" << thing << request.url().toString();
    QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
    connect(reply, &QNetworkReply::finished, thing, [=](){
        reply->deleteLater();
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

        QNetworkRequest request = buildConfigurationRequest(address, QString("mcp=%1").arg(channel->serverPort()));
        qCDebug(dcGoECharger()) << "Configure nymea mqtt server port on" << thing << request.url().toString();
        QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
        connect(reply, &QNetworkReply::finished, thing, [=](){
            reply->deleteLater();
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

            QNetworkRequest request = buildConfigurationRequest(address, QString("mcu=%1").arg(channel->username()));
            qCDebug(dcGoECharger()) << "Configure nymea mqtt server user name on" << thing << request.url().toString();
            QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
            connect(reply, &QNetworkReply::finished, thing, [=](){
                reply->deleteLater();
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

                QNetworkRequest request = buildConfigurationRequest(address, QString("mck=%1").arg(channel->password()));
                qCDebug(dcGoECharger()) << "Configure nymea mqtt server password on" << thing << request.url().toString();
                QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
                connect(reply, &QNetworkReply::finished, thing, [=](){
                    reply->deleteLater();
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

                    QNetworkRequest request = buildConfigurationRequest(address, QString("mce=1"));
                    qCDebug(dcGoECharger()) << "Enable custom mqtt server on" << thing << request.url().toString();
                    QNetworkReply *reply = hardwareManager()->networkManager()->sendCustomRequest(request, "SET");
                    connect(reply, &QNetworkReply::finished, thing, [=](){
                        reply->deleteLater();
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
                        update(thing, statusMap);
                    });
                });
            });
        });
    });
}


