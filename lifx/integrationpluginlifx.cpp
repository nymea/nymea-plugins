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

#include "integrationpluginlifx.h"

#include "integrations/integrationplugin.h"
#include "types/param.h"
#include "plugininfo.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"

#include <QDebug>
#include <QColor>
#include <QRgb>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

IntegrationPluginLifx::IntegrationPluginLifx()
{

}

void IntegrationPluginLifx::init()
{
    m_connectedStateTypeIds.insert(colorBulbThingClassId, colorBulbConnectedStateTypeId);
    m_connectedStateTypeIds.insert(dimmableBulbThingClassId, dimmableBulbConnectedStateTypeId);
    m_connectedStateTypeIds.insert(lifxAccountThingClassId, lifxAccountConnectedStateTypeId);

    m_powerStateTypeIds.insert(colorBulbThingClassId, colorBulbPowerStateTypeId);
    m_powerStateTypeIds.insert(dimmableBulbThingClassId, dimmableBulbPowerStateTypeId);

    m_brightnessStateTypeIds.insert(colorBulbThingClassId, colorBulbBrightnessStateTypeId);
    m_brightnessStateTypeIds.insert(dimmableBulbThingClassId, dimmableBulbBrightnessStateTypeId);


    m_colorTemperatureStateTypeIds.insert(colorBulbThingClassId, colorBulbColorTemperatureStateTypeId);
    m_colorTemperatureStateTypeIds.insert(dimmableBulbThingClassId, dimmableBulbColorTemperatureStateTypeId);

    m_hostAddressParamTypeIds.insert(colorBulbThingClassId, colorBulbThingHostParamTypeId);
    m_hostAddressParamTypeIds.insert(dimmableBulbThingClassId, dimmableBulbThingHostParamTypeId);

    m_portParamTypeIds.insert(colorBulbThingClassId, colorBulbThingPortParamTypeId);
    m_portParamTypeIds.insert(dimmableBulbThingClassId, dimmableBulbThingPortParamTypeId);

    m_idParamTypeIds.insert(colorBulbThingClassId, colorBulbThingIdParamTypeId);
    m_idParamTypeIds.insert(dimmableBulbThingClassId, dimmableBulbThingIdParamTypeId);

    m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_hap._tcp"); // discovers all homekit devices

    QFile file;
    file.setFileName("/tmp/products.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(dcLifx()) << "Could not open products file" << file.errorString() << "file name:" << file.fileName();
    } else {
        QJsonDocument productsJson = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (!productsJson.isArray()) {
            qCWarning(dcLifx()) << "Products JSON is not a valid array";
        } else {
            QJsonArray productsArray = productsJson.array().first().toObject().value("products").toArray();
            foreach (QJsonValue value, productsArray) {
                QJsonObject object = value.toObject();
                Lifx::LifxProduct product;
                product.pid = object["pid"].toInt();
                product.name = object["name"].toString();
                qCDebug(dcLifx()) << "Lifx product JSON, found product. PID:" << product.pid << "Name" << product.name;
                QJsonObject features = object["features"].toObject();
                product.color = features["color"].toBool();
                product.infrared = features["infrared"].toBool();
                product.matrix = features["matrix"].toBool();
                product.multizone = features["multizone"].toBool();
                product.minColorTemperature = features["temperature_range"].toArray().first().toInt();
                product.maxColorTemperature = features["temperature_range"].toArray().last().toInt();
                product.chain = features["chain"].toBool();
                m_lifxProducts.insert(product.pid, product);
            }
        }
    }

    m_networkManager = hardwareManager()->networkManager();
}

void IntegrationPluginLifx::startPairing(ThingPairingInfo *info)
{
    QUrl url("https://api.lifx.com/v1");
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, reply, info] {

        if (reply->error() == QNetworkReply::NetworkError::HostNotFoundError) {
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("LIFX server is not reachable."));
        } else {
            info->finish(Thing::ThingErrorNoError, "Please enter your Token and password. Get the token from https://cloud.lifx.com/settings");
        }
    });
}

