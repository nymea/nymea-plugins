// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationpluginopenccu.h"
#include "plugininfo.h"

#include <integrations/thing.h>
#include <network/networkaccessmanager.h>

#include <QUrl>
#include <QUrlQuery>
#include <QHostAddress>

#include <QXmlStreamReader>


IntegrationPluginOpenCCU::IntegrationPluginOpenCCU()
{
}

void IntegrationPluginOpenCCU::init()
{
}


void IntegrationPluginOpenCCU::discoverThings(ThingDiscoveryInfo *info)
{
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginOpenCCU::setupThing(ThingSetupInfo *info)
{
    if (info->thing()->thingClassId() == thermostatThingClassId) {
        m_thermostats.insert(info->thing(), Thermostat());
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginOpenCCU::postSetupThing(Thing *thing)
{
    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this](){
            // Refresh thermostats
            foreach (Thing *gatewayThing, myThings().filterByThingClassId(openCCUThingClassId)) {
                qCDebug(dcOpenCCU()) << "Refresh" << gatewayThing;
                getStateList(gatewayThing);
            }
        });

        m_pluginTimer->start();
    }

    if (thing->thingClassId() == openCCUThingClassId) {
        // Sync devices
        getDevices(thing);
    } else if (thing->thingClassId() == floorHeatingControllerThingClassId) {
        // Sync channels
        getChannels(thing);
    }
}


void IntegrationPluginOpenCCU::thingRemoved(Thing *thing)
{
    if (m_thermostats.contains(thing))
        m_thermostats.remove(thing);

    if (myThings().isEmpty() && m_pluginTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginOpenCCU::executeAction(ThingActionInfo *info)
{
    if (info->thing()->thingClassId() == thermostatThingClassId) {

        // Get the parent thing for the URL
        Thing *gatewayThing = myThings().findById(info->thing()->parentId());

        QUrlQuery query;
        if (info->action().actionTypeId() == thermostatModeStateTypeId) {

            int iseId = m_thermostats.value(info->thing()).channels.value(1).controlModeId;
            if (iseId < 0) {
                // A device which has to be controlled using the mode id, not the control mode id...
                iseId = m_thermostats.value(info->thing()).channels.value(1).modeId;
            }

            if (iseId < 0) {
                qCWarning(dcOpenCCU()) << "The ise ID of the mode is not known yet.";
                info->finish(Thing::ThingErrorHardwareNotAvailable);
                return;
            }

            query.addQueryItem("ise_id", QString::number(iseId));
            if (info->action().paramValue(thermostatModeActionModeParamTypeId).toString() == "Auto") {
                query.addQueryItem("new_value", QString::number(0));
            } else {
                query.addQueryItem("new_value", QString::number(1));
            }

            QUrl requestUrl = buildUrl(gatewayThing, "statechange.cgi", query);
            qCDebug(dcOpenCCU()) << requestUrl.toString();
            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(requestUrl));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::sslErrors, this, &IntegrationPluginOpenCCU::onSslError);
            connect(reply, &QNetworkReply::finished, this, [info, reply](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCDebug(dcOpenCCU()) << "Execute action finished with error" << reply->errorString();
                    info->finish(Thing::ThingErrorHardwareFailure);
                    return;
                }

                qCDebug(dcOpenCCU()) << "Execute action finished successfully" << reply->readAll();
                info->thing()->setStateValue(thermostatModeStateTypeId,
                                             info->action().paramValue(thermostatModeActionModeParamTypeId).toString());

                info->finish(Thing::ThingErrorNoError);
            });

        } else if (info->action().actionTypeId() == thermostatTargetTemperatureActionTypeId) {
            int iseId = m_thermostats.value(info->thing()).channels.value(1).targetTemperatureId;
            if (iseId < 0) {
                qCWarning(dcOpenCCU()) << "The ise ID of the target temperature is not known yet.";
                info->finish(Thing::ThingErrorHardwareNotAvailable);
                return;
            }

            double value = info->action().paramValue(thermostatTargetTemperatureActionTargetTemperatureParamTypeId).toDouble();
            value = qRound(value * 2) / 2.0;
            qCDebug(dcOpenCCU()) << "Setting target temperature of" << info->thing()->name() << "to" << value << "°C";
            query.addQueryItem("ise_id", QString::number(iseId));
            query.addQueryItem("new_value", QString::number(value));

            QUrl requestUrl = buildUrl(gatewayThing, "statechange.cgi", query);
            qCDebug(dcOpenCCU()) << requestUrl.toString();
            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(requestUrl));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::sslErrors, this, &IntegrationPluginOpenCCU::onSslError);
            connect(reply, &QNetworkReply::finished, this, [info, reply, value](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCDebug(dcOpenCCU()) << "Execute action finished with error" << reply->errorString();
                    info->finish(Thing::ThingErrorHardwareFailure);
                    return;
                }

                qCDebug(dcOpenCCU()) << "Execute action finished successfully" << reply->readAll();
                info->thing()->setStateValue(thermostatTargetTemperatureStateTypeId, value);
                info->finish(Thing::ThingErrorNoError);
            });
        }
    }
}

