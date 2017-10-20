/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2014 Michael Zanetti <michael_zanetti@gmx.net>           *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of guh.                                              *
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

/*!
    \page philipshue.html
    \title Philips hue
    \brief Plugin for the Philips Hue lighting system.

    \ingroup plugins
    \ingroup guh-plugins

    This plugin allows to interact with the \l{http://www2.meethue.com/}{Philips hue} bridge. Each light bulp connected to the bridge
    will appear automatically in the system, once the bridge is added to guh.

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/philipshue/devicepluginphilipshue.json
*/

#include "devicepluginphilipshue.h"

#include "devicemanager.h"
#include "plugin/device.h"
#include "types/param.h"
#include "plugininfo.h"
#include "network/upnp/upnpdiscovery.h"
#include "network/upnp/upnpdiscoveryreply.h"

#include <QDebug>
#include <QStringList>
#include <QColor>
#include <QJsonDocument>

DevicePluginPhilipsHue::DevicePluginPhilipsHue()
{

}

DevicePluginPhilipsHue::~DevicePluginPhilipsHue()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void DevicePluginPhilipsHue::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(5);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginPhilipsHue::onPluginTimer);
}

DeviceManager::DeviceError DevicePluginPhilipsHue::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    Q_UNUSED(deviceClassId)

    UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices("libhue:idl");
    connect(reply, &UpnpDiscoveryReply::finished, this, &DevicePluginPhilipsHue::onUpnpDiscoveryFinished);

    return DeviceManager::DeviceErrorAsync;
}