void IntegrationPluginLifx::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    QNetworkRequest request;
    request.setUrl(QUrl("https://api.lifx.com/v1/lights/all"));
    request.setRawHeader("Authorization","Bearer "+secret.toUtf8());
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [info, reply, secret, username, this] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // check HTTP status code
        if (status != 200) {
            // Error setting up device with invalid token
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("This token is invalid."));
            return;
        }
        qCDebug(dcLifx()) << "Confirm pairing successfull";
        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("token", secret);
        pluginStorage()->endGroup();

        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginLifx::discoverThings(ThingDiscoveryInfo *info)
{
    if ((info->thingClassId() == colorBulbThingClassId) || (info->thingClassId() == dimmableBulbThingClassId)) {
        QHash<QString, ThingDescriptor> descriptors;
        foreach (const ZeroConfServiceEntry avahiEntry, m_serviceBrowser->serviceEntries()) {
            if (!avahiEntry.name().contains("lifx", Qt::CaseSensitivity::CaseInsensitive)) {
                continue;
            }

            QString id;
            QString model;
            foreach (const QString &txt, avahiEntry.txt()) {
                //qCDebug(dcLifx()) << "txt entry. Key:" << txt.split("=").first() << "value:" << txt.split("=").last();
                if (txt.startsWith("id", Qt::CaseSensitivity::CaseInsensitive)) {
                    id = txt.split("=").last();
                } else if (txt.startsWith("md")) {
                    model = txt.split("=").last();
                }
            }
            if (descriptors.contains(id)) {
                // Might appear multiple times, IPv4 and IPv6
                continue;
            }
            qCDebug(dcLifx()) << "Found LIFX device" << model << "ID" << id;
            ThingDescriptor descriptor(info->thingClassId(), model, avahiEntry.name() + " (" + avahiEntry.hostAddress().toString() + ")");
            ParamList params;
            params << Param(m_idParamTypeIds.value(info->thingClassId()), id);
            params << Param(m_hostAddressParamTypeIds.value(info->thingClassId()), avahiEntry.hostAddress().toString());
            params << Param(m_portParamTypeIds.value(info->thingClassId()), avahiEntry.port());
            descriptor.setParams(params);

            Things existing = myThings().filterByParam(m_idParamTypeIds.value(info->thingClassId()), id);
            if (existing.count() > 0) {
                descriptor.setThingId(existing.first()->id());
            }
            descriptors.insert(id, descriptor);
        }
        info->addThingDescriptors(descriptors.values());
        info->finish(Thing::ThingErrorNoError);
    } else {
        Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(info->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginLifx::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == colorBulbThingClassId || thing->thingClassId() == dimmableBulbThingClassId) {
        Lifx *lifx = new Lifx(QHostAddress(thing->paramValue(m_hostAddressParamTypeIds.value(thing->thingClassId())).toString()),
                              thing->paramValue(m_portParamTypeIds.value(thing->thingClassId())).toInt(), this);
        if(lifx->enable()) {
            m_lifxConnections.insert(thing, lifx);
            //TODO async setup
            info->finish(Thing::ThingErrorNoError);
        } else {
            lifx->deleteLater();
            info->finish(Thing::ThingErrorSetupFailed);
        }
    } else if (thing->thingClassId() == lifxAccountThingClassId) {

        pluginStorage()->beginGroup(thing->id().toString());
        QByteArray token = pluginStorage()->value("token").toByteArray();
        QByteArray username = pluginStorage()->value("username").toByteArray();
        pluginStorage()->endGroup();

        if (token.isEmpty()) {
            qCWarning(dcLifx()) << "Lifx setup, token is not stored";
            return info->finish(Thing::ThingErrorAuthenticationFailure);
        }
        thing->setStateValue(lifxAccountUserDisplayNameStateTypeId, username);
        LifxCloud *lifxCloud = new LifxCloud(hardwareManager()->networkManager(), this);
        m_asyncCloudSetups.insert(lifxCloud, info);
        connect(info, &ThingSetupInfo::aborted, info, [lifxCloud, this] {
            m_asyncCloudSetups.remove(lifxCloud);
            lifxCloud->deleteLater();
        });
        connect(lifxCloud, &LifxCloud::lightsListReceived, this, &IntegrationPluginLifx::onLifxCloudLightsListReceived);
        connect(lifxCloud, &LifxCloud::scenesListReceived, this, &IntegrationPluginLifx::onLifxCloudScenesListReceived);
        connect(lifxCloud, &LifxCloud::requestExecuted, this, &IntegrationPluginLifx::onLifxCloudRequestExecuted);
        connect(lifxCloud, &LifxCloud::connectionChanged, this, &IntegrationPluginLifx::onLifxCloudConnectionChanged);
        connect(lifxCloud, &LifxCloud::authenticationChanged, this, &IntegrationPluginLifx::onLifxCloudAuthenticationChanged);
        lifxCloud->setAuthorizationToken(token);
        lifxCloud->listLights();

        //TODO try setup again if it failes
    } else {
        Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginLifx::postSetupThing(Thing *thing)
{
    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this]() {
            foreach (Lifx *lifx, m_lifxConnections) {
                Q_UNUSED(lifx)
            }

            foreach (LifxCloud *lifx, m_lifxCloudConnections) {
                lifx->listLights();
            }
        });
    }

    if (thing->thingClassId() == lifxAccountThingClassId) {
        thing->setStateValue(lifxAccountConnectedStateTypeId, true);
        thing->setStateValue(lifxAccountLoggedInStateTypeId, true);
    }
}

