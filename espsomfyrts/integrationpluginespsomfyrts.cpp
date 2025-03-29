/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2024, nymea GmbH
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

#include "integrationpluginespsomfyrts.h"

#include <math.h>

#include <QUrlQuery>
#include <QHostAddress>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonParseError>

#include "plugininfo.h"
#include "espsomfyrtsdiscovery.h"

IntegrationPluginEspSomfyRts::IntegrationPluginEspSomfyRts()
{

}

void IntegrationPluginEspSomfyRts::init()
{

}

void IntegrationPluginEspSomfyRts::discoverThings(ThingDiscoveryInfo *info)
{
    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
        qCWarning(dcESPSomfyRTS()) << "Failed to discover network devices. The network device discovery is not available.";
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to discover devices in your network."));
        return;
    }

    qCInfo(dcESPSomfyRTS()) << "Starting network discovery...";
    EspSomfyRtsDiscovery *discovery = new EspSomfyRtsDiscovery(hardwareManager()->networkManager(), hardwareManager()->networkDeviceDiscovery(), info);
    connect(discovery, &EspSomfyRtsDiscovery::discoveryFinished, info, [=](){
        ThingDescriptors descriptors;
        qCInfo(dcESPSomfyRTS()) << "Discovery finished. Found" << discovery->results().count() << "devices";
        foreach (const EspSomfyRtsDiscovery::Result &result, discovery->results()) {
            qCInfo(dcESPSomfyRTS()) << "Discovered device on" << result.networkDeviceInfo;

            QString title = "ESP Somfy RTS (" + result.name + ")";
            QString description = result.networkDeviceInfo.address().toString();

            ThingDescriptor descriptor(espSomfyRtsThingClassId, title, description);

            ParamList params;
            params << Param(espSomfyRtsThingMacAddressParamTypeId, result.networkDeviceInfo.thingParamValueMacAddress());
            params << Param(espSomfyRtsThingHostNameParamTypeId, result.networkDeviceInfo.thingParamValueHostName());
            params << Param(espSomfyRtsThingAddressParamTypeId, result.networkDeviceInfo.thingParamValueAddress());
            descriptor.setParams(params);

            // Check if we already have set up this device
            Thing *existingThing = myThings().findByParams(params);
            if (existingThing) {
                qCDebug(dcESPSomfyRTS()) << "This thing already exists in the system:" << result.networkDeviceInfo;
                descriptor.setThingId(existingThing->id());
            }

            info->addThingDescriptor(descriptor);
        }

        info->finish(Thing::ThingErrorNoError);
    });

    discovery->startDiscovery();
}

void IntegrationPluginEspSomfyRts::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == espSomfyRtsThingClassId) {
        if (!hardwareManager()->networkDeviceDiscovery()->available()) {
            qCWarning(dcESPSomfyRTS()) << "Cannot set up thing because the network discovery is not available.";
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        NetworkDeviceMonitor *monitor = hardwareManager()->networkDeviceDiscovery()->registerMonitor(thing);
        if (!monitor) {
            qCWarning(dcESPSomfyRTS()) << "Could not register monitor with the given parameters" << thing << thing->params();
            info->finish(Thing::ThingErrorInvalidParameter);
            return;
        }

        EspSomfyRts *somfy = new EspSomfyRts(monitor, thing);
        m_somfys.insert(thing, somfy);

        connect(somfy, &EspSomfyRts::connectedChanged, thing, [this, thing](bool connected){
            onEspSomfyConnectedChanged(thing, connected);
        });

        connect(somfy, &EspSomfyRts::signalStrengthChanged, thing, [thing](uint signalStrength){
            thing->setStateValue(espSomfyRtsSignalStrengthStateTypeId, signalStrength);
        });

        connect(somfy, &EspSomfyRts::firmwareVersionChanged, thing, [thing](const QString &firmwareVersion){
            thing->setStateValue(espSomfyRtsFirmwareVersionStateTypeId, firmwareVersion);
        });

        connect(somfy, &EspSomfyRts::shadeStateReceived, thing, [this](const QVariantMap &shadeState){
            int shadeId = shadeState.value("shadeId").toInt();
            if (m_shadeThings.contains(shadeId)) {
                processShadeState(m_shadeThings.value(shadeId), shadeState);
            }
        });

        info->finish(Thing::ThingErrorNoError);
        return;
    } else {
        qCDebug(dcESPSomfyRTS()) << "Setting up" << thing->thingClass().name() << thing->name();
        m_shadeThings.insert(thing->paramValue("shadeId").toUInt(), thing);
        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginEspSomfyRts::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == espSomfyRtsThingClassId) {
        EspSomfyRts *somfy = m_somfys.value(thing);
        onEspSomfyConnectedChanged(thing, somfy->connected());

        if (!m_refreshTimer) {
            m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
            connect(m_refreshTimer, &PluginTimer::timeout, thing, [this, thing](){
                if (m_somfys.value(thing)->connected()) {
                    synchronizeShades(thing);
                }
            });
        }

    } else {
        Thing *parent = myThings().findById(thing->parentId());
        EspSomfyRts *somfy = m_somfys.value(parent);
        if (!parent || !somfy)
            return;

        thing->setStateValue("connected", somfy->connected());
    }
}

