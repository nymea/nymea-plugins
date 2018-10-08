/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2014 Michael Zanetti <michael_zanetti@gmx.net>           *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *  Copyright (C) 2018 Michael Zanetti <michael.zanetti@guh.io>            *
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

/*!
    \page philipshue.html
    \title Philips hue
    \brief Plugin for the Philips Hue lighting system.

    \ingroup plugins
    \ingroup nymea-plugins

    This plugin allows to interact with the \l{http://www2.meethue.com/}{Philips hue} bridge. Each light bulp connected to the bridge
    will appear automatically in the system, once the bridge is added to nymea.

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
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer1Sec);
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5Sec);
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer15Sec);
}

void DevicePluginPhilipsHue::init()
{
    m_pluginTimer1Sec = hardwareManager()->pluginTimerManager()->registerTimer(1);
    connect(m_pluginTimer1Sec, &PluginTimer::timeout, this, [this]() {
        // refresh sensors every second
        if (m_remotes.isEmpty()) {
            return;
        }
        foreach (HueBridge *bridge, m_bridges.keys()) {
            refreshSensors(bridge);
        }
    });
    m_pluginTimer5Sec = hardwareManager()->pluginTimerManager()->registerTimer(5);
    connect(m_pluginTimer5Sec, &PluginTimer::timeout, this, [this]() {
        // refresh lights every 5 seconds
        foreach (HueBridge *bridge, m_bridges.keys()) {
            refreshLights(bridge);
        }
    });
    m_pluginTimer15Sec = hardwareManager()->pluginTimerManager()->registerTimer(15);
    connect(m_pluginTimer15Sec, &PluginTimer::timeout, this, [this]() {
        // refresh bridges every 15 seconds
        foreach (Device *device, m_bridges.values()) {
            refreshBridge(device);
        }
    });

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
    if (device->deviceClassId() == bridgeDeviceClassId) {
        // unconfigured bridges (from pairing)
        foreach (HueBridge *b, m_unconfiguredBridges) {
            if (b->hostAddress().toString() == device->paramValue(bridgeDeviceHostParamTypeId).toString()) {
                m_unconfiguredBridges.removeAll(b);
                qCDebug(dcPhilipsHue) << "Setup unconfigured Hue Bridge" << b->name();
                // set data which was not known during discovery
                device->setParamValue(bridgeDeviceNameParamTypeId, b->name());
                device->setParamValue(bridgeDeviceApiKeyParamTypeId, b->apiKey());
                device->setParamValue(bridgeDeviceZigbeeChannelParamTypeId, b->zigbeeChannel());
                device->setParamValue(bridgeDeviceIdParamTypeId, b->id());
                device->setParamValue(bridgeDeviceMacParamTypeId, b->macAddress());
                m_bridges.insert(b, device);
                device->setStateValue(bridgeConnectedStateTypeId, true);
                discoverBridgeDevices(b);
                return DeviceManager::DeviceSetupStatusSuccess;
            }
        }

        // loaded bridge
        qCDebug(dcPhilipsHue) << "Setup Hue Bridge" << device->params();

        HueBridge *bridge = new HueBridge(this);
        bridge->setId(device->paramValue(bridgeDeviceIdParamTypeId).toString());
        bridge->setApiKey(device->paramValue(bridgeDeviceApiKeyParamTypeId).toString());
        bridge->setHostAddress(QHostAddress(device->paramValue(bridgeDeviceHostParamTypeId).toString()));
        bridge->setName(device->paramValue(bridgeDeviceNameParamTypeId).toString());
        bridge->setMacAddress(device->paramValue(bridgeDeviceMacParamTypeId).toString());
        bridge->setZigbeeChannel(device->paramValue(bridgeDeviceZigbeeChannelParamTypeId).toInt());
        m_bridges.insert(bridge, device);

        discoverBridgeDevices(bridge);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    // hue color light
    if (device->deviceClassId() == lightDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue color light" << device->params();

        HueBridge *bridge = m_bridges.key(myDevices().findById(device->parentId()));
        HueLight *hueLight = new HueLight(this);
        hueLight->setHostAddress(bridge->hostAddress());
        hueLight->setApiKey(bridge->apiKey());
        hueLight->setId(device->paramValue(lightDeviceLightIdParamTypeId).toInt());
        hueLight->setName(device->paramValue(lightDeviceNameParamTypeId).toString());
        hueLight->setModelId(device->paramValue(lightDeviceModelIdParamTypeId).toString());
        hueLight->setUuid(device->paramValue(lightDeviceUuidParamTypeId).toString());
        hueLight->setType(device->paramValue(lightDeviceTypeParamTypeId).toString());

        connect(hueLight, &HueLight::stateChanged, this, &DevicePluginPhilipsHue::lightStateChanged);
        m_lights.insert(hueLight, device);

        device->setName(hueLight->name());

        refreshLight(device);
        setLightName(device, device->paramValue(lightDeviceNameParamTypeId).toString());

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    // hue white light
    if (device->deviceClassId() == whiteLightDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue white light" << device->params();

        HueBridge *bridge = m_bridges.key(myDevices().findById(device->parentId()));
        HueLight *hueLight = new HueLight(this);
        hueLight->setHostAddress(bridge->hostAddress());
        hueLight->setApiKey(bridge->apiKey());
        hueLight->setId(device->paramValue(whiteLightDeviceLightIdParamTypeId).toInt());
        hueLight->setName(device->paramValue(whiteLightDeviceNameParamTypeId).toString());
        hueLight->setModelId(device->paramValue(whiteLightDeviceModelIdParamTypeId).toString());
        hueLight->setUuid(device->paramValue(whiteLightDeviceUuidParamTypeId).toString());
        hueLight->setType(device->paramValue(whiteLightDeviceTypeParamTypeId).toString());

        connect(hueLight, &HueLight::stateChanged, this, &DevicePluginPhilipsHue::lightStateChanged);

        device->setName(hueLight->name());

        m_lights.insert(hueLight, device);
        refreshLight(device);

        setLightName(device, device->paramValue(whiteLightDeviceNameParamTypeId).toString());
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    // hue remote
    if (device->deviceClassId() == remoteDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue remote" << device->params() << device->deviceClassId();

        HueBridge *bridge = m_bridges.key(myDevices().findById(device->parentId()));
        HueRemote *hueRemote = new HueRemote(this);
        hueRemote->setHostAddress(bridge->hostAddress());
        hueRemote->setApiKey(bridge->apiKey());
        hueRemote->setId(device->paramValue(remoteDeviceSensorIdParamTypeId).toInt());
        hueRemote->setName(device->paramValue(remoteDeviceNameParamTypeId).toString());
        hueRemote->setModelId(device->paramValue(remoteDeviceModelIdParamTypeId).toString());
        hueRemote->setType(device->paramValue(remoteDeviceTypeParamTypeId).toString());
        hueRemote->setUuid(device->paramValue(remoteDeviceUuidParamTypeId).toString());

        device->setName(hueRemote->name());

        connect(hueRemote, &HueRemote::stateChanged, this, &DevicePluginPhilipsHue::remoteStateChanged);
        connect(hueRemote, &HueRemote::buttonPressed, this, &DevicePluginPhilipsHue::onRemoteButtonEvent);

        m_remotes.insert(hueRemote, device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    // hue tap
    if (device->deviceClassId() == tapDeviceClassId) {
        HueRemote *hueTap = new HueRemote(this);
        hueTap->setName(device->name());
        hueTap->setId(device->paramValue(tapDeviceSensorIdParamTypeId).toInt());
        hueTap->setModelId(device->paramValue(tapDeviceModelIdParamTypeId).toString());

        connect(hueTap, &HueRemote::stateChanged, this, &DevicePluginPhilipsHue::remoteStateChanged);
        connect(hueTap, &HueRemote::buttonPressed, this, &DevicePluginPhilipsHue::onRemoteButtonEvent);

        m_remotes.insert(hueTap, device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    qCWarning(dcPhilipsHue()) << "Unhandled setupDevice call" << device->deviceClassId();

    return DeviceManager::DeviceSetupStatusFailure;
}

void DevicePluginPhilipsHue::deviceRemoved(Device *device)
{
    abortRequests(m_lightRefreshRequests, device);
    abortRequests(m_lightSetNameRequests, device);
    abortRequests(m_bridgeRefreshRequests, device);
    abortRequests(m_lightsRefreshRequests, device);
    abortRequests(m_sensorsRefreshRequests, device);
    abortRequests(m_bridgeLightsDiscoveryRequests, device);
    abortRequests(m_bridgeSensorsDiscoveryRequests, device);
    abortRequests(m_bridgeSearchDevicesRequests, device);

    if (device->deviceClassId() == bridgeDeviceClassId) {
        HueBridge *bridge = m_bridges.key(device);
        m_bridges.remove(bridge);
        bridge->deleteLater();
    }

    if (device->deviceClassId() == lightDeviceClassId || device->deviceClassId() == whiteLightDeviceClassId) {
        HueLight *light = m_lights.key(device);
        m_lights.remove(light);
        light->deleteLater();
    }

    if (device->deviceClassId() == remoteDeviceClassId || device->deviceClassId() == tapDeviceClassId) {
        HueRemote *remote = m_remotes.key(device);
        m_remotes.remove(remote);
        remote->deleteLater();
    }
}

DeviceManager::DeviceSetupStatus DevicePluginPhilipsHue::confirmPairing(const PairingTransactionId &pairingTransactionId, const DeviceClassId &deviceClassId, const ParamList &params, const QString &secret)
{
    Q_UNUSED(secret)

    if (deviceClassId != bridgeDeviceClassId)
        return DeviceManager::DeviceSetupStatusFailure;

    PairingInfo *pairingInfo = new PairingInfo(this);
    pairingInfo->setPairingTransactionId(pairingTransactionId);
    pairingInfo->setHost(QHostAddress(params.paramValue(bridgeDeviceHostParamTypeId).toString()));

    QVariantMap deviceTypeParam;
    deviceTypeParam.insert("devicetype", "nymea");

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

//    qCDebug(dcPhilipsHue()) << "Hue reply:" << status << reply->error() << reply->errorString();

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
            if (device->stateValue(bridgeConnectedStateTypeId).toBool()) {
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
            if (device->stateValue(lightConnectedStateTypeId).toBool()) {
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
            if (device->stateValue(remoteConnectedStateTypeId).toBool() || device->stateValue(tapConnectedStateTypeId).toBool()) {
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
    } else {
        qCWarning(dcPhilipsHue()) << "Unhandled bridge reply" << reply->error() << reply->readAll();
    }
    reply->deleteLater();
}

DeviceManager::DeviceError DevicePluginPhilipsHue::executeAction(Device *device, const Action &action)
{
    qCDebug(dcPhilipsHue) << "Execute action" << action.actionTypeId() << action.params();

    // color light
    if (device->deviceClassId() == lightDeviceClassId) {
        HueLight *light = m_lights.key(device);

        if (!light->reachable()) {
            qCWarning(dcPhilipsHue) << "Light" << light->name() << "not reachable";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (action.actionTypeId() == lightPowerActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetPowerRequest(action.param(lightPowerActionPowerParamTypeId).value().toBool());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == lightColorActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetColorRequest(action.param(lightColorActionColorParamTypeId).value().value<QColor>());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply,QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == lightBrightnessActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetBrightnessRequest(percentageToBrightness(action.param(lightBrightnessActionBrightnessParamTypeId).value().toInt()));
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == lightEffectActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetEffectRequest(action.param(lightEffectActionEffectParamTypeId).value().toString());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == lightAlertActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createFlashRequest(action.param(lightAlertActionAlertParamTypeId).value().toString());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == lightColorTemperatureActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetTemperatureRequest(action.param(lightColorTemperatureActionColorTemperatureParamTypeId).value().toInt());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    // white light
    if (device->deviceClassId() == whiteLightDeviceClassId) {
        HueLight *light = m_lights.key(device);

        if (!light->reachable()) {
            qCWarning(dcPhilipsHue) << "Light" << light->name() << "not reachable";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (action.actionTypeId() == whiteLightPowerActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetPowerRequest(action.param(whiteLightPowerActionPowerParamTypeId).value().toBool());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == whiteLightBrightnessActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetBrightnessRequest(percentageToBrightness(action.param(whiteLightBrightnessActionBrightnessParamTypeId).value().toInt()));
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == whiteLightAlertActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createFlashRequest(action.param(whiteLightAlertActionAlertParamTypeId).value().toString());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == bridgeDeviceClassId) {
        HueBridge *bridge = m_bridges.key(device);
        if (!device->stateValue(bridgeConnectedStateTypeId).toBool()) {
            qCWarning(dcPhilipsHue) << "Bridge" << bridge->hostAddress().toString() << "not reachable";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (action.actionTypeId() == bridgeSearchNewDevicesActionTypeId) {
            searchNewDevices(bridge);
            return DeviceManager::DeviceErrorNoError;
        } else if (action.actionTypeId() == bridgeCheckForUpdatesActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = bridge->createCheckUpdatesRequest();
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == bridgeUpgradeActionTypeId) {
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

    if (device->deviceClassId() == lightDeviceClassId) {
        device->setStateValue(lightConnectedStateTypeId, light->reachable());
        device->setStateValue(lightColorStateTypeId, QVariant::fromValue(light->color()));
        device->setStateValue(lightPowerStateTypeId, light->power());
        device->setStateValue(lightBrightnessStateTypeId, brightnessToPercentage(light->brightness()));
        device->setStateValue(lightColorTemperatureStateTypeId, light->ct());
        device->setStateValue(lightEffectStateTypeId, light->effect());
    } else if (device->deviceClassId() == whiteLightDeviceClassId) {
        device->setStateValue(whiteLightConnectedStateTypeId, light->reachable());
        device->setStateValue(whiteLightPowerStateTypeId, light->power());
        device->setStateValue(whiteLightBrightnessStateTypeId, brightnessToPercentage(light->brightness()));
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
    if (device->deviceClassId() == tapDeviceClassId) {
        device->setStateValue(tapConnectedStateTypeId, remote->reachable());
    } else {
        device->setStateValue(remoteConnectedStateTypeId, remote->reachable());
        device->setStateValue(remoteBatteryLevelStateTypeId, remote->battery());
        device->setStateValue(remoteBatteryCriticalStateTypeId, remote->battery() < 5);
    }
}

void DevicePluginPhilipsHue::onRemoteButtonEvent(const int &buttonCode)
{
    HueRemote *remote = static_cast<HueRemote *>(sender());

    EventTypeId id;
    Param param;

    // TODO: Legacy events should be removed eventually
    switch (buttonCode) {
    case HueRemote::OnPressed:
        param = Param(remotePressedEventButtonNameParamTypeId, "ON");
        id = remotePressedEventTypeId;
        break;
    case HueRemote::OnLongPressed:
        emitEvent(Event(remoteOnLongPressedEventTypeId, m_remotes.value(remote)->id()));
        param = Param(remoteLongPressedEventButtonNameParamTypeId, "ON");
        id = remoteLongPressedEventTypeId;
        break;
    case HueRemote::DimUpPressed:
        emitEvent(Event(remoteDimUpPressedEventTypeId, m_remotes.value(remote)->id()));
        param = Param(remotePressedEventButtonNameParamTypeId, "DIM UP");
        id = remotePressedEventTypeId;
        break;
    case HueRemote::DimUpLongPressed:
        emitEvent(Event(remoteDimUpLongPressedEventTypeId, m_remotes.value(remote)->id()));
        param = Param(remoteLongPressedEventButtonNameParamTypeId, "DIM UP");
        id = remoteLongPressedEventTypeId;
        break;
    case HueRemote::DimDownPressed:
        emitEvent(Event(remoteDimDownPressedEventTypeId, m_remotes.value(remote)->id()));
        param = Param(remotePressedEventButtonNameParamTypeId, "DIM DOWN");
        id = remotePressedEventTypeId;
        break;
    case HueRemote::DimDownLongPressed:
        emitEvent(Event(remoteDimDownLongPressedEventTypeId, m_remotes.value(remote)->id()));
        param = Param(remoteLongPressedEventButtonNameParamTypeId, "DIM DOWN");
        id = remoteLongPressedEventTypeId;
        break;
    case HueRemote::OffPressed:
        emitEvent(Event(remoteOffPressedEventTypeId, m_remotes.value(remote)->id()));
        param = Param(remotePressedEventButtonNameParamTypeId, "OFF");
        id = remotePressedEventTypeId;
        break;
    case HueRemote::OffLongPressed:
        emitEvent(Event(remoteOffLongPressedEventTypeId, m_remotes.value(remote)->id()));
        param = Param(remoteLongPressedEventButtonNameParamTypeId, "OFF");
        id = remoteLongPressedEventTypeId;
        break;
    case HueRemote::TapButton1Pressed:
        param = Param(tapPressedEventButtonNameParamTypeId, "•");
        id = tapPressedEventTypeId;
        break;
    case HueRemote::TapButton2Pressed:
        param = Param(tapPressedEventButtonNameParamTypeId, "••");
        id = tapPressedEventTypeId;
        break;
    case HueRemote::TapButton3Pressed:
        param = Param(tapPressedEventButtonNameParamTypeId, "•••");
        id = tapPressedEventTypeId;
        break;
    case HueRemote::TapButton4Pressed:
        param = Param(tapPressedEventButtonNameParamTypeId, "•••••");
        id = tapPressedEventTypeId;
        break;
    default:
        break;
    }
    emitEvent(Event(id, m_remotes.value(remote)->id(), ParamList() << param));
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
            DeviceDescriptor descriptor(bridgeDeviceClassId, "Philips Hue Bridge", upnpDevice.hostAddress().toString());
            ParamList params;
            params.append(Param(bridgeDeviceNameParamTypeId, upnpDevice.friendlyName()));
            params.append(Param(bridgeDeviceHostParamTypeId, upnpDevice.hostAddress().toString()));
            params.append(Param(bridgeDeviceApiKeyParamTypeId, QString()));
            params.append(Param(bridgeDeviceMacParamTypeId, QString()));
            params.append(Param(bridgeDeviceIdParamTypeId, upnpDevice.serialNumber().toLower()));
            params.append(Param(bridgeDeviceZigbeeChannelParamTypeId, -1));
            descriptor.setParams(params);
            deviceDescriptors.append(descriptor);
        }
    }

    emit devicesDiscovered(bridgeDeviceClassId, deviceDescriptors);
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
    HueBridge *bridge = m_bridges.key(device);
//    qCDebug(dcPhilipsHue()) << "refreshing bridge";

    QNetworkRequest request(QUrl("http://" + bridge->hostAddress().toString() + "/api/" + bridge->apiKey() + "/config"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_bridgeRefreshRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::refreshLights(HueBridge *bridge)
{
    Device *device = m_bridges.value(bridge);
//    qCDebug(dcPhilipsHue()) << "refreshing lights";

    QNetworkRequest request(QUrl("http://" + bridge->hostAddress().toString() + "/api/" + bridge->apiKey() + "/lights"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_lightsRefreshRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::refreshSensors(HueBridge *bridge)
{
    Device *device = m_bridges.value(bridge);
//    qCDebug(dcPhilipsHue()) << "refreshing sensors";

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
        DeviceDescriptor descriptor(bridgeDeviceClassId, "Philips Hue Bridge", bridgeMap.value("internalipaddress").toString());
        ParamList params;
        params.append(Param(bridgeDeviceNameParamTypeId, "Philips hue"));
        params.append(Param(bridgeDeviceHostParamTypeId, bridgeMap.value("internalipaddress").toString()));
        params.append(Param(bridgeDeviceApiKeyParamTypeId, QString()));
        params.append(Param(bridgeDeviceMacParamTypeId, QString()));
        params.append(Param(bridgeDeviceIdParamTypeId, bridgeMap.value("internalipaddress").toString().toLower()));
        params.append(Param(bridgeDeviceZigbeeChannelParamTypeId, -1));
        descriptor.setParams(params);
        deviceDescriptors.append(descriptor);
    }
    qCDebug(dcPhilipsHue) << "N-UPNP discover finished. Found" << deviceDescriptors.count() << "devices.";
    emit devicesDiscovered(bridgeDeviceClassId, deviceDescriptors);
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
            DeviceDescriptor descriptor(whiteLightDeviceClassId, "Philips Hue White Light", lightMap.value("name").toString(), device->id());
            ParamList params;
            params.append(Param(whiteLightDeviceNameParamTypeId, lightMap.value("name").toString()));
            params.append(Param(whiteLightDeviceModelIdParamTypeId, model));
            params.append(Param(whiteLightDeviceTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(whiteLightDeviceUuidParamTypeId, uuid));
            params.append(Param(whiteLightDeviceLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            whiteLightDescriptors.append(descriptor);

            qCDebug(dcPhilipsHue) << "Found new white light" << lightMap.value("name").toString() << model;

        } else {
            DeviceDescriptor descriptor(lightDeviceClassId, "Philips Hue Light", lightMap.value("name").toString(), device->id());
            ParamList params;
            params.append(Param(lightDeviceNameParamTypeId, lightMap.value("name").toString()));
            params.append(Param(lightDeviceModelIdParamTypeId, model));
            params.append(Param(lightDeviceTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(lightDeviceUuidParamTypeId, uuid));
            params.append(Param(lightDeviceLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            lightDescriptors.append(descriptor);
            qCDebug(dcPhilipsHue) << "Found new color light" << lightMap.value("name").toString() << model;
        }
    }

    if (!lightDescriptors.isEmpty())
        emit autoDevicesAppeared(lightDeviceClassId, lightDescriptors);

    if (!whiteLightDescriptors.isEmpty())
        emit autoDevicesAppeared(whiteLightDeviceClassId, whiteLightDescriptors);
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
    QVariantMap sensorsMap = jsonDoc.toVariant().toMap();
    foreach (QString sensorId, sensorsMap.keys()) {
        QVariantMap sensorMap = sensorsMap.value(sensorId).toMap();

        QString uuid = sensorMap.value("uniqueid").toString();
        QString model = sensorMap.value("modelid").toString();

        if (sensorAlreadyAdded(uuid))
            continue;

        if (model == "RWL021" || model == "RWL020") {
            DeviceDescriptor descriptor(remoteDeviceClassId, "Philips Hue Remote", sensorMap.value("name").toString(), device->id());
            ParamList params;
            params.append(Param(remoteDeviceNameParamTypeId, sensorMap.value("name").toString()));
            params.append(Param(remoteDeviceModelIdParamTypeId, model));
            params.append(Param(remoteDeviceTypeParamTypeId, sensorMap.value("type").toString()));
            params.append(Param(remoteDeviceUuidParamTypeId, uuid));
            params.append(Param(remoteDeviceSensorIdParamTypeId, sensorId));
            descriptor.setParams(params);
            emit autoDevicesAppeared(remoteDeviceClassId, {descriptor});
            qCDebug(dcPhilipsHue) << "Found new remote" << sensorMap.value("name").toString() << model;
        } else if (model == "ZGPSWITCH") {
            DeviceDescriptor descriptor(tapDeviceClassId, "Hue Tap", sensorMap.value("name").toString(), device->id());
            ParamList params;
            params.append(Param(tapDeviceUuidParamTypeId, uuid));
            params.append(Param(tapDeviceModelIdParamTypeId, model));
            params.append(Param(tapDeviceSensorIdParamTypeId, sensorId));
            descriptor.setParams(params);
            emit autoDevicesAppeared(tapDeviceClassId, {descriptor});
            qCDebug(dcPhilipsHue()) << "Found hue tap:" << sensorMap << tapDeviceClassId;
        } else {
            qCDebug(dcPhilipsHue()) << "Found unknown sensor:" << model;
        }
    }
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
    device->setStateValue(bridgeApiVersionStateTypeId, configMap.value("apiversion").toString());
    device->setStateValue(bridgeSoftwareVersionStateTypeId, configMap.value("swversion").toString());

    int updateStatus = configMap.value("swupdate").toMap().value("updatestate").toInt();
    switch (updateStatus) {
    case 0:
        device->setStateValue(bridgeUpdateStatusStateTypeId, "Up to date");
        break;
    case 1:
        device->setStateValue(bridgeUpdateStatusStateTypeId, "Downloading updates");
        break;
    case 2:
        device->setStateValue(bridgeUpdateStatusStateTypeId, "Updates ready to install");
        break;
    case 3:
        device->setStateValue(bridgeUpdateStatusStateTypeId, "Installing updates");
        break;
    default:
        break;
    }
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
            if (light->id() == lightId.toInt() && m_lights.value(light)->parentId() == device->id()) {
                light->updateStates(lightMap.value("state").toMap());
            }
        }
    }
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
            if (remote->id() == sensorId.toInt() && m_remotes.value(remote)->parentId() == device->id()) {
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

    if (device->deviceClassId() == lightDeviceClassId || device->deviceClassId() == whiteLightDeviceClassId)
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

    if (device->deviceClassId() != bridgeDeviceClassId)
        m_lights.key(device)->processActionResponse(jsonDoc.toVariant().toList());

    emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorNoError);
}

void DevicePluginPhilipsHue::bridgeReachableChanged(Device *device, const bool &reachable)
{
    if (reachable) {
        device->setStateValue(bridgeConnectedStateTypeId, true);
    } else {
        // mark bridge and corresponding hue devices unreachable
        if (device->deviceClassId() == bridgeDeviceClassId) {
            device->setStateValue(bridgeConnectedStateTypeId, false);

            foreach (HueLight *light, m_lights.keys()) {
                if (m_lights.value(light)->parentId() == device->id()) {
                    light->setReachable(false);
                    if (m_lights.value(light)->deviceClassId() == lightDeviceClassId) {
                        m_lights.value(light)->setStateValue(lightConnectedStateTypeId, false);
                    } else if (m_lights.value(light)->deviceClassId() == whiteLightDeviceClassId) {
                        m_lights.value(light)->setStateValue(whiteLightConnectedStateTypeId, false);
                    }
                }
            }

            foreach (HueRemote *remote, m_remotes.keys()) {
                if (m_remotes.value(remote)->parentId() == device->id()) {
                    remote->setReachable(false);
                    if (m_remotes.value(remote)->deviceClassId() == remoteDeviceClassId) {
                        m_remotes.value(remote)->setStateValue(remoteConnectedStateTypeId, false);
                    } else if (m_remotes.value(remote)->deviceClassId() == tapDeviceClassId) {
                        m_remotes.value(remote)->setStateValue(tapConnectedStateTypeId, false);
                    }
                }
            }
        }
    }

}

bool DevicePluginPhilipsHue::bridgeAlreadyAdded(const QString &id)
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == bridgeDeviceClassId) {
            if (device->paramValue(bridgeDeviceIdParamTypeId).toString() == id) {
                return true;
            }
        }
    }
    return false;
}

bool DevicePluginPhilipsHue::lightAlreadyAdded(const QString &uuid)
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == lightDeviceClassId) {
            if (device->paramValue(lightDeviceUuidParamTypeId).toString() == uuid) {
                return true;
            }
        } else if (device->deviceClassId() == whiteLightDeviceClassId) {
            if (device->paramValue(whiteLightDeviceUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }
    }
    return false;
}

bool DevicePluginPhilipsHue::sensorAlreadyAdded(const QString &uuid)
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == remoteDeviceClassId) {
            if (device->paramValue(remoteDeviceUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }
        if (device->deviceClassId() == tapDeviceClassId) {
            if (device->paramValue(tapDeviceUuidParamTypeId).toString() == uuid) {
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

void DevicePluginPhilipsHue::abortRequests(QHash<QNetworkReply *, Device *> requestList, Device *device)
{
    foreach (QNetworkReply* reply, requestList.keys()) {
        if (requestList.value(reply) == device) {
            reply->abort();
        }
    }
}