void IntegrationPluginLifx::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();
    bool cloudDevice = false;
    Lifx *lifx;
    LifxCloud *lifxCloud;

    if (m_lifxConnections.contains(thing)) {
        lifx = m_lifxConnections.value(thing);
    } else if (m_lifxCloudConnections.contains(myThings().findById(thing->parentId()))) {
        lifxCloud = m_lifxCloudConnections.value(myThings().findById(thing->parentId()));
        cloudDevice = true;
    } else {
        qCWarning(dcLifx()) << "Could not find any LIFX connection for thing" << thing->name();
        return info->finish(Thing::ThingErrorHardwareFailure);
    }

    if (thing->thingClassId() == colorBulbThingClassId) {
        QByteArray lightId = thing->paramValue(colorBulbThingIdParamTypeId).toByteArray();
        if (action.actionTypeId() == colorBulbPowerActionTypeId) {
            bool power = action.param(colorBulbPowerActionPowerParamTypeId).value().toBool();
            int requestId;
            if (cloudDevice) {
                requestId = lifxCloud->setPower(lightId, power);
            } else {
                requestId = lifx->setPower(power);
            }
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);

        } else if (action.actionTypeId() == colorBulbBrightnessActionTypeId) {

            if (!thing->stateValue(colorBulbPowerStateTypeId).toBool())
                lifx->setPower(true);

            int brightness = info->action().param(colorBulbBrightnessActionBrightnessParamTypeId).value().toInt();
            int requestId;
            if (cloudDevice) {
                requestId = lifxCloud->setBrightnesss(lightId, brightness);
            } else {
                requestId = lifx->setBrightness(brightness);
            }
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbColorActionColorParamTypeId) {
            QRgb color = QColor(action.param(colorBulbColorActionColorParamTypeId).value().toString()).rgba();
            if (!thing->stateValue(colorBulbPowerStateTypeId).toBool())
                lifx->setPower(true);
            int requestId;
            if (cloudDevice) {
                requestId = lifxCloud->setColor(lightId, color);
            } else {
                requestId = lifx->setColor(color);
            }
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbColorTemperatureActionTypeId) {
            int colorTemperature = 6500 - (action.param(colorBulbColorTemperatureActionColorTemperatureParamTypeId).value().toUInt() * -11.12);
            if (!thing->stateValue(colorBulbPowerStateTypeId).toBool())
                lifx->setPower(true);
            int requestId;
            if (cloudDevice) {
                requestId = lifxCloud->setColorTemperature(lightId, colorTemperature);
            } else {
                requestId = lifx->setColorTemperature(colorTemperature);
            }
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else {
            Q_ASSERT_X(false, "executeAction", QString("Unhandled actionTypeId: %1").arg(action.actionTypeId().toString()).toUtf8());
        }
    } else if (thing->thingClassId() == dimmableBulbThingClassId) {
        QByteArray lightId = thing->paramValue(dimmableBulbThingIdParamTypeId).toByteArray();
        if (action.actionTypeId() == dimmableBulbPowerActionTypeId) {
            bool power = action.param(dimmableBulbPowerActionPowerParamTypeId).value().toBool();
            int requestId;
            if (cloudDevice) {
                requestId = lifxCloud->setPower(lightId, power);
            } else {
                requestId = lifx->setPower(power);
            }
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == dimmableBulbBrightnessActionTypeId) {
            int brightness = action.param(dimmableBulbBrightnessActionBrightnessParamTypeId).value().toInt();
            if (!thing->stateValue(dimmableBulbPowerStateTypeId).toBool())
                lifx->setPower(true);
            int requestId;
            if (cloudDevice) {
                requestId = lifxCloud->setBrightnesss(lightId, brightness);
            } else {
                requestId = lifx->setBrightness(brightness);
            }
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else {
            Q_ASSERT_X(false, "executeAction", QString("Unhandled actionTypeId: %1").arg(action.actionTypeId().toString()).toUtf8());
        }
    } else {
        Q_ASSERT_X(false, "executeAction", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginLifx::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == colorBulbThingClassId || thing->thingClassId() == dimmableBulbThingClassId) {
        if (m_lifxConnections.contains(thing))
            m_lifxConnections.take(thing)->deleteLater();
    } else if (thing->thingClassId() == lifxAccountThingClassId) {
        if (m_lifxCloudConnections.contains(thing))
            m_lifxCloudConnections.take(thing)->deleteLater();
    }

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginLifx::browseThing(BrowseResult *result)
{
    Thing *thing = result->thing();
    LifxCloud *lifxCloud = m_lifxCloudConnections.value(thing);
    if (!lifxCloud)
        return;

    lifxCloud->listScenes();
    m_asyncBrowseResults.insert(lifxCloud, result);
    connect(result, &BrowseResult::aborted, this, [lifxCloud, this]{m_asyncBrowseResults.remove(lifxCloud);});
}

void IntegrationPluginLifx::browserItem(BrowserItemResult *result)
{
    Q_UNUSED(result)
    qCDebug(dcLifx()) << "BrowserItem called";
}

void IntegrationPluginLifx::executeBrowserItem(BrowserActionInfo *info)
{
    Thing *thing = info->thing();
    LifxCloud *lifxCloud = m_lifxCloudConnections.value(thing);
    int requestId = lifxCloud->activateScene(info->browserAction().itemId());
    m_asyncBrowserItem.insert(requestId, info);
    connect(info, &BrowserActionInfo::aborted, this, [requestId, this] {m_asyncBrowserItem.remove(requestId);});
}

void IntegrationPluginLifx::onConnectionChanged(bool connected)
{
    Q_UNUSED(connected)
    Lifx *lifx = static_cast<Lifx *>(sender());
    Thing *thing = m_lifxConnections.key(lifx);
    if (!thing)
        return;
    thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), connected);
}