void IntegrationPluginEspSomfyRts::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == espSomfyRtsThingClassId) {
        EspSomfyRts *somfy = m_somfys.take(thing);
        hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(somfy->monitor());
    }
}

void IntegrationPluginEspSomfyRts::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == awningThingClassId) {

        if (!thing->stateValue(awningConnectedStateTypeId).toBool()) {
            qCWarning(dcESPSomfyRTS()) << "Could not execute command because the thing is not connected" << thing;
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        Thing *parentThing = myThings().findById(thing->parentId());
        EspSomfyRts *somfy = m_somfys.value(parentThing);
        if (!parentThing || !somfy) {
            qCWarning(dcESPSomfyRTS()) << "Could not execute command because the parent thing could not be found for" << thing;
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        QVariantMap requestMap;
        requestMap.insert("shadeId", thing->paramValue(awningThingShadeIdParamTypeId).toUInt());

        if (action.actionTypeId() == awningOpenActionTypeId) {
            requestMap.insert("command", EspSomfyRts::getShadeCommandString(EspSomfyRts::ShadeCommandDown));
        } else if (action.actionTypeId() == awningStopActionTypeId) {
            requestMap.insert("command", EspSomfyRts::getShadeCommandString(EspSomfyRts::ShadeCommandMy));
        } else if (action.actionTypeId() == awningCloseActionTypeId) {
            requestMap.insert("command", EspSomfyRts::getShadeCommandString(EspSomfyRts::ShadeCommandUp));
        } else if (action.actionTypeId() == awningPercentageActionTypeId) {
            requestMap.insert("target", action.paramValue(awningPercentageActionPercentageParamTypeId).toUInt());
        }

        QNetworkRequest request(somfy->shadeCommandUrl());
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply *reply = hardwareManager()->networkManager()->put(request, QJsonDocument::fromVariant(requestMap).toJson());
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [reply, info](){

            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcESPSomfyRTS()) << "Could not execute command on" << info->thing() << "because the network request finished with error" << reply->errorString();
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            qCDebug(dcESPSomfyRTS()) << "Executed command successfully on" << info->thing();
            info->finish(Thing::ThingErrorNoError);
        });

        return;
    }

    if (thing->thingClassId() == venetianBlindThingClassId) {

        if (!thing->stateValue(venetianBlindConnectedStateTypeId).toBool()) {
            qCWarning(dcESPSomfyRTS()) << "Could not execute command because the thing is not connected" << thing;
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        Thing *parentThing = myThings().findById(thing->parentId());
        EspSomfyRts *somfy = m_somfys.value(parentThing);
        if (!parentThing || !somfy) {
            qCWarning(dcESPSomfyRTS()) << "Could not execute command because the parent thing could not be found for" << thing;
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        QVariantMap requestMap;
        requestMap.insert("shadeId", thing->paramValue(venetianBlindThingShadeIdParamTypeId).toUInt());

        QUrl url = somfy->shadeCommandUrl();
        if (action.actionTypeId() == venetianBlindOpenActionTypeId) {
            requestMap.insert("command", EspSomfyRts::getShadeCommandString(EspSomfyRts::ShadeCommandUp));
        } else if (action.actionTypeId() == venetianBlindStopActionTypeId) {
            requestMap.insert("command", EspSomfyRts::getShadeCommandString(EspSomfyRts::ShadeCommandMy));
        } else if (action.actionTypeId() == venetianBlindCloseActionTypeId) {
            requestMap.insert("command", EspSomfyRts::getShadeCommandString(EspSomfyRts::ShadeCommandDown));
        } else if (action.actionTypeId() == venetianBlindPercentageActionTypeId) {
            requestMap.insert("target", action.paramValue(venetianBlindPercentageActionPercentageParamTypeId).toUInt());
        } else if (action.actionTypeId() == venetianBlindAngleActionTypeId) {
            url = somfy->tiltCommandUrl();
            State angleState = thing->state(venetianBlindAngleStateTypeId);
            int minValue = angleState.minValue().toInt();
            int maxValue = angleState.maxValue().toInt();
            int angle = action.paramValue(venetianBlindAngleActionAngleParamTypeId).toInt();
            int percentage = calculatePercentageFromAngle(minValue, maxValue, angle);
            qCDebug(dcESPSomfyRTS()) << "######" << percentage;
            requestMap.insert("target", percentage);
        }

        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        qCDebug(dcESPSomfyRTS()) << "PUT" << url.toString() << qUtf8Printable(QJsonDocument::fromVariant(requestMap).toJson(QJsonDocument::Compact));
        QNetworkReply *reply = hardwareManager()->networkManager()->put(request, QJsonDocument::fromVariant(requestMap).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [reply, info](){

            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcESPSomfyRTS()) << "Could not execute command on" << info->thing() << "because the network request finished with error" << reply->errorString();
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            qCDebug(dcESPSomfyRTS()) << "Executed command successfully on" << info->thing();
            info->finish(Thing::ThingErrorNoError);
        });
    }
}

