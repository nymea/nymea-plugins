/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "devicepluginlgsmarttv.h"

#include "devices/device.h"
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

}

void DevicePluginLgSmartTv::init()
{

}

Device::DeviceError DevicePluginLgSmartTv::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    Q_UNUSED(deviceClassId)

    qCDebug(dcLgSmartTv()) << "Start discovering";
    UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices("udap:rootservice","UDAP/2.0");
    connect(reply, &UpnpDiscoveryReply::finished, this, &DevicePluginLgSmartTv::onUpnpDiscoveryFinished);
    return Device::DeviceErrorAsync;
}

Device::DeviceSetupStatus DevicePluginLgSmartTv::setupDevice(Device *device)
{
    qCDebug(dcLgSmartTv()) << "Setup LG smart TV" << device->name() << device->params();
    QHostAddress address = QHostAddress(device->paramValue(lgSmartTvDeviceHostAddressParamTypeId).toString());
    TvDevice *tvDevice = new TvDevice(address, device->paramValue(lgSmartTvDevicePortParamTypeId).toInt(), this);
    tvDevice->setUuid(device->paramValue(lgSmartTvDeviceUuidParamTypeId).toString());

    // If the key is missing, this setup call comes from a pairing procedure
    if (device->paramValue(lgSmartTvDeviceKeyParamTypeId) == QString()) {
        // Check if we know the key from the pairing procedure
        if (!m_tvKeys.contains(device->paramValue(lgSmartTvDeviceUuidParamTypeId).toString())) {
            qCWarning(dcLgSmartTv) << "could not find any pairing key";
            return Device::DeviceSetupStatusFailure;
        }
        // Use the key from the pairing procedure
        QString key = m_tvKeys.value(device->paramValue(lgSmartTvDeviceUuidParamTypeId).toString());
        tvDevice->setKey(key);
        device->setParamValue(lgSmartTvDeviceKeyParamTypeId, key);
    } else {
        //Add the key for editing
        if (!m_tvKeys.contains(device->paramValue(lgSmartTvDeviceUuidParamTypeId).toString())) {
            m_tvKeys.insert(tvDevice->uuid(), tvDevice->key());
        }
    }

    connect(tvDevice, &TvDevice::stateChanged, this, &DevicePluginLgSmartTv::stateChanged);
    m_tvList.insert(tvDevice, device);


    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginLgSmartTv::onPluginTimer);
    }

    return Device::DeviceSetupStatusSuccess;
}

void DevicePluginLgSmartTv::deviceRemoved(Device *device)
{
    if (!m_tvList.values().contains(device))
        return;

    TvDevice *tvDevice= m_tvList.key(device);
    qCDebug(dcLgSmartTv) << "Remove device" << device->name();
    unpairTvDevice(device);
    m_tvList.remove(tvDevice);
    delete tvDevice;

    if (m_tvList.isEmpty() && m_pluginTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void DevicePluginLgSmartTv::postSetupDevice(Device *device)
{
    pairTvDevice(device);
}

Device::DeviceError DevicePluginLgSmartTv::executeAction(Device *device, const Action &action)
{
    TvDevice * tvDevice = m_tvList.key(device);

    if (!tvDevice->reachable()) {
        qCWarning(dcLgSmartTv) << "Device not reachable";
        return Device::DeviceErrorHardwareNotAvailable;
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
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorAsync;
}

DevicePairingInfo DevicePluginLgSmartTv::pairDevice(DevicePairingInfo &devicePairingInfo)
{

    QHostAddress host = QHostAddress(devicePairingInfo.params().paramValue(lgSmartTvDeviceHostAddressParamTypeId).toString());
    int port = devicePairingInfo.params().paramValue(lgSmartTvDevicePortParamTypeId).toInt();
    QPair<QNetworkRequest, QByteArray> request = TvDevice::createDisplayKeyRequest(host, port);

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, [this, reply, devicePairingInfo](){
        reply->deleteLater();
        DevicePairingInfo copy(devicePairingInfo); // lambda captures are read-only
        copy.setMessage(tr("Please enter the pairing key displayed on the Tv."));
        if (reply->error() != QNetworkReply::NoError) {
            copy.setStatus(Device::DeviceErrorAuthenticationFailure);
        } else {
            copy.setStatus(Device::DeviceErrorNoError);
        }
        emit pairingStarted(copy);
    });

    devicePairingInfo.setStatus(Device::DeviceErrorAsync);
    return devicePairingInfo;
}

DevicePairingInfo DevicePluginLgSmartTv::confirmPairing(DevicePairingInfo &devicePairingInfo, const QString &username, const QString &secret)
{
    Q_UNUSED(username)

    QHostAddress host = QHostAddress(devicePairingInfo.params().paramValue(lgSmartTvDeviceHostAddressParamTypeId).toString());
    int port = devicePairingInfo.params().paramValue(lgSmartTvDevicePortParamTypeId).toInt();
    QPair<QNetworkRequest, QByteArray> request = TvDevice::createPairingRequest(host, port, secret);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);

    m_setupPairingTv.insert(reply, devicePairingInfo);
    m_tvKeys.insert(devicePairingInfo.params().paramValue(lgSmartTvDeviceUuidParamTypeId).toString(), secret);

    devicePairingInfo.setStatus(Device::DeviceErrorAsync);
    return devicePairingInfo;
}

void DevicePluginLgSmartTv::pairTvDevice(Device *device)
{
    qCDebug(dcLgSmartTv()) << "Send pair request TV" << device->name();
    QHostAddress host = QHostAddress(device->paramValue(lgSmartTvDeviceHostAddressParamTypeId).toString());
    int port = device->paramValue(lgSmartTvDevicePortParamTypeId).toInt();
    QString key = device->paramValue(lgSmartTvDeviceKeyParamTypeId).toString();
    QPair<QNetworkRequest, QByteArray> request = TvDevice::createPairingRequest(host, port, key);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);

    m_asyncSetup.insert(reply, device);
}