void IntegrationPluginLifx::onRequestExecuted(int requestId, bool success)
{
    if (m_asyncActions.contains(requestId)) {
        ThingActionInfo *info = m_asyncActions.take(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    } else if (m_asyncBrowserItem.contains(requestId)) {
        BrowserActionInfo *info = m_asyncBrowserItem.take(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
    }
}

void IntegrationPluginLifx::onLifxCloudConnectionChanged(bool connected)
{
    LifxCloud *lifxCloud = static_cast<LifxCloud *>(sender());
    Thing *accountThing = m_lifxCloudConnections.key(lifxCloud);
    if (!accountThing)
        return;
    accountThing->setStateValue(m_connectedStateTypeIds.value(accountThing->thingClassId()), connected);

    foreach (Thing *thing, myThings().filterByParentId(accountThing->id())) {
        if (!connected)
            thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), connected);
    }
}

void IntegrationPluginLifx::onLifxCloudAuthenticationChanged(bool authenticated)
{
    LifxCloud *lifxCloud = static_cast<LifxCloud *>(sender());
    Thing *accountThing = m_lifxCloudConnections.key(lifxCloud);
    if (!accountThing)
        return;
    accountThing->setStateValue(lifxAccountLoggedInStateTypeId, authenticated);
}

void IntegrationPluginLifx::onLifxCloudLightsListReceived(const QList<LifxCloud::Light> &lights)
{
    LifxCloud *lifxCloud = static_cast<LifxCloud *>(sender());
    if (m_asyncCloudSetups.contains(lifxCloud)) {
        ThingSetupInfo *info = m_asyncCloudSetups.take(lifxCloud);
        m_lifxCloudConnections.insert(info->thing(), lifxCloud);
        info->finish(Thing::ThingErrorNoError);
    }

    ThingDescriptors thingDescriptors;
    Q_FOREACH(LifxCloud::Light light, lights) {
        Thing *parentThing = m_lifxCloudConnections.key(lifxCloud);
        if (!parentThing) {
            qCWarning(dcLifx()) << "Could not find thing to cloud connection";
            return;
        }
        ThingClassId thingClassId;
        if (light.product.capabilities.color) {
            thingClassId = colorBulbThingClassId;
        } else if (light.product.capabilities.colorTemperature) {
            thingClassId = dimmableBulbThingClassId;
        } else {
            qCWarning(dcLifx()) << "LIFX product is not supported";
        }
        qCDebug(dcLifx()) << "Light product:" << light.id << light.uuid << light.label << light.product.identifier;
        ThingDescriptor thingDescriptor(thingClassId, light.product.name, light.location.name, parentThing->id());
        foreach (Thing * thing, myThings().filterByParam(m_idParamTypeIds.value(thingClassId), light.id)) {
            thing->setStateValue(m_connectedStateTypeIds.value(thingClassId), light.connected);
            thing->setStateValue(m_brightnessStateTypeIds.value(thingClassId), light.brightness);
            thing->setStateValue(m_colorTemperatureStateTypeIds.value(thingClassId), light.colorTemperature);
            thing->setStateValue(m_powerStateTypeIds.value(thingClassId), light.power);
            if (thingClassId == colorBulbThingClassId) {
                thing->setStateValue(colorBulbColorStateTypeId, light.color);
            }
            thingDescriptor.setThingId(thing->id());
            break;
        }
        ParamList params;
        params << Param(m_idParamTypeIds.value(thingDescriptor.thingClassId()), light.id);
        params << Param(m_hostAddressParamTypeIds.value(thingDescriptor.thingClassId()), "-");
        params << Param(m_portParamTypeIds.value(thingDescriptor.thingClassId()), 0);
        thingDescriptor.setParams(params);
        thingDescriptors.append(thingDescriptor);
    }
    if (!thingDescriptors.isEmpty())
        autoThingsAppeared(thingDescriptors);
}

void IntegrationPluginLifx::onLifxCloudRequestExecuted(int requestId, bool success)
{
    if (m_asyncActions.contains(requestId)) {
        ThingActionInfo *info = m_asyncActions.take(requestId);
        if (!info) {
            return;
        }
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
    } else if (m_asyncBrowserItem.contains(requestId)) {
        BrowserActionInfo *info = m_asyncBrowserItem.value(requestId);
        if (!info) {
            return;
        }
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
    }
}


void IntegrationPluginLifx::onLifxCloudScenesListReceived(const QList<LifxCloud::Scene> &scenes)
{
    LifxCloud *lifxCloud = static_cast<LifxCloud *>(sender());
    Thing *thing = m_lifxCloudConnections.key(lifxCloud);
    if (!thing)
        return;
    qCDebug(dcLifx()) << "Scene list received, count: " << scenes.length();

    if (m_asyncBrowseResults.contains(lifxCloud)) {
        BrowseResult *result = m_asyncBrowseResults.take(lifxCloud);
        foreach (LifxCloud::Scene scene, scenes) {
            BrowserItem item;
            item.setId(scene.id);
            item.setBrowsable(false);
            item.setExecutable(true);
            item.setDisplayName(scene.name);
            item.setDisabled(false);
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
    }
}