int IntegrationPluginEspSomfyRts::calculateAngleFromPercentage(int minAngle, int maxAngle, int percentage)
{
    int minValue = qMin(minAngle, maxAngle);
    int maxValue = qMax(minAngle, maxAngle);
    int range = maxValue - minValue;
    int angle = std::round(range * percentage / 100.0) + minAngle;
    //qCDebug(dcESPSomfyRTS()) << "Calculate angle" << angle << "for percentage" << percentage << "min:" << minValue << "max:" << maxValue << "range:" << range;
    return angle;
}

int IntegrationPluginEspSomfyRts::calculatePercentageFromAngle(int minAngle, int maxAngle, int angle)
{
    int minValue = qMin(minAngle, maxAngle);
    int maxValue = qMax(minAngle, maxAngle);
    int range = maxValue - minValue;
    int percentage = std::round(angle * 100.0 / range) + 50;
    //qCDebug(dcESPSomfyRTS()) << "Calculated percentage" << percentage << "for angle" << angle << "min:" << minValue << "max:" << maxValue << "range:" << range;
    // FIXME: check the percentage of the negative part if asymetric
    return percentage;
}

void IntegrationPluginEspSomfyRts::createThingForShade(const QVariantMap &shadeMap, const ThingId &parentThingId)
{
    QString shadeName = shadeMap.value("name").toString();
    uint shadeId = shadeMap.value("shadeId").toUInt();
    EspSomfyRts::ShadeType shadeType = static_cast<EspSomfyRts::ShadeType>(shadeMap.value("shadeType").toInt());

    qCDebug(dcESPSomfyRTS()) << "Creating thing for" << shadeType << shadeId << shadeName;

    ThingDescriptor desciptor;
    ThingDescriptors desciptors;

    switch (shadeType) {
    case EspSomfyRts::ShadeTypeAwning:
        desciptor = ThingDescriptor(awningThingClassId, shadeName);
        desciptor.setParams(ParamList() << Param(awningThingShadeIdParamTypeId, shadeId));
        desciptor.setParentId(parentThingId);
        desciptors.append(desciptor);
        break;
    case EspSomfyRts::ShadeTypeBlind:
        desciptor = ThingDescriptor(venetianBlindThingClassId, shadeName);
        desciptor.setParams(ParamList() << Param(venetianBlindThingShadeIdParamTypeId, shadeId));
        desciptor.setParentId(parentThingId);
        desciptors.append(desciptor);
        break;
    default:
        break;
    }

    if (desciptors.isEmpty())
        return;

    emit autoThingsAppeared(desciptors);
}

