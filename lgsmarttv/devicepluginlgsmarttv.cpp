/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
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
    \page lgsmarttv.html
    \title LG Smart Tv
    \brief Plugin for LG smart TVs.

    \ingroup plugins
    \ingroup guh-plugins

    This plugin allows to interact with \l{http://www.lg.com/us/experience-tvs/smart-tv}{LG Smart Tv's}
    with the \l{http://developer.lgappstv.com/TV_HELP/index.jsp?topic=%2Flge.tvsdk.references.book%2Fhtml%2FUDAP%2FUDAP%2FLG+UDAP+2+0+Protocol+Specifications.htm}{LG UDAP 2.0 Protocol Specifications}.

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/lgsmarttv/devicepluginlgsmarttv.json
*/

#include "devicepluginlgsmarttv.h"

#include "plugin/device.h"
#include "devicemanager.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"
#include "network/upnp/upnpdiscovery.h"
#include "hardwaremanager.h"

#include <QDebug>

DevicePluginLgSmartTv::DevicePluginLgSmartTv()
{
}

DevicePluginLgSmartTv::~DevicePluginLgSmartTv()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void DevicePluginLgSmartTv::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(15);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginLgSmartTv::onPluginTimer);
}

DeviceManager::DeviceError DevicePluginLgSmartTv::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    Q_UNUSED(deviceClassId)

    qCDebug(dcLgSmartTv()) << "Start discovering";
    UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices("udap:rootservice","UDAP/2.0");
    connect(reply, &UpnpDiscoveryReply::finished, this, &DevicePluginLgSmartTv::onUpnpDiscoveryFinished);
    return DeviceManager::DeviceErrorAsync;
}

DeviceManager::DeviceSetupStatus DevicePluginLgSmartTv::setupDevice(Device *device)
{
    if (device->deviceClassId() != lgSmartTvDeviceClassId) {
        return DeviceManager::DeviceSetupStatusFailure;
    }

    TvDevice *tvDevice = new TvDevice(QHostAddress(device->paramValue(lgSmartTvHostAddressParamTypeId).toString()),
                                      device->paramValue(lgSmartTvPortParamTypeId).toInt(), this);
    tvDevice->setUuid(device->paramValue(lgSmartTvUuidParamTypeId).toString());

    // if the key is missing, this setup call comes from a pairing procedure
    if (device->paramValue(lgSmartTvKeyParamTypeId) == QString()) {
        // check if we know the key from the pairing procedure
        if (!m_tvKeys.contains(device->paramValue(lgSmartTvUuidParamTypeId).toString())) {
            qCWarning(dcLgSmartTv) << "could not find any pairing key";
            return DeviceManager::DeviceSetupStatusFailure;
        }
        // use the key from the pairing procedure
        QString key = m_tvKeys.value(device->paramValue(lgSmartTvUuidParamTypeId).toString());

        tvDevice->setKey(key);
        device->setParamValue(lgSmartTvKeyParamTypeId, key);
    } else {
        // add the key for editing
        if (!m_tvKeys.contains(device->paramValue(lgSmartTvUuidParamTypeId).toString())) {
            m_tvKeys.insert(tvDevice->uuid(), tvDevice->key());
        }
    }

    connect(tvDevice, &TvDevice::stateChanged, this, &DevicePluginLgSmartTv::stateChanged);

    m_tvList.insert(tvDevice, device);
    pairTvDevice(device, true);

    return DeviceManager::DeviceSetupStatusAsync;
}

void DevicePluginLgSmartTv::deviceRemoved(Device *device)
{
    if (!m_tvList.values().contains(device)) {
        return;
    }

    TvDevice *tvDevice= m_tvList.key(device);
    qCDebug(dcLgSmartTv) << "Remove device" << device->name();
    unpairTvDevice(device);
    m_tvList.remove(tvDevice);
    delete tvDevice;
}

DeviceManager::DeviceError DevicePluginLgSmartTv::executeAction(Device *device, const Action &action)
{
    TvDevice * tvDevice = m_tvList.key(device);

    if (!tvDevice->reachable()) {
        qCWarning(dcLgSmartTv) << "Device not reachable";
        return DeviceManager::DeviceErrorHardwareNotAvailable;
    }

    if (action.actionTypeId() == lgSmartTvCommandVolumeUpActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::VolUp);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandVolumeDownActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::VolDown);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandMuteActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Mute);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandChannelUpActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::ChannelUp);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandChannelDownActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::ChannelDown);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandPowerOffActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Power);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandArrowUpActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Up);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandArrowDownActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Down);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandArrowLeftActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Left);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandArrowRightActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Right);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandOkActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Ok);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandBackActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Back);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandHomeActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Home);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandInputSourceActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::ExternalInput);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandExitActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Exit);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandInfoActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Info);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandMyAppsActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::MyApps);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else if(action.actionTypeId() == lgSmartTvCommandProgramListActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::ProgramList);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
        m_asyncActions.insert(reply, action.id());
    } else {
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorAsync;
}