DeviceManager::DeviceSetupStatus DevicePluginPhilipsHue::setupDevice(Device *device)
{
    // hue bridge
    if (device->deviceClassId() == hueBridgeDeviceClassId) {
        // unconfigured bridges (from pairing)
        foreach (HueBridge *b, m_unconfiguredBridges) {
            if (b->hostAddress().toString() == device->paramValue(hueBridgeBridgeHostParamTypeId).toString()) {
                m_unconfiguredBridges.removeAll(b);
                qCDebug(dcPhilipsHue) << "Setup unconfigured Hue Bridge" << b->name();
                // set data which was not known during discovery
                device->setParamValue(hueBridgeBridgeNameParamTypeId, b->name());
                device->setParamValue(hueBridgeBridgeApiKeyParamTypeId, b->apiKey());
                device->setParamValue(hueBridgeBridgeZigbeeChannelParamTypeId, b->zigbeeChannel());
                device->setParamValue(hueBridgeBridgeIdParamTypeId, b->id());
                device->setParamValue(hueBridgeBridgeMacParamTypeId, b->macAddress());
                m_bridges.insert(b, device);
                device->setStateValue(hueBridgeBridgeReachableStateTypeId, true);
                discoverBridgeDevices(b);
                return DeviceManager::DeviceSetupStatusSuccess;
            }
        }

        // loaded bridge
        qCDebug(dcPhilipsHue) << "Setup Hue Bridge" << device->params();

        HueBridge *bridge = new HueBridge(this);
        bridge->setId(device->paramValue(hueBridgeBridgeIdParamTypeId).toString());
        bridge->setApiKey(device->paramValue(hueBridgeBridgeApiKeyParamTypeId).toString());
        bridge->setHostAddress(QHostAddress(device->paramValue(hueBridgeBridgeHostParamTypeId).toString()));
        bridge->setName(device->paramValue(hueBridgeBridgeNameParamTypeId).toString());
        bridge->setMacAddress(device->paramValue(hueBridgeBridgeMacParamTypeId).toString());
        bridge->setZigbeeChannel(device->paramValue(hueBridgeBridgeZigbeeChannelParamTypeId).toInt());

        m_bridges.insert(bridge, device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    // hue color light
    if (device->deviceClassId() == hueLightDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue color light" << device->params();

        HueLight *hueLight = new HueLight(this);
        hueLight->setId(device->paramValue(hueLightLightIdParamTypeId).toInt());
        hueLight->setHostAddress(QHostAddress(device->paramValue(hueLightHostParamTypeId).toString()));
        hueLight->setName(device->paramValue(hueLightNameParamTypeId).toString());
        hueLight->setApiKey(device->paramValue(hueLightApiKeyParamTypeId).toString());
        hueLight->setModelId(device->paramValue(hueLightModelIdParamTypeId).toString());
        hueLight->setUuid(device->paramValue(hueLightUuidParamTypeId).toString());
        hueLight->setType(device->paramValue(hueLightTypeParamTypeId).toString());
        hueLight->setBridgeId(DeviceId(device->paramValue(hueLightBridgeParamTypeId).toString()));
        device->setParentId(hueLight->bridgeId());

        connect(hueLight, &HueLight::stateChanged, this, &DevicePluginPhilipsHue::lightStateChanged);
        m_lights.insert(hueLight, device);

        device->setName(hueLight->name());

        refreshLight(device);
        setLightName(device, device->paramValue(hueLightNameParamTypeId).toString());

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    // hue white light
    if (device->deviceClassId() == hueWhiteLightDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue white light" << device->params();

        HueLight *hueLight = new HueLight(this);
        hueLight->setId(device->paramValue(hueWhiteLightLightIdParamTypeId).toInt());
        hueLight->setHostAddress(QHostAddress(device->paramValue(hueWhiteLightHostParamTypeId).toString()));
        hueLight->setName(device->paramValue(hueWhiteLightNameParamTypeId).toString());
        hueLight->setApiKey(device->paramValue(hueWhiteLightApiKeyParamTypeId).toString());
        hueLight->setModelId(device->paramValue(hueWhiteLightModelIdParamTypeId).toString());
        hueLight->setUuid(device->paramValue(hueWhiteLightUuidParamTypeId).toString());
        hueLight->setType(device->paramValue(hueWhiteLightTypeParamTypeId).toString());
        hueLight->setBridgeId(DeviceId(device->paramValue(hueWhiteLightBridgeParamTypeId).toString()));
        device->setParentId(hueLight->bridgeId());

        connect(hueLight, &HueLight::stateChanged, this, &DevicePluginPhilipsHue::lightStateChanged);

        device->setName(hueLight->name());

        m_lights.insert(hueLight, device);
        refreshLight(device);

        setLightName(device, device->paramValue(hueWhiteLightNameParamTypeId).toString());
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    // hue remote
    if (device->deviceClassId() == hueRemoteDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue remote" << device->params();

        HueRemote *hueRemote = new HueRemote(this);
        hueRemote->setId(device->paramValue(hueRemoteSensorIdParamTypeId).toInt());
        hueRemote->setHostAddress(QHostAddress(device->paramValue(hueRemoteHostParamTypeId).toString()));
        hueRemote->setName(device->paramValue(hueRemoteNameParamTypeId).toString());
        hueRemote->setApiKey(device->paramValue(hueRemoteApiKeyParamTypeId).toString());
        hueRemote->setModelId(device->paramValue(hueRemoteModelIdParamTypeId).toString());
        hueRemote->setType(device->paramValue(hueRemoteTypeParamTypeId).toString());
        hueRemote->setUuid(device->paramValue(hueRemoteUuidParamTypeId).toString());
        hueRemote->setBridgeId(DeviceId(device->paramValue(hueRemoteBridgeParamTypeId).toString()));
        device->setParentId(hueRemote->bridgeId());

        device->setName(hueRemote->name());

        connect(hueRemote, &HueRemote::stateChanged, this, &DevicePluginPhilipsHue::remoteStateChanged);
        connect(hueRemote, &HueRemote::buttonPressed, this, &DevicePluginPhilipsHue::onRemoteButtonEvent);

        m_remotes.insert(hueRemote, device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    return DeviceManager::DeviceSetupStatusFailure;
}

void DevicePluginPhilipsHue::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == hueBridgeDeviceClassId) {
        HueBridge *bridge = m_bridges.key(device);
        m_bridges.remove(bridge);
        bridge->deleteLater();
    }

    if (device->deviceClassId() == hueLightDeviceClassId || device->deviceClassId() == hueWhiteLightDeviceClassId) {
        HueLight *light = m_lights.key(device);
        m_lights.remove(light);
        light->deleteLater();
    }

    if (device->deviceClassId() == hueRemoteDeviceClassId) {
        HueRemote *remote = m_remotes.key(device);
        m_remotes.remove(remote);
        remote->deleteLater();
    }
}

DeviceManager::DeviceSetupStatus DevicePluginPhilipsHue::confirmPairing(const PairingTransactionId &pairingTransactionId, const DeviceClassId &deviceClassId, const ParamList &params, const QString &secret)
{
    Q_UNUSED(secret)

    if (deviceClassId != hueBridgeDeviceClassId)
        return DeviceManager::DeviceSetupStatusFailure;

    PairingInfo *pairingInfo = new PairingInfo(this);
    pairingInfo->setPairingTransactionId(pairingTransactionId);
    pairingInfo->setHost(QHostAddress(params.paramValue(hueBridgeBridgeHostParamTypeId).toString()));

    QVariantMap deviceTypeParam;
    deviceTypeParam.insert("devicetype", "guh");

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(deviceTypeParam);

    QNetworkRequest request(QUrl("http://" + pairingInfo->host().toString() + "/api"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, jsonDoc.toJson());
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);

    m_pairingRequests.insert(reply, pairingInfo);

    return DeviceManager::DeviceSetupStatusAsync;
}

void DevicePluginPhilipsHue::networkManagerReplyReady()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // create user finished
    if (m_pairingRequests.contains(reply)) {
        PairingInfo *pairingInfo = m_pairingRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Request error:" << status << reply->errorString();
            pairingInfo->deleteLater();
            reply->deleteLater();
            return;
        }
        processPairingResponse(pairingInfo, reply->readAll());

    } else if (m_informationRequests.contains(reply)) {
        PairingInfo *pairingInfo = m_informationRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Request error:" << status << reply->errorString();
            reply->deleteLater();
            pairingInfo->deleteLater();
            return;
        }
        processInformationResponse(pairingInfo, reply->readAll());

    } else if (m_discoveryRequests.contains(reply)) {
        m_discoveryRequests.removeAll(reply);
        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "N-UPNP discovery error:" << status << reply->errorString();
            reply->deleteLater();
            return;
        }
        processNUpnpResponse(reply->readAll());

    } else if (m_bridgeLightsDiscoveryRequests.contains(reply)) {
        Device *device = m_bridgeLightsDiscoveryRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Bridge light discovery error:" << status << reply->errorString();
            bridgeReachableChanged(device, false);
            reply->deleteLater();
            return;
        }
        processBridgeLightDiscoveryResponse(device, reply->readAll());

    } else if (m_bridgeSensorsDiscoveryRequests.contains(reply)) {
        Device *device = m_bridgeSensorsDiscoveryRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Bridge sensor discovery error:" << status << reply->errorString();
            bridgeReachableChanged(device, false);
            reply->deleteLater();
            return;
        }
        processBridgeSensorDiscoveryResponse(device, reply->readAll());

    } else if (m_bridgeSearchDevicesRequests.contains(reply)) {
        Device *device = m_bridgeSearchDevicesRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Bridge search new devices error:" << status << reply->errorString();
            bridgeReachableChanged(device, false);
            reply->deleteLater();
            return;
        }
        discoverBridgeDevices(m_bridges.key(device));

    } else if (m_bridgeRefreshRequests.contains(reply)) {
        Device *device = m_bridgeRefreshRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (device->stateValue(hueBridgeBridgeReachableStateTypeId).toBool()) {
                qCWarning(dcPhilipsHue) << "Refresh Hue Bridge request error:" << status << reply->errorString();
                bridgeReachableChanged(device, false);
            }
            reply->deleteLater();
            return;
        }
        processBridgeRefreshResponse(device, reply->readAll());

    } else if (m_lightRefreshRequests.contains(reply)) {
        Device *device = m_lightRefreshRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Refresh Hue Light request error:" << status << reply->errorString();
            bridgeReachableChanged(device, false);
            reply->deleteLater();
            return;
        }
        processLightRefreshResponse(device, reply->readAll());

    } else if (m_lightsRefreshRequests.contains(reply)) {
        Device *device = m_lightsRefreshRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (device->stateValue(hueLightReachableStateTypeId).toBool()) {
                qCWarning(dcPhilipsHue) << "Refresh Hue lights request error:" << status << reply->errorString();
                bridgeReachableChanged(device, false);
            }
            reply->deleteLater();
            return;
        }
        processLightsRefreshResponse(device, reply->readAll());

    } else if (m_sensorsRefreshRequests.contains(reply)) {
        Device *device = m_sensorsRefreshRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (device->stateValue(hueRemoteHueReachableStateTypeId).toBool()) {
                qCWarning(dcPhilipsHue) << "Refresh Hue sensors request error:" << status << reply->errorString();
                bridgeReachableChanged(device, false);
            }
            reply->deleteLater();
            return;
        }
        processSensorsRefreshResponse(device, reply->readAll());

    } else if (m_asyncActions.contains(reply)) {
        QPair<Device *, ActionId> actionInfo = m_asyncActions.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Execute Hue Light action request error:" << status << reply->errorString();
            bridgeReachableChanged(actionInfo.first, false);
            emit actionExecutionFinished(actionInfo.second, DeviceManager::DeviceErrorHardwareNotAvailable);
            reply->deleteLater();
            return;
        }
        processActionResponse(actionInfo.first, actionInfo.second, reply->readAll());

    } else if (m_lightSetNameRequests.contains(reply)) {
        Device *device = m_lightSetNameRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Set name of Hue Light request error:" << status << reply->errorString();
            bridgeReachableChanged(device, false);
            reply->deleteLater();
            return;
        }
        processSetNameResponse(device, reply->readAll());
    }
    reply->deleteLater();
}