void IntegrationPluginEspSomfyRts::processShadeState(Thing *thing, const QVariantMap &shadeState)
{
    if (thing->thingClassId() == awningThingClassId) {

        if (shadeState.contains("position"))
            thing->setStateValue(awningPercentageStateTypeId, shadeState.value("position").toInt());

        if (shadeState.contains("direction"))
            thing->setStateValue(awningMovingStateTypeId, shadeState.value("direction").toInt() != EspSomfyRts::MovingDirectionRest);
        return;
    }

    if (thing->thingClassId() == venetianBlindThingClassId) {
        if (shadeState.contains("position"))
            thing->setStateValue(venetianBlindPercentageStateTypeId, shadeState.value("position").toInt());

        if (shadeState.contains("direction"))
            thing->setStateValue(venetianBlindMovingStateTypeId, shadeState.value("direction").toInt() != EspSomfyRts::MovingDirectionRest);

        State angleState = thing->state(venetianBlindAngleStateTypeId);
        int angle = calculateAngleFromPercentage(angleState.minValue().toInt(), angleState.maxValue().toInt(), shadeState.value("tiltPosition").toInt());
        thing->setStateValue(venetianBlindAngleStateTypeId, angle);
        return;
    }
}

void IntegrationPluginEspSomfyRts::synchronizeShades(Thing *thing)
{
    EspSomfyRts *somfy = m_somfys.value(thing);
    qCDebug(dcESPSomfyRTS()) << "Synchronize shades of" << thing->name() << somfy->address().toString();

    QUrl url = somfy->shadesUrl();
    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [this, reply, thing](){

        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(dcESPSomfyRTS()) << "Get shades reply finished with error" << reply->errorString();
            return;
        }

        QJsonParseError jsonError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            qCWarning(dcESPSomfyRTS()) << "Get shades reply contains invalid JSON data" << jsonError.errorString();
            return;
        }

        QList<ThingId> handledThingIds;
        QVariantList shadesList = jsonDoc.toVariant().toList();

        // Get shades we need to add
        QList<QVariantMap> shadesToCreateThingFor;
        foreach (const QVariant &shadeVariant, shadesList) {
            QVariantMap shadeMap = shadeVariant.toMap();

            // Check if we have a thing for this shade ID
            uint shadeId = shadeMap.value("shadeId").toUInt();
            if (!m_shadeThings.contains(shadeId)) {
                shadesToCreateThingFor.append(shadeMap);
            } else {
                // We already have a shade for this map, let's update the states
                processShadeState(m_shadeThings.value(shadeId), shadeMap);
                handledThingIds.append(m_shadeThings.value(shadeId)->id());
            }

            // TODO: check if a shade has changed the type, in that case,
            // remove the old one and recreate a new one with the matching thing class
        }

        // Remove things if shade does not exist any more
        foreach (Thing *existingThing, myThings().filterByParentId(thing->id())) {
            if (!handledThingIds.contains(existingThing->id())) {
                qCDebug(dcESPSomfyRTS()) << "Removing thing" << existingThing << "because the shade with ID" << existingThing->paramValue("shadeId").toUInt() << "does not exist any more on the ESP Somfy RTS.";
                emit autoThingDisappeared(existingThing->id());
            }
        }

        // Add things for shades new shades
        foreach (const QVariantMap &shadeMap, shadesToCreateThingFor) {
            createThingForShade(shadeMap, thing->id());
        }
    });
}

void IntegrationPluginEspSomfyRts::onEspSomfyConnectedChanged(Thing *thing, bool connected)
{
    thing->setStateValue(espSomfyRtsConnectedStateTypeId, connected);
    foreach(Thing *childThing, myThings().filterByParentId(thing->id())) {
        childThing->setStateValue("connected", connected);
    }

    if (connected) {
        synchronizeShades(thing);
    }
}