DeviceManager::DeviceError DevicePluginLgSmartTv::displayPin(const PairingTransactionId &pairingTransactionId, const DeviceDescriptor &deviceDescriptor)
{
    Q_UNUSED(pairingTransactionId)

    QHostAddress host = QHostAddress(deviceDescriptor.params().paramValue(lgSmartTvHostAddressParamTypeId).toString());
    int port = deviceDescriptor.params().paramValue(lgSmartTvPortParamTypeId).toInt();
    QPair<QNetworkRequest, QByteArray> request = TvDevice::createDisplayKeyRequest(host, port);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);

    m_showPinReply.append(reply);
    return DeviceManager::DeviceErrorNoError;
}

DeviceManager::DeviceSetupStatus DevicePluginLgSmartTv::confirmPairing(const PairingTransactionId &pairingTransactionId, const DeviceClassId &deviceClassId, const ParamList &params, const QString &secret)
{
    Q_UNUSED(deviceClassId)

    QHostAddress host = QHostAddress(params.paramValue(lgSmartTvHostAddressParamTypeId).toString());
    int port = params.paramValue(lgSmartTvPortParamTypeId).toInt();
    QPair<QNetworkRequest, QByteArray> request = TvDevice::createPairingRequest(host, port, secret);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);

    m_setupPairingTv.insert(reply, pairingTransactionId);
    m_tvKeys.insert(params.paramValue(lgSmartTvUuidParamTypeId).toString(), secret);

    return DeviceManager::DeviceSetupStatusAsync;
}

void DevicePluginLgSmartTv::pairTvDevice(Device *device, const bool &setup)
{
    QHostAddress host = QHostAddress(device->paramValue(lgSmartTvHostAddressParamTypeId).toString());
    int port = device->paramValue(lgSmartTvPortParamTypeId).toInt();
    QString key = device->paramValue(lgSmartTvKeyParamTypeId).toString();
    QPair<QNetworkRequest, QByteArray> request = TvDevice::createPairingRequest(host, port, key);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);

    if (setup) {
        m_asyncSetup.insert(reply, device);
    } else {
        m_pairRequests.insert(reply, device);
    }
}

void DevicePluginLgSmartTv::unpairTvDevice(Device *device)
{
    QHostAddress host = QHostAddress(device->paramValue(lgSmartTvHostAddressParamTypeId).toString());
    int port = device->paramValue(lgSmartTvPortParamTypeId).toInt();
    QPair<QNetworkRequest, QByteArray> request = TvDevice::createEndPairingRequest(host, port);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);

    m_deleteTv.append(reply);
}