void DevicePluginLgSmartTv::unpairTvDevice(Device *device)
{
    QHostAddress host = QHostAddress(device->paramValue(lgSmartTvDeviceHostAddressParamTypeId).toString());
    int port = device->paramValue(lgSmartTvDevicePortParamTypeId).toInt();
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
        params.append(Param(lgSmartTvDeviceNameParamTypeId, upnpDeviceDescriptor.friendlyName()));
        params.append(Param(lgSmartTvDeviceUuidParamTypeId, upnpDeviceDescriptor.uuid()));
        params.append(Param(lgSmartTvDeviceModelParamTypeId, upnpDeviceDescriptor.modelName()));
        params.append(Param(lgSmartTvDeviceHostAddressParamTypeId, upnpDeviceDescriptor.hostAddress().toString()));
        params.append(Param(lgSmartTvDevicePortParamTypeId, upnpDeviceDescriptor.port()));
        params.append(Param(lgSmartTvDeviceKeyParamTypeId, QString()));
        descriptor.setParams(params);

        foreach (Device *existingDevice, myDevices()) {
            if (existingDevice->paramValue(lgSmartTvDeviceUuidParamTypeId).toString() == upnpDeviceDescriptor.uuid()) {
                descriptor.setDeviceId(existingDevice->id());
                break;
            }
        }
        deviceDescriptors.append(descriptor);
    }
    emit devicesDiscovered(lgSmartTvDeviceClassId, deviceDescriptors);

}

void DevicePluginLgSmartTv::onNetworkManagerReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    reply->deleteLater();


    if (m_setupPairingTv.keys().contains(reply)) {
        DevicePairingInfo pairingInfo = m_setupPairingTv.take(reply);
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "pair TV request error:" << status << reply->errorString();
            pairingInfo.setStatus(Device::DeviceErrorAuthenticationFailure);
            emit pairingFinished(pairingInfo);
        } else {
            // End pairing before calling setupDevice, which will always try to pair
            QPair<QNetworkRequest, QByteArray> request = TvDevice::createEndPairingRequest(reply->request().url());
            QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
            m_setupEndPairingTv.insert(reply, pairingInfo);
        }
    } else if (m_setupEndPairingTv.keys().contains(reply)) {
        DevicePairingInfo pairingInfo = m_setupEndPairingTv.take(reply);
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "end pairing TV request error:" << status << reply->errorString();
            pairingInfo.setStatus(Device::DeviceErrorAuthenticationFailure);
            emit pairingFinished(pairingInfo);
        } else {
            pairingInfo.setStatus(Device::DeviceErrorNoError);
            emit pairingFinished(pairingInfo);
        }
    } else if (m_asyncSetup.keys().contains(reply)) {
        Device *device = m_asyncSetup.take(reply);
        TvDevice *tv = m_tvList.key(device);
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "Pair TV request error:" << status << reply->errorString();
            tv->setPaired(false);
        } else {
            qCDebug(dcLgSmartTv) << "Paired TV successfully.";
            tv->setPaired(true);
            refreshTv(device);
        }
    } else if (m_deleteTv.contains(reply)) {
        m_deleteTv.removeAll(reply);
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "End pairing TV (device deleted) request error:" << status << reply->errorString();
        } else {
            qCDebug(dcLgSmartTv) << "End pairing TV (device deleted) successfully.";
        }
    } else if (m_volumeInfoRequests.keys().contains(reply)) {
        Device *device = m_volumeInfoRequests.take(reply);
        TvDevice *tv = m_tvList.key(device);
        if(status != 200) {
            tv->setReachable(false);
            qCWarning(dcLgSmartTv) << "Volume information request error:" << status << reply->errorString();
            if (status == 401) {
                qCDebug(dcLgSmartTv()) << status << reply->errorString();
                pairTvDevice(device);
            }
        } else {
            tv->setReachable(true);
            tv->onVolumeInformationUpdate(reply->readAll());

            // Check channel information
            QNetworkReply *channelReply = hardwareManager()->networkManager()->get(tv->createChannelInformationRequest());
            connect(channelReply, &QNetworkReply::finished, this, &DevicePluginLgSmartTv::onNetworkManagerReplyFinished);
            m_channelInfoRequests.insert(channelReply, device);
        }
    } else if (m_channelInfoRequests.keys().contains(reply)) {
        Device *device = m_channelInfoRequests.take(reply);
        TvDevice *tv = m_tvList.key(device);
        if(status != 200) {
            tv->setReachable(false);
            qCWarning(dcLgSmartTv) << "Channel information request error:" << status << reply->errorString();
            if (status == 401) {
                qCDebug(dcLgSmartTv()) << status << reply->errorString();
                pairTvDevice(device);
            }
        } else {
            tv->setReachable(true);
            tv->onChannelInformationUpdate(reply->readAll());
        }
    } else if (m_asyncActions.keys().contains(reply)) {
        ActionId actionId = m_asyncActions.value(reply);
        if(status != 200) {
            emit actionExecutionFinished(actionId, Device::DeviceErrorHardwareNotAvailable);
            qCWarning(dcLgSmartTv) << "Action request error:" << status << reply->errorString();
        } else {
            emit actionExecutionFinished(actionId, Device::DeviceErrorNoError);
        }
    }
}

void DevicePluginLgSmartTv::stateChanged()
{
    TvDevice *tvDevice = static_cast<TvDevice*>(sender());
    Device *device = m_tvList.value(tvDevice);

    device->setStateValue(lgSmartTvConnectedStateTypeId, tvDevice->reachable());
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
