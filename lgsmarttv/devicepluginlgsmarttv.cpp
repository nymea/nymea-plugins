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

void DevicePluginLgSmartTv::discoverDevices(DeviceDiscoveryInfo *info)
{
    qCDebug(dcLgSmartTv()) << "Start discovering";
    UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices("udap:rootservice","UDAP/2.0");

    // Clean up in any case when the reply finishes
    connect(reply, &UpnpDiscoveryReply::finished, reply, &UpnpDiscoveryReply::deleteLater);

    // Connect reply to discovery info for rsults.
    connect(reply, &UpnpDiscoveryReply::finished, info, [this, info, reply](){

        if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
            qCWarning(dcLgSmartTv()) << "Upnp discovery error" << reply->error();
            return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error discovering devices. Please check your network connection."));
        }

        foreach (UpnpDeviceDescriptor upnpDeviceDescriptor, reply->deviceDescriptors()) {
            if (!upnpDeviceDescriptor.friendlyName().contains("LG") || !upnpDeviceDescriptor.deviceType().contains("TV"))
                continue;
            qCDebug(dcLgSmartTv) << upnpDeviceDescriptor;
            DeviceDescriptor descriptor(lgSmartTvDeviceClassId, "Lg Smart Tv", upnpDeviceDescriptor.modelName());
            ParamList params;
            params << Param(lgSmartTvDeviceNameParamTypeId, upnpDeviceDescriptor.friendlyName());
            params << Param(lgSmartTvDeviceUuidParamTypeId, upnpDeviceDescriptor.uuid());
            params << Param(lgSmartTvDeviceModelParamTypeId, upnpDeviceDescriptor.modelName());
            params << Param(lgSmartTvDeviceHostAddressParamTypeId, upnpDeviceDescriptor.hostAddress().toString());
            params << Param(lgSmartTvDevicePortParamTypeId, upnpDeviceDescriptor.port());
            descriptor.setParams(params);

            foreach (Device *existingDevice, myDevices()) {
                if (existingDevice->paramValue(lgSmartTvDeviceUuidParamTypeId).toString() == upnpDeviceDescriptor.uuid()) {
                    descriptor.setDeviceId(existingDevice->id());
                    break;
                }
            }
            info->addDeviceDescriptor(descriptor);
        }
        info->finish(Device::DeviceErrorNoError);
    });
}

void DevicePluginLgSmartTv::startPairing(DevicePairingInfo *info)
{
    QHostAddress host = QHostAddress(info->params().paramValue(lgSmartTvDeviceHostAddressParamTypeId).toString());
    int port = info->params().paramValue(lgSmartTvDevicePortParamTypeId).toInt();
    QPair<QNetworkRequest, QByteArray> request = TvDevice::createDisplayKeyRequest(host, port);

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, info, [info, reply](){
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
            qCWarning(dcLgSmartTv) << "display pin on TV request error:" << status << reply->errorString();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error connecting to the TV."));
            return;
        }

        info->finish(Device::DeviceErrorNoError, QT_TR_NOOP("Please enter the key displayed on the TV."));
    });
}

void DevicePluginLgSmartTv::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)

    QHostAddress host = QHostAddress(info->params().paramValue(lgSmartTvDeviceHostAddressParamTypeId).toString());
    int port = info->params().paramValue(lgSmartTvDevicePortParamTypeId).toInt();
    QPair<QNetworkRequest, QByteArray> request = TvDevice::createPairingRequest(host, port, secret);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, info, [this, info, reply, secret](){
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "pair TV request error:" << status << reply->errorString();
            return info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("Error pairing TV. Please try again."));
        }

        // End pairing before calling setupDevice, which will always try to pair
        QPair<QNetworkRequest, QByteArray> request = TvDevice::createEndPairingRequest(reply->request().url());

        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

        connect(reply, &QNetworkReply::finished, info, [this, info, reply, secret](){
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if(status != 200) {
                qCWarning(dcLgSmartTv) << "end pairing TV request error:" << status << reply->errorString();
                info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("Error pairing TV. Please try again."));
                return;
            }

            pluginStorage()->beginGroup(info->deviceId().toString());
            pluginStorage()->setValue("key", secret);
            pluginStorage()->endGroup();

            info->finish(Device::DeviceErrorNoError);
        });
    });
}