void IntegrationPluginOpenCCU::getDevices(Thing *thing)
{
    QUrl url = buildUrl(thing,  "devicelist.cgi");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::sslErrors, this, &IntegrationPluginOpenCCU::onSslError);
    connect(reply, &QNetworkReply::finished, this, [this, thing, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(dcOpenCCU()) << "Reply finished with error" << reply->errorString();
            return;
        }

        thing->setStateValue(openCCUConnectedStateTypeId, true);

        QByteArray data = reply->readAll();
        //qCDebug(dcOpenCCU()) << "-->" << data;

        ThingDescriptors descriptors;
        QString deviceName;
        QXmlStreamReader xml(data);
        while(!xml.atEnd() && !xml.hasError()) {
            xml.readNext();
            if (xml.name() == QString("device") && xml.isStartElement())  {
                qCDebug(dcOpenCCU()) << "-->" << xml.name() << deviceName;
                foreach (const QXmlStreamAttribute &attribute, xml.attributes()) {
                    qCDebug(dcOpenCCU()) << "    " << attribute.name() << attribute.value();
                }

                deviceName = xml.attributes().value("name").toString();
                QString deviceType = xml.attributes().value("device_type").toString();
                int iseId = xml.attributes().value("ise_id").toInt();
                // Thermostats
                if (deviceType.startsWith("HmIP-STH")) {
                    QString serialNumber = xml.attributes().value("address").toString();

                    if (myThings().filterByParam(thermostatThingSerialNumberParamTypeId, serialNumber).isEmpty()) {
                        qCDebug(dcOpenCCU()) << "Adding new" << deviceType;
                        ThingDescriptor descriptor(thermostatThingClassId, deviceName, deviceType + " - " + serialNumber, thing->id());
                        ParamList params;
                        params.append(Param(thermostatThingSerialNumberParamTypeId, serialNumber));
                        params.append(Param(thermostatThingTypeParamTypeId, deviceType));
                        params.append(Param(thermostatThingIseIdParamTypeId, iseId));
                        descriptor.setParams(params);
                        descriptors.append(descriptor);
                    } else {
                        qCDebug(dcOpenCCU()) << "Thing for" << deviceType << serialNumber << "already created.";
                    }
                }

                // Floor heating controller // C-8 or C-12
                if (deviceType.startsWith("HmIP-FALMOT-C")) {
                    QString serialNumber = xml.attributes().value("address").toString();

                    if (myThings().filterByParam(floorHeatingControllerThingSerialNumberParamTypeId, serialNumber).isEmpty()) {
                        qCDebug(dcOpenCCU()) << "Adding new" << deviceType;
                        ThingDescriptor descriptor(floorHeatingControllerThingClassId, deviceName, deviceType + " - " + serialNumber, thing->id());
                        ParamList params;
                        params.append(Param(floorHeatingControllerThingSerialNumberParamTypeId, serialNumber));
                        params.append(Param(floorHeatingControllerThingTypeParamTypeId, deviceType));
                        params.append(Param(floorHeatingControllerThingIseIdParamTypeId, iseId));
                        descriptor.setParams(params);
                        descriptors.append(descriptor);
                    } else {
                        qCDebug(dcOpenCCU()) << "Thing for" << deviceType << serialNumber << "already created.";
                    }
                }
            }
        }

        if (!descriptors.isEmpty()) {
            emit autoThingsAppeared(descriptors);
        }

        getStateList(thing);
    });
}