void DevicePluginLgSmartTv::refreshTv(Device *device)
{
    TvDevice *tv = m_tvList.key(device);
    // check volume information
    QNetworkReply *volumeReply = hardwareManager()->networkManager()->get(tv->createVolumeInformationRequest());
    connect(volumeReply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
    m_volumeInfoRequests.insert(volumeReply, device);

    // check channel information
    QNetworkReply *channelReply = hardwareManager()->networkManager()->get(tv->createChannelInformationRequest());
    connect(channelReply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
    m_channelInfoRequests.insert(channelReply, device);
}

void DevicePluginLgSmartTv::onPluginTimer()
{
    foreach (Device *device, m_tvList.values()) {
        TvDevice *tv = m_tvList.key(device);
        if (tv->paired()) {
            refreshTv(device);
        } else {
            pairTvDevice(device);
        }
    }
}

void DevicePluginLgSmartTv::onUpnpDiscoveryFinished()
{
    qCDebug(dcLgSmartTv()) << "Upnp discovery finished";

    UpnpDiscoveryReply *reply = static_cast<UpnpDiscoveryReply *>(sender());
    if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
        qCWarning(dcLgSmartTv()) << "Upnp discovery error" << reply->error();
    }
    reply->deleteLater();

    QList<DeviceDescriptor> deviceDescriptors;
    foreach (UpnpDeviceDescriptor upnpDeviceDescriptor, reply->deviceDescriptors()) {
        if (!upnpDeviceDescriptor.friendlyName().contains("LG") || !upnpDeviceDescriptor.deviceType().contains("TV"))
            continue;
        qCDebug(dcLgSmartTv) << upnpDeviceDescriptor;
        DeviceDescriptor descriptor(lgSmartTvDeviceClassId, "Lg Smart Tv", upnpDeviceDescriptor.modelName());
        ParamList params;
        params.append(Param(lgSmartTvNameParamTypeId, upnpDeviceDescriptor.friendlyName()));
        params.append(Param(lgSmartTvUuidParamTypeId, upnpDeviceDescriptor.uuid()));
        params.append(Param(lgSmartTvModelParamTypeId, upnpDeviceDescriptor.modelName()));
        params.append(Param(lgSmartTvHostAddressParamTypeId, upnpDeviceDescriptor.hostAddress().toString()));
        params.append(Param(lgSmartTvPortParamTypeId, upnpDeviceDescriptor.port()));
        params.append(Param(lgSmartTvKeyParamTypeId, QString()));
        descriptor.setParams(params);
        deviceDescriptors.append(descriptor);
    }
    emit devicesDiscovered(lgSmartTvDeviceClassId, deviceDescriptors);

}

void DevicePluginLgSmartTv::onNetworkManagerReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (m_showPinReply.contains(reply)) {
        m_showPinReply.removeAll(reply);
        if (status != 200) {
            qCWarning(dcLgSmartTv) << "display pin on TV request error:" << status << reply->errorString();
        }
    } else if (m_setupPairingTv.keys().contains(reply)) {
        PairingTransactionId pairingTransactionId = m_setupPairingTv.take(reply);
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "pair TV request error:" << status << reply->errorString();
            emit pairingFinished(pairingTransactionId, DeviceManager::DeviceSetupStatusFailure);
        } else {
            // End pairing before calling setupDevice, which will always try to pair
            QPair<QNetworkRequest, QByteArray> request = TvDevice::createEndPairingRequest(reply->request().url());
            QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
            m_setupEndPairingTv.insert(reply, pairingTransactionId);
        }
    } else if (m_setupEndPairingTv.keys().contains(reply)) {
        PairingTransactionId pairingTransactionId = m_setupEndPairingTv.take(reply);
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "end pairing TV request error:" << status << reply->errorString();
            emit pairingFinished(pairingTransactionId, DeviceManager::DeviceSetupStatusFailure);
        } else {
            emit pairingFinished(pairingTransactionId, DeviceManager::DeviceSetupStatusSuccess);
        }
    } else if (m_asyncSetup.keys().contains(reply)) {
        Device *device = m_asyncSetup.take(reply);
        TvDevice *tv = m_tvList.key(device);
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "Pair TV request error:" << status << reply->errorString();
            tv->setPaired(false);
            emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
        } else {
            qCDebug(dcLgSmartTv) << "Paired TV successfully.";
            tv->setPaired(true);
            refreshTv(device);
            emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusSuccess);
        }
    } else if (m_deleteTv.contains(reply)) {
        m_deleteTv.removeAll(reply);
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "Rnd pairing TV (device deleted) request error:" << status << reply->errorString();
        } else {
            qCDebug(dcLgSmartTv) << "End pairing TV (device deleted) successfully.";
        }
    } else if (m_volumeInfoRequests.keys().contains(reply)) {
        Device *device = m_volumeInfoRequests.take(reply);
        TvDevice *tv = m_tvList.key(device);
        if(status != 200) {
            tv->setReachable(false);
            qCWarning(dcLgSmartTv) << "Volume information request error:" << status << reply->errorString();
        } else {
            tv->setReachable(true);
            tv->onVolumeInformationUpdate(reply->readAll());
        }
    } else if (m_channelInfoRequests.keys().contains(reply)) {
        Device *device = m_channelInfoRequests.take(reply);
        TvDevice *tv = m_tvList.key(device);
        if(status != 200) {
            tv->setReachable(false);
            qCWarning(dcLgSmartTv) << "Channel information request error:" << status << reply->errorString();
        } else {
            tv->setReachable(true);
            tv->onChannelInformationUpdate(reply->readAll());
        }
    } else if (m_asyncActions.keys().contains(reply)) {
        ActionId actionId = m_asyncActions.value(reply);
        if(status != 200) {
            emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorHardwareNotAvailable);
            qCWarning(dcLgSmartTv) << "Action request error:" << status << reply->errorString();
        } else {
            emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorNoError);
        }
    }
    reply->deleteLater();
}

void DevicePluginLgSmartTv::stateChanged()
{
    TvDevice *tvDevice = static_cast<TvDevice*>(sender());
    Device *device = m_tvList.value(tvDevice);

    device->setStateValue(lgSmartTvReachableStateTypeId, tvDevice->reachable());
    device->setStateValue(lgSmartTvTv3DModeStateTypeId, tvDevice->is3DMode());
    device->setStateValue(lgSmartTvTvVolumeLevelStateTypeId, tvDevice->volumeLevel());
    device->setStateValue(lgSmartTvTvMuteStateTypeId, tvDevice->mute());
    device->setStateValue(lgSmartTvTvChannelTypeStateTypeId, tvDevice->channelType());
    device->setStateValue(lgSmartTvTvChannelNameStateTypeId, tvDevice->channelName());
    device->setStateValue(lgSmartTvTvChannelNumberStateTypeId, tvDevice->channelNumber());
    device->setStateValue(lgSmartTvTvProgramNameStateTypeId, tvDevice->programName());
    device->setStateValue(lgSmartTvTvInputSourceIndexStateTypeId, tvDevice->inputSourceIndex());
    device->setStateValue(lgSmartTvTvInputSourceLabelNameStateTypeId, tvDevice->inputSourceLabelName());
}