DeviceManager::DeviceError DevicePluginPhilipsHue::executeAction(Device *device, const Action &action)
{
    qCDebug(dcPhilipsHue) << "Execute action" << action.actionTypeId() << action.params();

    // color light
    if (device->deviceClassId() == hueLightDeviceClassId) {
        HueLight *light = m_lights.key(device);

        if (!light->reachable()) {
            qCWarning(dcPhilipsHue) << "Light" << light->name() << "not reachable";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (action.actionTypeId() == hueLightPowerActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetPowerRequest(action.param(hueLightPowerStateParamTypeId).value().toBool());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == hueLightColorActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetColorRequest(action.param(hueLightColorStateParamTypeId).value().value<QColor>());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply,QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == hueLightBrightnessActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetBrightnessRequest(percentageToBrightness(action.param(hueLightBrightnessStateParamTypeId).value().toInt()));
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == hueLightHueEffectActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetEffectRequest(action.param(hueLightHueEffectStateParamTypeId).value().toString());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == hueLightHueAlertActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createFlashRequest(action.param(hueLightAlertParamTypeId).value().toString());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == hueLightColorTemperatureActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetTemperatureRequest(action.param(hueLightColorTemperatureStateParamTypeId).value().toInt());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    // white light
    if (device->deviceClassId() == hueWhiteLightDeviceClassId) {
        HueLight *light = m_lights.key(device);

        if (!light->reachable()) {
            qCWarning(dcPhilipsHue) << "Light" << light->name() << "not reachable";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (action.actionTypeId() == hueWhiteLightPowerActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetPowerRequest(action.param(hueWhiteLightPowerStateParamTypeId).value().toBool());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == hueWhiteLightBrightnessActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetBrightnessRequest(percentageToBrightness(action.param(hueWhiteLightBrightnessStateParamTypeId).value().toInt()));
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == hueWhiteLightHueAlertActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createFlashRequest(action.param(hueWhiteLightAlertParamTypeId).value().toString());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == hueBridgeDeviceClassId) {
        HueBridge *bridge = m_bridges.key(device);
        if (!device->stateValue(hueBridgeBridgeReachableStateTypeId).toBool()) {
            qCWarning(dcPhilipsHue) << "Bridge" << bridge->hostAddress().toString() << "not reachable";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (action.actionTypeId() == hueBridgeSearchNewDevicesActionTypeId) {
            searchNewDevices(bridge);
            return DeviceManager::DeviceErrorNoError;
        } else if (action.actionTypeId() == hueBridgeCheckForUpdatesActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = bridge->createCheckUpdatesRequest();
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == hueBridgeUpgradeActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = bridge->createUpgradeRequest();
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginPhilipsHue::lightStateChanged()
{
    HueLight *light = static_cast<HueLight *>(sender());

    Device *device = m_lights.value(light);
    if (!device) {
        qCWarning(dcPhilipsHue) << "Could not find device for light" << light->name();
        return;
    }

    if (device->deviceClassId() == hueLightDeviceClassId) {
        device->setStateValue(hueLightReachableStateTypeId, light->reachable());
        device->setStateValue(hueLightColorStateTypeId, QVariant::fromValue(light->color()));
        device->setStateValue(hueLightPowerStateTypeId, light->power());
        device->setStateValue(hueLightBrightnessStateTypeId, brightnessToPercentage(light->brightness()));
        device->setStateValue(hueLightColorTemperatureStateTypeId, light->ct());
        device->setStateValue(hueLightHueEffectStateTypeId, light->effect());
    } else if (device->deviceClassId() == hueWhiteLightDeviceClassId) {
        device->setStateValue(hueWhiteLightHueReachableStateTypeId, light->reachable());
        device->setStateValue(hueWhiteLightPowerStateTypeId, light->power());
        device->setStateValue(hueWhiteLightBrightnessStateTypeId, brightnessToPercentage(light->brightness()));
    }
}

void DevicePluginPhilipsHue::remoteStateChanged()
{
    HueRemote *remote = static_cast<HueRemote *>(sender());

    Device *device = m_remotes.value(remote);
    if (!device) {
        qCWarning(dcPhilipsHue) << "Could not find device for remote" << remote->name();
        return;
    }

    device->setStateValue(hueRemoteHueReachableStateTypeId, remote->reachable());
    device->setStateValue(hueRemoteBatteryStateTypeId, remote->battery());
}

void DevicePluginPhilipsHue::onRemoteButtonEvent(const int &buttonCode)
{
    HueRemote *remote = static_cast<HueRemote *>(sender());

    // TODO: Legacy events should be removed eventually
    switch (buttonCode) {
    case HueRemote::OnPressed:
        emitEvent(Event(hueRemoteOnPressedEventTypeId, m_remotes.value(remote)->id()));
        break;
    case HueRemote::OnLongPressed:
        emitEvent(Event(hueRemoteOnLongPressedEventTypeId, m_remotes.value(remote)->id()));
        break;
    case HueRemote::DimUpPressed:
        emitEvent(Event(hueRemoteDimUpPressedEventTypeId, m_remotes.value(remote)->id()));
        break;
    case HueRemote::DimUpLongPressed:
        emitEvent(Event(hueRemoteDimUpLongPressedEventTypeId, m_remotes.value(remote)->id()));
        break;
    case HueRemote::DimDownPressed:
        emitEvent(Event(hueRemoteDimDownPressedEventTypeId, m_remotes.value(remote)->id()));
        break;
    case HueRemote::DimDownLongPressed:
        emitEvent(Event(hueRemoteDimDownLongPressedEventTypeId, m_remotes.value(remote)->id()));
        break;
    case HueRemote::OffPressed:
        emitEvent(Event(hueRemoteOffPressedEventTypeId, m_remotes.value(remote)->id()));
        break;
    case HueRemote::OffLongPressed:
        emitEvent(Event(hueRemoteOffLongPressedEventTypeId, m_remotes.value(remote)->id()));
        break;
    default:
        break;
    }
}

void DevicePluginPhilipsHue::onPluginTimer()
{
    foreach (Device *device, m_bridges.values()) {
        refreshBridge(device);
    }
}

void DevicePluginPhilipsHue::onUpnpDiscoveryFinished()
{
    qCDebug(dcPhilipsHue()) << "Upnp discovery finished";
    UpnpDiscoveryReply *reply = static_cast<UpnpDiscoveryReply *>(sender());
    if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
        qCWarning(dcPhilipsHue()) << "Upnp discovery error" << reply->error();
    }
    reply->deleteLater();

    if (reply->deviceDescriptors().isEmpty()) {
        qCDebug(dcPhilipsHue) << "No UPnP device found. Try N-UPNP discovery.";
        QNetworkRequest request(QUrl("https://www.meethue.com/api/nupnp"));
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
        m_discoveryRequests.append(reply);
        return;
    }

    QList<DeviceDescriptor> deviceDescriptors;
    foreach (const UpnpDeviceDescriptor &upnpDevice, reply->deviceDescriptors()) {
        if (upnpDevice.modelDescription().contains("Philips")) {
            DeviceDescriptor descriptor(hueBridgeDeviceClassId, "Philips Hue Bridge", upnpDevice.hostAddress().toString());
            ParamList params;
            params.append(Param(hueBridgeBridgeNameParamTypeId, upnpDevice.friendlyName()));
            params.append(Param(hueBridgeBridgeHostParamTypeId, upnpDevice.hostAddress().toString()));
            params.append(Param(hueBridgeBridgeApiKeyParamTypeId, QString()));
            params.append(Param(hueBridgeBridgeMacParamTypeId, QString()));
            params.append(Param(hueBridgeBridgeIdParamTypeId, upnpDevice.serialNumber().toLower()));
            params.append(Param(hueBridgeBridgeZigbeeChannelParamTypeId, -1));
            descriptor.setParams(params);
            deviceDescriptors.append(descriptor);
        }
    }

    emit devicesDiscovered(hueBridgeDeviceClassId, deviceDescriptors);
}

void DevicePluginPhilipsHue::refreshLight(Device *device)
{
    HueLight *light = m_lights.key(device);

    QNetworkRequest request(QUrl("http://" + light->hostAddress().toString() + "/api/" + light->apiKey() + "/lights/" + QString::number(light->id())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_lightRefreshRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::refreshBridge(Device *device)
{
    if (m_bridgeRefreshRequests.values().contains(device)) {
        qCWarning(dcPhilipsHue()) << "Old bridge refresh call pending. Cleaning up and marking device as unreachable.";
        QNetworkReply *reply = m_bridgeRefreshRequests.key(device);
        reply->abort();
        m_bridgeRefreshRequests.remove(reply);
        reply->deleteLater();
        bridgeReachableChanged(device, false);
    }

    HueBridge *bridge = m_bridges.key(device);

    QNetworkRequest request(QUrl("http://" + bridge->hostAddress().toString() + "/api/" + bridge->apiKey() + "/config"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_bridgeRefreshRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::refreshLights(HueBridge *bridge)
{
    Device *device = m_bridges.value(bridge);

    QNetworkRequest request(QUrl("http://" + bridge->hostAddress().toString() + "/api/" + bridge->apiKey() + "/lights"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_lightsRefreshRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::refreshSensors(HueBridge *bridge)
{
    Device *device = m_bridges.value(bridge);

    QNetworkRequest request(QUrl("http://" + bridge->hostAddress().toString() + "/api/" + bridge->apiKey() + "/sensors"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_sensorsRefreshRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::discoverBridgeDevices(HueBridge *bridge)
{
    Device *device = m_bridges.value(bridge);
    qCDebug(dcPhilipsHue) << "Discover bridge devices" << bridge->hostAddress();

    QPair<QNetworkRequest, QByteArray> lightsRequest = bridge->createDiscoverLightsRequest();
    QNetworkReply *lightsReply = hardwareManager()->networkManager()->get(lightsRequest.first);
    connect(lightsReply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_bridgeLightsDiscoveryRequests.insert(lightsReply, device);

    QPair<QNetworkRequest, QByteArray> sensorsRequest = bridge->createSearchSensorsRequest();
    QNetworkReply *reply = hardwareManager()->networkManager()->get(sensorsRequest.first);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_bridgeSensorsDiscoveryRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::searchNewDevices(HueBridge *bridge)
{
    Device *device = m_bridges.value(bridge);
    qCDebug(dcPhilipsHue) << "Discover bridge devices" << bridge->hostAddress();

    QPair<QNetworkRequest, QByteArray> request = bridge->createSearchLightsRequest();
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_bridgeSearchDevicesRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::setLightName(Device *device, const QString &name)
{
    HueLight *light = m_lights.key(device);

    QVariantMap requestMap;
    requestMap.insert("name", name);
    QJsonDocument jsonDoc = QJsonDocument::fromVariant(requestMap);

    QNetworkRequest request(QUrl("http://" + light->hostAddress().toString() + "/api/" + light->apiKey() +
                                 "/lights/" + QString::number(light->id())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = hardwareManager()->networkManager()->put(request,jsonDoc.toJson());
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_lightSetNameRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::processNUpnpResponse(const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "N-UPNP discovery JSON error in response" << error.errorString();
        return;
    }

    QVariantList bridgeList = jsonDoc.toVariant().toList();

    QList<DeviceDescriptor> deviceDescriptors;
    foreach (const QVariant &bridgeVariant, bridgeList) {
        QVariantMap bridgeMap = bridgeVariant.toMap();
        DeviceDescriptor descriptor(hueBridgeDeviceClassId, "Philips Hue Bridge", bridgeMap.value("internalipaddress").toString());
        ParamList params;
        params.append(Param(hueBridgeBridgeNameParamTypeId, "Philips hue"));
        params.append(Param(hueBridgeBridgeHostParamTypeId, bridgeMap.value("internalipaddress").toString()));
        params.append(Param(hueBridgeBridgeApiKeyParamTypeId, QString()));
        params.append(Param(hueBridgeBridgeMacParamTypeId, QString()));
        params.append(Param(hueBridgeBridgeIdParamTypeId, bridgeMap.value("internalipaddress").toString().toLower()));
        params.append(Param(hueBridgeBridgeZigbeeChannelParamTypeId, -1));
        descriptor.setParams(params);
        deviceDescriptors.append(descriptor);
    }
    qCDebug(dcPhilipsHue) << "N-UPNP discover finished. Found" << deviceDescriptors.count() << "devices.";
    emit devicesDiscovered(hueBridgeDeviceClassId, deviceDescriptors);
}

void DevicePluginPhilipsHue::processBridgeLightDiscoveryResponse(Device *device, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Bridge light discovery json error in response" << error.errorString();
        return;
    }

    // check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to discover Hue Bridge lights:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to discover Hue Bridge lights: Invalid error message format";
        }
        return;
    }

    // create Lights if not already added
    QList<DeviceDescriptor> lightDescriptors;
    QList<DeviceDescriptor> whiteLightDescriptors;

    QVariantMap lightsMap = jsonDoc.toVariant().toMap();
    foreach (QString lightId, lightsMap.keys()) {
        QVariantMap lightMap = lightsMap.value(lightId).toMap();

        QString uuid = lightMap.value("uniqueid").toString();
        QString model = lightMap.value("modelid").toString();

        if (lightAlreadyAdded(uuid))
            continue;

        // check if this is a white light
        if (model == "LWB004" || model == "LWB006" || model == "LWB007") {
            DeviceDescriptor descriptor(hueWhiteLightDeviceClassId, "Philips Hue White Light", lightMap.value("name").toString());
            ParamList params;
            params.append(Param(hueWhiteLightNameParamTypeId, lightMap.value("name").toString()));
            params.append(Param(hueWhiteLightApiKeyParamTypeId, device->paramValue(hueBridgeBridgeApiKeyParamTypeId).toString()));
            params.append(Param(hueWhiteLightBridgeParamTypeId, device->id().toString()));
            params.append(Param(hueWhiteLightHostParamTypeId, device->paramValue(hueBridgeBridgeHostParamTypeId).toString()));
            params.append(Param(hueWhiteLightModelIdParamTypeId, model));
            params.append(Param(hueWhiteLightTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(hueWhiteLightUuidParamTypeId, uuid));
            params.append(Param(hueWhiteLightLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            whiteLightDescriptors.append(descriptor);

            qCDebug(dcPhilipsHue) << "Found new white light" << lightMap.value("name").toString() << model;

        } else {
            DeviceDescriptor descriptor(hueLightDeviceClassId, "Philips Hue Light", lightMap.value("name").toString());
            ParamList params;
            params.append(Param(hueLightNameParamTypeId, lightMap.value("name").toString()));
            params.append(Param(hueLightApiKeyParamTypeId, device->paramValue(hueBridgeBridgeApiKeyParamTypeId).toString()));
            params.append(Param(hueLightBridgeParamTypeId, device->id().toString()));
            params.append(Param(hueLightHostParamTypeId, device->paramValue(hueBridgeBridgeHostParamTypeId).toString()));
            params.append(Param(hueLightModelIdParamTypeId, model));
            params.append(Param(hueLightTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(hueLightUuidParamTypeId, uuid));
            params.append(Param(hueLightLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            lightDescriptors.append(descriptor);
            qCDebug(dcPhilipsHue) << "Found new color light" << lightMap.value("name").toString() << model;
        }
    }

    if (!lightDescriptors.isEmpty())
        emit autoDevicesAppeared(hueLightDeviceClassId, lightDescriptors);

    if (!whiteLightDescriptors.isEmpty())
        emit autoDevicesAppeared(hueWhiteLightDeviceClassId, whiteLightDescriptors);
}

void DevicePluginPhilipsHue::processBridgeSensorDiscoveryResponse(Device *device, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Bridge sensor discovery json error in response" << error.errorString();
        return;
    }

    // check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to discover Hue Bridge sensors:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to discover Hue Bridge sensors: Invalid error message format";
        }
        return;
    }

    // create sensors if not already added
    QList<DeviceDescriptor> sensorDescriptors;
    QVariantMap sensorsMap = jsonDoc.toVariant().toMap();
    foreach (QString sensorId, sensorsMap.keys()) {
        QVariantMap sensorMap = sensorsMap.value(sensorId).toMap();

        QString uuid = sensorMap.value("uniqueid").toString();
        QString model = sensorMap.value("modelid").toString();

        if (sensorAlreadyAdded(uuid))
            continue;

        // check if this is a white light
        if (model == "RWL021" || model == "RWL020") {
            DeviceDescriptor descriptor(hueRemoteDeviceClassId, "Philips Hue Remote", sensorMap.value("name").toString());
            ParamList params;
            params.append(Param(hueRemoteNameParamTypeId, sensorMap.value("name").toString()));
            params.append(Param(hueRemoteApiKeyParamTypeId, device->paramValue(hueBridgeBridgeApiKeyParamTypeId).toString()));
            params.append(Param(hueRemoteBridgeParamTypeId, device->id().toString()));
            params.append(Param(hueRemoteHostParamTypeId, device->paramValue(hueBridgeBridgeHostParamTypeId).toString()));
            params.append(Param(hueRemoteModelIdParamTypeId, model));
            params.append(Param(hueRemoteTypeParamTypeId, sensorMap.value("type").toString()));
            params.append(Param(hueRemoteUuidParamTypeId, uuid));
            params.append(Param(hueRemoteSensorIdParamTypeId, sensorId));
            descriptor.setParams(params);
            sensorDescriptors.append(descriptor);
            qCDebug(dcPhilipsHue) << "Found new remote" << sensorMap.value("name").toString() << model;
        }
    }

    if (!sensorDescriptors.isEmpty())
        emit autoDevicesAppeared(hueRemoteDeviceClassId, sensorDescriptors);

}

void DevicePluginPhilipsHue::processLightRefreshResponse(Device *device, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        return;
    }

    // check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to refresh Hue Light:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to refresh Hue Light: Invalid error message format";
        }
        return;
    }

    HueLight *light = m_lights.key(device);
    light->updateStates(jsonDoc.toVariant().toMap().value("state").toMap());
}

void DevicePluginPhilipsHue::processBridgeRefreshResponse(Device *device, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        return;
    }

    if (!jsonDoc.toVariant().toList().isEmpty()) {
        qCWarning(dcPhilipsHue) << "Failed to refresh Hue Bridge:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        bridgeReachableChanged(device, false);
        return;
    }

    QVariantMap configMap = jsonDoc.toVariant().toMap();

    // mark bridge as reachable
    bridgeReachableChanged(device, true);
    device->setStateValue(hueBridgeApiVersionStateTypeId, configMap.value("apiversion").toString());
    device->setStateValue(hueBridgeSoftwareVersionStateTypeId, configMap.value("swversion").toString());

    int updateStatus = configMap.value("swupdate").toMap().value("updatestate").toInt();
    switch (updateStatus) {
    case 0:
        device->setStateValue(hueBridgeUpdateStatusStateTypeId, "Up to date");
        break;
    case 1:
        device->setStateValue(hueBridgeUpdateStatusStateTypeId, "Downloading updates");
        break;
    case 2:
        device->setStateValue(hueBridgeUpdateStatusStateTypeId, "Updates ready to install");
        break;
    case 3:
        device->setStateValue(hueBridgeUpdateStatusStateTypeId, "Installing updates");
        break;
    default:
        break;
    }

    // do lights/sensor update right after successfull bridge update
    HueBridge *bridge = m_bridges.key(device);
    refreshLights(bridge);
}

void DevicePluginPhilipsHue::processLightsRefreshResponse(Device *device, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        return;
    }

    // check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to refresh Hue Lights:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to refresh Hue Lights: Invalid error message format";
        }
        return;
    }

    // Update light states
    QVariantMap lightsMap = jsonDoc.toVariant().toMap();
    foreach (const QString &lightId, lightsMap.keys()) {
        QVariantMap lightMap = lightsMap.value(lightId).toMap();
        // get the light of this bridge
        foreach (HueLight *light, m_lights.keys()) {
            if (light->id() == lightId.toInt() && light->bridgeId() == device->id()) {
                light->updateStates(lightMap.value("state").toMap());
            }
        }
    }

    if (!m_remotes.isEmpty())
        refreshSensors(m_bridges.key(device));
}

void DevicePluginPhilipsHue::processSensorsRefreshResponse(Device *device, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        return;
    }

    // check response error
    if (!jsonDoc.toVariant().toList().isEmpty()) {
        qCWarning(dcPhilipsHue) << "Failed to refresh Hue Sensors:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        return;
    }

    // Update sensor states
    QVariantMap sensorsMap = jsonDoc.toVariant().toMap();
    foreach (const QString &sensorId, sensorsMap.keys()) {
        QVariantMap sensorMap = sensorsMap.value(sensorId).toMap();
        foreach (HueRemote *remote, m_remotes.keys()) {
            if (remote->id() == sensorId.toInt() && remote->bridgeId() == device->id()) {
                //qCDebug(dcPhilipsHue) << "update remote" << remote->id() << remote->name();
                remote->updateStates(sensorMap.value("state").toMap(), sensorMap.value("config").toMap());
            }
        }
    }
}

void DevicePluginPhilipsHue::processSetNameResponse(Device *device, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
        return;
    }

    // check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to set name of Hue:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to set name of Hue: Invalid error message format";
        }
        emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
        return;
    }

    //emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusSuccess);

    if (device->deviceClassId() == hueLightDeviceClassId || device->deviceClassId() == hueWhiteLightDeviceClassId)
        refreshLight(device);

}

void DevicePluginPhilipsHue::processPairingResponse(PairingInfo *pairingInfo, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        emit pairingFinished(pairingInfo->pairingTransactionId(), DeviceManager::DeviceSetupStatusFailure);
        pairingInfo->deleteLater();
        return;
    }

    // check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to pair Hue Bridge:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to pair Hue Bridge: Invalid error message format";
        }
        emit pairingFinished(pairingInfo->pairingTransactionId(), DeviceManager::DeviceSetupStatusFailure);
        pairingInfo->deleteLater();
        return;
    }

    pairingInfo->setApiKey(jsonDoc.toVariant().toList().first().toMap().value("success").toMap().value("username").toString());

    qCDebug(dcPhilipsHue) << "Got api key from bridge:" << pairingInfo->apiKey();

    if (pairingInfo->apiKey().isEmpty()) {
        qCWarning(dcPhilipsHue) << "Failed to pair Hue Bridge: did not get any key from the bridge";
        emit pairingFinished(pairingInfo->pairingTransactionId(), DeviceManager::DeviceSetupStatusFailure);
        pairingInfo->deleteLater();
        return;
    }

    // Paired successfully, check bridge information
    QNetworkRequest request(QUrl("http://" + pairingInfo->host().toString() + "/api/" + pairingInfo->apiKey() + "/config"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_informationRequests.insert(reply, pairingInfo);
}

void DevicePluginPhilipsHue::processInformationResponse(PairingInfo *pairingInfo, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        emit pairingFinished(pairingInfo->pairingTransactionId(), DeviceManager::DeviceSetupStatusFailure);
        pairingInfo->deleteLater();
        return;
    }

    QVariantMap response = jsonDoc.toVariant().toMap();

    // check response error
    if (response.contains("error")) {
        qCWarning(dcPhilipsHue) << "Failed to get information from Hue Bridge:" << response.value("error").toMap().value("description").toString();
        emit pairingFinished(pairingInfo->pairingTransactionId(), DeviceManager::DeviceSetupStatusFailure);
        pairingInfo->deleteLater();
        return;
    }

    // create Bridge
    HueBridge *bridge = new HueBridge(this);
    bridge->setId(response.value("bridgeid").toString());
    bridge->setApiKey(pairingInfo->apiKey());
    bridge->setHostAddress(pairingInfo->host());
    bridge->setApiVersion(response.value("apiversion").toString());
    bridge->setSoftwareVersion(response.value("swversion").toString());
    bridge->setMacAddress(response.value("mac").toString());
    bridge->setName(response.value("name").toString());
    bridge->setZigbeeChannel(response.value("zigbeechannel").toInt());

    if (bridgeAlreadyAdded(bridge->id())) {
        qCWarning(dcPhilipsHue) << "Bridge with id" << bridge->id() << "already added.";
        emit pairingFinished(pairingInfo->pairingTransactionId(), DeviceManager::DeviceSetupStatusFailure);
        bridge->deleteLater();
        pairingInfo->deleteLater();
    }

    m_unconfiguredBridges.append(bridge);

    emit pairingFinished(pairingInfo->pairingTransactionId(), DeviceManager::DeviceSetupStatusSuccess);
    pairingInfo->deleteLater();
}

void DevicePluginPhilipsHue::processActionResponse(Device *device, const ActionId actionId, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorHardwareFailure);
        return;
    }

    // check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to execute Hue action:" << jsonDoc.toJson(); //jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to execute Hue action: Invalid error message format";
        }
        emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorHardwareFailure);
        return;
    }

    if (device->deviceClassId() != hueBridgeDeviceClassId)
        m_lights.key(device)->processActionResponse(jsonDoc.toVariant().toList());

    emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorNoError);
}

void DevicePluginPhilipsHue::bridgeReachableChanged(Device *device, const bool &reachable)
{
    if (reachable) {
        device->setStateValue(hueBridgeBridgeReachableStateTypeId, true);
    } else {
        // mark bridge and corresponding hue devices unreachable
        if (device->deviceClassId() == hueBridgeDeviceClassId) {
            device->setStateValue(hueBridgeBridgeReachableStateTypeId, false);

            foreach (HueLight *light, m_lights.keys()) {
                if (light->bridgeId() == device->id()) {
                    light->setReachable(false);
                    if (m_lights.value(light)->deviceClassId() == hueLightDeviceClassId) {
                        m_lights.value(light)->setStateValue(hueLightReachableStateTypeId, false);
                    } else if (m_lights.value(light)->deviceClassId() == hueWhiteLightDeviceClassId) {
                        m_lights.value(light)->setStateValue(hueWhiteLightHueReachableStateTypeId, false);
                    }
                }
            }

            foreach (HueRemote *remote, m_remotes.keys()) {
                if (remote->bridgeId() == device->id()) {
                    remote->setReachable(false);
                    m_remotes.value(remote)->setStateValue(hueRemoteHueReachableStateTypeId, false);
                }
            }
        }
    }

}

bool DevicePluginPhilipsHue::bridgeAlreadyAdded(const QString &id)
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == hueBridgeDeviceClassId) {
            if (device->paramValue(hueBridgeBridgeIdParamTypeId).toString() == id) {
                return true;
            }
        }
    }
    return false;
}

bool DevicePluginPhilipsHue::lightAlreadyAdded(const QString &uuid)
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == hueLightDeviceClassId) {
            if (device->paramValue(hueLightUuidParamTypeId).toString() == uuid) {
                return true;
            }
        } else if (device->deviceClassId() == hueWhiteLightDeviceClassId) {
            if (device->paramValue(hueWhiteLightUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }
    }
    return false;
}

bool DevicePluginPhilipsHue::sensorAlreadyAdded(const QString &uuid)
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == hueRemoteDeviceClassId) {
            if (device->paramValue(hueRemoteUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }
    }
    return false;
}

int DevicePluginPhilipsHue::brightnessToPercentage(int brightness)
{
    return qRound((100.0 * brightness) / 255.0);
}

int DevicePluginPhilipsHue::percentageToBrightness(int percentage)
{
    return qRound((255.0 * percentage) / 100.0);
}