void IntegrationPluginOpenCCU::getDeviceTypeList(Thing *thing)
{
    QUrl url = buildUrl(thing,  "devicetypelist.cgi");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::sslErrors, this, &IntegrationPluginOpenCCU::onSslError);
    connect(reply, &QNetworkReply::finished, this, [thing, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(dcOpenCCU()) << "Reply finished with error" << reply->errorString();
            return;
        }

        QByteArray data = reply->readAll();
        //qCDebug(dcOpenCCU()) << "-->" << data;
        thing->setStateValue(openCCUConnectedStateTypeId, true);

        QXmlStreamReader xml(data);
        while(!xml.atEnd() && !xml.hasError()) {
            xml.readNext();
            if (xml.name() == QString("deviceType")) {
                qCDebug(dcOpenCCU()) << "-->" << xml.name() << xml.text();
                foreach (const QXmlStreamAttribute &attribute, xml.attributes()) {
                    qCDebug(dcOpenCCU()) << "    " << attribute.name() << attribute.value();
                }
            }
        }
    });
}

void IntegrationPluginOpenCCU::getState(Thing *thing)
{
    QUrl url = buildUrl(thing,  "state.cgi");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::sslErrors, this, &IntegrationPluginOpenCCU::onSslError);
    connect(reply, &QNetworkReply::finished, this, [thing, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(dcOpenCCU()) << "Reply finished with error" << reply->errorString();
            return;
        }

        QByteArray data = reply->readAll();
        //qCDebug(dcOpenCCU()) << "-->" << data;
        thing->setStateValue(openCCUConnectedStateTypeId, true);

        QXmlStreamReader xml(data);
        while(!xml.atEnd() && !xml.hasError()) {
            xml.readNext();
            if (xml.name() == QString("device")) {
                qCDebug(dcOpenCCU()) << "-->" << xml.name() << xml.text();
                foreach (const QXmlStreamAttribute &attribute, xml.attributes()) {
                    qCDebug(dcOpenCCU()) << "    " << attribute.name() << attribute.value();
                }
            }
        }
    });
}

void IntegrationPluginOpenCCU::getStateList(Thing *thing)
{
    // filtering if ise_id is not working with API version 2.3
    // QUrlQuery query;
    // if (iseId >= 0) {
    //     query.addQueryItem("ise_id", QString::number(iseId));
    // }

    QUrl url = buildUrl(thing, "statelist.cgi");
    qCDebug(dcOpenCCU()) << "GET" << url.toString();
    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::sslErrors, this, &IntegrationPluginOpenCCU::onSslError);
    connect(reply, &QNetworkReply::finished, this, [this, thing, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(dcOpenCCU()) << "Reply finished with error" << reply->errorString();
            return;
        }

        QByteArray data = reply->readAll();
        //qCDebug(dcOpenCCU()) << "-->" << data;
        thing->setStateValue(openCCUConnectedStateTypeId, true);

        QXmlStreamReader xml(data);
        while(!xml.atEnd() && !xml.hasError()) {
            xml.readNext();
            if (xml.name() == QString("device") && xml.isStartElement())  {
                qCDebug(dcOpenCCU()) << "-->" << xml.name() << xml.attributes().value("name").toString();
                foreach (const QXmlStreamAttribute &attribute, xml.attributes()) {
                    qCDebug(dcOpenCCU()) << "    " << attribute.name() << attribute.value();
                }

                int iseId = xml.attributes().value("ise_id").toInt();
                foreach (Thing *childThing, myThings().filterByParentId(thing->id())) {
                    if (childThing->paramValue("iseId").toInt() == iseId) {
                        qCDebug(dcOpenCCU()) << "Updating states of" << childThing;
                        processThingStateList(&xml, childThing);
                    }
                }
            }
        }
    });
}