void DevicePluginLgSmartTv::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    qCDebug(dcLgSmartTv()) << "Setup LG smart TV" << device->name() << device->params();
    QHostAddress address = QHostAddress(device->paramValue(lgSmartTvDeviceHostAddressParamTypeId).toString());
    TvDevice *tvDevice = new TvDevice(address, device->paramValue(lgSmartTvDevicePortParamTypeId).toInt(), this);
    tvDevice->setUuid(device->paramValue(lgSmartTvDeviceUuidParamTypeId).toString());

    pluginStorage()->beginGroup(device->id().toString());
    QString key = pluginStorage()->value("key").toString();
    pluginStorage()->endGroup();

    tvDevice->setKey(key);

    connect(tvDevice, &TvDevice::stateChanged, this, &DevicePluginLgSmartTv::stateChanged);
    m_tvList.insert(tvDevice, device);


    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginLgSmartTv::onPluginTimer);
    }

    info->finish(Device::DeviceErrorNoError);
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

void DevicePluginLgSmartTv::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    TvDevice * tvDevice = m_tvList.key(device);

    if (!tvDevice->reachable()) {
        qCWarning(dcLgSmartTv) << "Device not reachable";
        return info->finish(Device::DeviceErrorHardwareNotAvailable);
    }

    QNetworkReply *reply = nullptr;

    if (action.actionTypeId() == lgSmartTvCommandVolumeUpActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::VolUp);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandVolumeDownActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::VolDown);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandMuteActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Mute);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandChannelUpActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::ChannelUp);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandChannelDownActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::ChannelDown);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandPowerOffActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Power);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandArrowUpActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Up);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandArrowDownActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Down);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandArrowLeftActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Left);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandArrowRightActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Right);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandOkActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Ok);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandBackActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Back);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandHomeActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Home);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandInputSourceActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::ExternalInput);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandExitActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Exit);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandInfoActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Info);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandMyAppsActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::MyApps);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    } else if(action.actionTypeId() == lgSmartTvCommandProgramListActionTypeId) {
        QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::ProgramList);
        reply = hardwareManager()->networkManager()->post(request.first, request.second);
    }

    if (!reply) {
        Q_ASSERT_X(false, "DevicePluginLGSmartTV", "Unhandled action " + info->action().actionTypeId().toString().toUtf8());
        info->finish(Device::DeviceErrorActionTypeNotFound);
        return;
    }

    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, info, [info, reply](){
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "Action request error:" << status << reply->errorString();
            info->finish(Device::DeviceErrorHardwareFailure);
            return;
        }

        info->finish(Device::DeviceErrorNoError);
    });
}


void DevicePluginLgSmartTv::pairTvDevice(Device *device)
{
    qCDebug(dcLgSmartTv()) << "Send pair request TV" << device->name();
    QHostAddress host = QHostAddress(device->paramValue(lgSmartTvDeviceHostAddressParamTypeId).toString());
    int port = device->paramValue(lgSmartTvDevicePortParamTypeId).toInt();

    pluginStorage()->beginGroup(device->id().toString());
    QString key = pluginStorage()->value("key").toString();
    pluginStorage()->endGroup();

    QPair<QNetworkRequest, QByteArray> request = TvDevice::createPairingRequest(host, port, key);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);

    connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, device, [this, device, reply]() {
        TvDevice *tv = m_tvList.key(device);

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "Pair TV request error:" << status << reply->errorString();
            tv->setPaired(false);
        } else {
            qCDebug(dcLgSmartTv) << "Paired TV successfully.";
            tv->setPaired(true);
            refreshTv(device);
        }
    });
}

void DevicePluginLgSmartTv::unpairTvDevice(Device *device)
{
    QHostAddress host = QHostAddress(device->paramValue(lgSmartTvDeviceHostAddressParamTypeId).toString());
    int port = device->paramValue(lgSmartTvDevicePortParamTypeId).toInt();
    QPair<QNetworkRequest, QByteArray> request = TvDevice::createEndPairingRequest(host, port);

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, this, [reply](){
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "End pairing TV (device deleted) request error:" << status << reply->errorString();
        } else {
            qCDebug(dcLgSmartTv) << "End pairing TV (device deleted) successfully.";
        }
    });
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

void DevicePluginLgSmartTv::onNetworkManagerReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    reply->deleteLater();

    if (m_volumeInfoRequests.keys().contains(reply)) {
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