void IntegrationPluginOpenCCU::getChannels(Thing *floorHeatinController)
{
    qCDebug(dcOpenCCU()) << "Sync channels from" << floorHeatinController;

    QUrl url = buildUrl(myThings().findById(floorHeatinController->parentId()), "statelist.cgi");
    qCDebug(dcOpenCCU()) << "GET" << url.toString();
    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::sslErrors, this, &IntegrationPluginOpenCCU::onSslError);
    connect(reply, &QNetworkReply::finished, this, [this, floorHeatinController, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(dcOpenCCU()) << "Reply finished with error" << reply->errorString();
            return;
        }

        QByteArray data = reply->readAll();
        //qCDebug(dcOpenCCU()) << "-->" << qUtf8Printable(data);
        floorHeatinController->setStateValue(floorHeatingControllerConnectedStateTypeId, true);
        int floorHeatingControllerIseId = floorHeatinController->paramValue(floorHeatingControllerThingIseIdParamTypeId).toInt();

        QXmlStreamReader xml(data);
        while(!xml.atEnd() && !xml.hasError()) {
            xml.readNext();
            if (xml.name() == QString("device") && xml.isStartElement())  {
                qCDebug(dcOpenCCU()) << "-->" << xml.name() << xml.attributes().value("name").toString();
                foreach (const QXmlStreamAttribute &attribute, xml.attributes()) {
                    qCDebug(dcOpenCCU()) << "    " << attribute.name() << attribute.value();
                }

                int iseId = xml.attributes().value("ise_id").toInt();
                if (iseId == floorHeatingControllerIseId) {

                    int currentChannelIndex = 0;
                    ThingDescriptors desciptors;

                    // Read all channels of this device and verify if there is a valve connected and if we have already set up device for it
                    while (!xml.atEnd() && !xml.hasError() && !(xml.name() == QString("device") && xml.isEndElement())) {
                        xml.readNext();
                        if (xml.name() == QString("channel")  && xml.isStartElement()) {

                            // Channel 0 = Maintainance channel
                            currentChannelIndex = xml.attributes().value("index").toInt();

                            // qCDebug(dcOpenCCU()) << "-->" << xml->name() << xml->attributes().value("name").toString();
                            // foreach (const QXmlStreamAttribute &attribute, xml->attributes()) {
                            //     qCDebug(dcOpenCCU()) << "    " << attribute.name() << attribute.value();
                            // }
                        }

                        if (xml.name() == QString("datapoint")  && xml.isStartElement()) {
                            qCDebug(dcOpenCCU()) << "  -->" << xml.name() << xml.attributes().value("name").toString();
                            foreach (const QXmlStreamAttribute &attribute, xml.attributes()) {
                                qCDebug(dcOpenCCU()) << "    " << attribute.name() << attribute.value();
                            }


                            QString type = xml.attributes().value("type").toString();
                            int iseId = xml.attributes().value("ise_id").toInt();

                            if (currentChannelIndex == 0) {
                                if (type == "RSSI_DEVICE") {
                                    int signalStrength = getSignalStrenthFromRssi(xml.attributes().value("value").toInt());
                                    floorHeatinController->setStateValue(floorHeatingControllerSignalStrengthStateTypeId, signalStrength);
                                    qCDebug(dcOpenCCU()) << "Floor heating controller" << floorHeatinController->name() << iseId << "signal strength:" << signalStrength << "%";
                                } else if (type == "UNREACH") {
                                    floorHeatinController->setStateValue(floorHeatingControllerReachableStateTypeId, xml.attributes().value("value").toString() == "false");
                                    floorHeatinController->setStateValue(floorHeatingControllerConnectedStateTypeId, floorHeatinController->stateValue(floorHeatingControllerReachableStateTypeId));
                                    qCDebug(dcOpenCCU()) << "Floor heating controller" << floorHeatinController->name() << iseId << "unreachable:" << xml.attributes().value("value").toString();
                                }
                            } else {
                                // Actual valve channels, if a valve is connected, the value is not empty
                                if (type == "LEVEL") {
                                    if (xml.attributes().value("value").toString().isEmpty()) {
                                        qCDebug(dcOpenCCU()) << "Floorheating channel" << currentChannelIndex << "has no valve connected";
                                    } else {

                                        QString thingName = floorHeatinController->name() + " Channel " + QString::number(currentChannelIndex);
                                        if (myThings().filterByParam(floorHeatingValveThingIseIdParamTypeId, iseId).isEmpty()) {
                                            qCDebug(dcOpenCCU()) << "Adding new floor heating valve" << iseId;
                                            ThingDescriptor desciptor(floorHeatingValveThingClassId, thingName, QString(), floorHeatinController ->id());
                                            ParamList params;
                                            params.append(Param(floorHeatingValveThingIseIdParamTypeId, iseId));
                                            desciptor.setParams(params);
                                            desciptors.append(desciptor);
                                        } else {
                                            qCDebug(dcOpenCCU()) << "Thing for" << thingName << "already created.";
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (!desciptors.isEmpty()) {
                        emit autoThingsAppeared(desciptors);
                    }
                }
            }
        }
    });

}


void IntegrationPluginOpenCCU::processThingStateList(QXmlStreamReader *xml, Thing *thing)
{
    int currentChannelIndex = 0;

    while (!xml->atEnd() && !xml->hasError() && !(xml->name() == QString("device") && xml->isEndElement())) {
        xml->readNext();
        if (xml->name() == QString("channel")  && xml->isStartElement()) {
            // qCDebug(dcOpenCCU()) << "-->" << xml->name() << xml->attributes().value("name").toString();
            // foreach (const QXmlStreamAttribute &attribute, xml->attributes()) {
            //     qCDebug(dcOpenCCU()) << "    " << attribute.name() << attribute.value();
            // }

            currentChannelIndex = xml->attributes().value("index").toInt();

            if (thing->thingClassId() == thermostatThingClassId) {
                // Channel 0 = Maintainance channel
                m_thermostats[thing].channels[currentChannelIndex].index = currentChannelIndex;
                m_thermostats[thing].channels[currentChannelIndex].iseId = xml->attributes().value("type").toInt();
            }
        }

        if (xml->name() == QString("datapoint")  && xml->isStartElement()) {
            // qCDebug(dcOpenCCU()) << "  -->" << xml->name() << xml->attributes().value("name").toString();
            // foreach (const QXmlStreamAttribute &attribute, xml->attributes()) {
            //     qCDebug(dcOpenCCU()) << "    " << attribute.name() << attribute.value();
            // }

            QString type = xml->attributes().value("type").toString();
            int iseId = xml->attributes().value("ise_id").toInt();
            if (thing->thingClassId() == thermostatThingClassId) {
                if (type == "LOW_BAT") {
                    thing->setStateValue(thermostatBatteryCriticalStateTypeId, xml->attributes().value("value").toString() == "true");
                } else if (type == "RSSI_DEVICE") {
                    thing->setStateValue(thermostatSignalStrengthStateTypeId, getSignalStrenthFromRssi(xml->attributes().value("value").toInt()));
                } else if (type == "UNREACH") {
                    thing->setStateValue(thermostatReachableStateTypeId, xml->attributes().value("value").toString() == "false");
                    thing->setStateValue(thermostatConnectedStateTypeId, thing->stateValue(thermostatReachableStateTypeId));
                    qCDebug(dcOpenCCU()) << "Thermostat" << thing->name() << iseId << "unreachable:" << xml->attributes().value("value").toString();
                } else if (type == "ACTUAL_TEMPERATURE") {
                    thing->setStateValue(thermostatTemperatureStateTypeId, xml->attributes().value("value").toDouble());
                    qCDebug(dcOpenCCU()) << "Thermostat" << thing->name() << iseId << "temperature:" << thing->stateValue(thermostatTemperatureStateTypeId).toDouble();
                } else if (type == "SET_POINT_TEMPERATURE") {
                    thing->setStateValue(thermostatTargetTemperatureStateTypeId, xml->attributes().value("value").toDouble());
                    m_thermostats[thing].channels[currentChannelIndex].targetTemperatureId = xml->attributes().value("ise_id").toInt();
                    qCDebug(dcOpenCCU()) << "Thermostat" << thing->name() << iseId << "target temperature:" << thing->stateValue(thermostatTargetTemperatureStateTypeId).toDouble();
                } else if (type == "WINDOW_STATE") {
                    thing->setStateValue(thermostatWindowOpenDetectedStateTypeId, xml->attributes().value("value").toInt() > 1);
                    qCDebug(dcOpenCCU()) << "Thermostat" << thing->name() << iseId << "window state:" << thing->stateValue(thermostatWindowOpenDetectedStateTypeId).toBool();
                } else if (type == "HUMIDITY") {
                    thing->setStateValue(thermostatHumidityStateTypeId, xml->attributes().value("value").toDouble());
                    qCDebug(dcOpenCCU()) << "Thermostat" << thing->name() << iseId << "humidity:" << thing->stateValue(thermostatHumidityStateTypeId).toInt();
                } else if (type == "SET_POINT_MODE") {
                    m_thermostats[thing].channels[currentChannelIndex].modeId = iseId;
                    int setPointMode = xml->attributes().value("value").toInt();
                    qCDebug(dcOpenCCU()) << "Thermostat" << thing->name() << iseId << "mode:" << setPointMode;
                    if (setPointMode == 0) {
                        thing->setStateValue(thermostatModeStateTypeId, "Auto");
                    } else if (setPointMode == 1) {
                        thing->setStateValue(thermostatModeStateTypeId, "Manual");
                    }
                } else if (type == "CONTROL_MODE") {
                    // Some device can be controlled using this data point instead of SET_POINT_MODE.
                    // If this data point exists, we update manual and auto mode using this iseId, not the
                    // iseId of the SET_POINT_MODE
                    m_thermostats[thing].channels[currentChannelIndex].controlModeId = iseId;
                }
            } else if (thing->thingClassId() == floorHeatingControllerThingClassId) {
                if (type == "RSSI_DEVICE") {
                    int signalStrength = getSignalStrenthFromRssi(xml->attributes().value("value").toInt());
                    thing->setStateValue(floorHeatingControllerSignalStrengthStateTypeId, signalStrength);
                } else if (type == "UNRACH") {
                    bool isConnected = xml->attributes().value("value").toString() == "false";
                    thing->setStateValue(floorHeatingControllerConnectedStateTypeId, isConnected);
                    foreach (Thing *childThing, myThings().filterByParentId(thing->id())) {
                        childThing->setStateValue(floorHeatingValveConnectedStateTypeId, isConnected);
                    }
                }
            } else if (thing->thingClassId() == floorHeatingValveThingClassId) {
                if (type == "LEVEL") {
                    thing->setStateValue(floorHeatingValvePercentageStateTypeId, xml->attributes().value("value").toDouble());
                    qCDebug(dcOpenCCU()) << "Valve position" << thing->name() << thing->stateValue(floorHeatingValvePercentageStateTypeId).toDouble() << "%";
                }
            }
        }
    }
}

int IntegrationPluginOpenCCU::getSignalStrenthFromRssi(int rssi)
{
    int signalStrength = 0;
    if (rssi > -65) {
        signalStrength = 100;
    } else if (rssi <= -65 && rssi >= -75) {
        signalStrength = 75;
    } else if (rssi <= -75 && rssi >= -85) {
        signalStrength = 50;
    } else if (rssi <= -85) {
        signalStrength = 25;
    }
    return signalStrength;
}

QUrl IntegrationPluginOpenCCU::buildUrl(Thing *thing, const QString &method, const QUrlQuery &query)
{
    QString token = thing->paramValue(openCCUThingTokenParamTypeId).toString();
    QHostAddress address(thing->paramValue(openCCUThingAddressParamTypeId).toString());

    QUrlQuery newQuery = query;
    newQuery.addQueryItem("sid", token);

    bool usingSsl = m_usingSsl.value(thing, true);

    QUrl url;
    url.setScheme(usingSsl ? "https" : "http");
    url.setHost(address.toString());
    url.setPath("/addons/xmlapi/" + method);
    url.setQuery(newQuery);
    return url;
}

void IntegrationPluginOpenCCU::onSslError(const QList<QSslError> &errors)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (errors.count() == 1 && errors.first().error() == QSslError::SelfSignedCertificate) {
        reply->ignoreSslErrors();
    } else {
        qCWarning(dcOpenCCU()) << "SSL error:" << errors.first().error() << errors.first().errorString();
        reply->abort();
    }
}


