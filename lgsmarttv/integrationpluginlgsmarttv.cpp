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

#include "integrationpluginlgsmarttv.h"

#include "integrations/thing.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"
#include "network/upnp/upnpdiscovery.h"
#include "hardwaremanager.h"

#include <QDebug>

IntegrationPluginLgSmartTv::IntegrationPluginLgSmartTv()
{
}

IntegrationPluginLgSmartTv::~IntegrationPluginLgSmartTv()
{

}

void IntegrationPluginLgSmartTv::init()
{

}

void IntegrationPluginLgSmartTv::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == lgSmartTvThingClassId) {

        qCDebug(dcLgSmartTv()) << "Start discovering";
        UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices("udap:rootservice","UDAP/2.0");

        // Clean up in any case when the reply finishes
        connect(reply, &UpnpDiscoveryReply::finished, reply, &UpnpDiscoveryReply::deleteLater);

        // Connect reply to discovery info for rsults.
        connect(reply, &UpnpDiscoveryReply::finished, info, [this, info, reply](){

            if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
                qCWarning(dcLgSmartTv()) << "Upnp discovery error" << reply->error();
                return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error discovering devices. Please check your network connection."));
            }

            foreach (UpnpDeviceDescriptor upnpDeviceDescriptor, reply->deviceDescriptors()) {
                if (!upnpDeviceDescriptor.friendlyName().contains("LG") || !upnpDeviceDescriptor.deviceType().contains("TV"))
                    continue;

                qCDebug(dcLgSmartTv) << upnpDeviceDescriptor;
                ThingDescriptor descriptor(lgSmartTvThingClassId, "Lg Smart Tv", upnpDeviceDescriptor.modelName());
                ParamList params;
                params << Param(lgSmartTvThingNameParamTypeId, upnpDeviceDescriptor.friendlyName());
                params << Param(lgSmartTvThingUuidParamTypeId, upnpDeviceDescriptor.uuid());
                params << Param(lgSmartTvThingModelParamTypeId, upnpDeviceDescriptor.modelName());
                params << Param(lgSmartTvThingHostAddressParamTypeId, upnpDeviceDescriptor.hostAddress().toString());
                params << Param(lgSmartTvThingPortParamTypeId, upnpDeviceDescriptor.port());
                descriptor.setParams(params);

                foreach (Thing *existingThing, myThings()) {
                    if (existingThing->paramValue(lgSmartTvThingUuidParamTypeId).toString() == upnpDeviceDescriptor.uuid()) {
                        descriptor.setThingId(existingThing->id());
                        break;
                    }
                }
                info->addThingDescriptor(descriptor);
            }
            info->finish(Thing::ThingErrorNoError);
        });

    } else if (info->thingClassId() == webosTvThingClassId) {
        qCDebug(dcLgSmartTv()) << "Start discovering Webos tv";
        UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices("");
        connect(reply, &UpnpDiscoveryReply::finished, reply, &UpnpDiscoveryReply::deleteLater);
        connect(reply, &UpnpDiscoveryReply::finished, info, [this, info, reply](){
            Q_UNUSED(this)

            if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
                qCWarning(dcLgSmartTv()) << "Upnp discovery error" << reply->error();
                return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error discovering devices. Please check your network connection."));
            }

            foreach (UpnpDeviceDescriptor upnpDeviceDescriptor, reply->deviceDescriptors()) {
                qCDebug(dcLgSmartTv) << upnpDeviceDescriptor;

                // FIXME: check what are the propper upnp settings for discovering UPnP
            }

            info->finish(Thing::ThingErrorNoError);
        });
    }

}

void IntegrationPluginLgSmartTv::startPairing(ThingPairingInfo *info)
{
    QHostAddress host = QHostAddress(info->params().paramValue(lgSmartTvThingHostAddressParamTypeId).toString());
    int port = info->params().paramValue(lgSmartTvThingPortParamTypeId).toInt();
    QPair<QNetworkRequest, QByteArray> request = TvDevice::createDisplayKeyRequest(host, port);

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, info, [info, reply](){
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
            qCWarning(dcLgSmartTv) << "display pin on TV request error:" << status << reply->errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error connecting to the TV."));
            return;
        }

        info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter the key displayed on the TV."));
    });
}

void IntegrationPluginLgSmartTv::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)

    QHostAddress host = QHostAddress(info->params().paramValue(lgSmartTvThingHostAddressParamTypeId).toString());
    int port = info->params().paramValue(lgSmartTvThingPortParamTypeId).toInt();
    QPair<QNetworkRequest, QByteArray> request = TvDevice::createPairingRequest(host, port, secret);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, info, [this, info, reply, secret](){
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "pair TV request error:" << status << reply->errorString();
            return info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Error pairing TV. Please try again."));
        }

        // End pairing before calling setupDevice, which will always try to pair
        QPair<QNetworkRequest, QByteArray> request = TvDevice::createEndPairingRequest(reply->request().url());

        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

        connect(reply, &QNetworkReply::finished, info, [this, info, reply, secret](){
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if(status != 200) {
                qCWarning(dcLgSmartTv) << "end pairing TV request error:" << status << reply->errorString();
                info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Error pairing TV. Please try again."));
                return;
            }

            pluginStorage()->beginGroup(info->thingId().toString());
            pluginStorage()->setValue("key", secret);
            pluginStorage()->endGroup();

            info->finish(Thing::ThingErrorNoError);
        });
    });
}

void IntegrationPluginLgSmartTv::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == lgSmartTvThingClassId) {
        qCDebug(dcLgSmartTv()) << "Setup LG smart TV" << thing->name() << thing->params();
        QHostAddress address = QHostAddress(thing->paramValue(lgSmartTvThingHostAddressParamTypeId).toString());
        TvDevice *tvDevice = new TvDevice(address, thing->paramValue(lgSmartTvThingPortParamTypeId).toInt(), this);
        tvDevice->setUuid(thing->paramValue(lgSmartTvThingUuidParamTypeId).toString());

        pluginStorage()->beginGroup(thing->id().toString());
        QString key = pluginStorage()->value("key").toString();
        pluginStorage()->endGroup();

        tvDevice->setKey(key);

        connect(tvDevice, &TvDevice::stateChanged, this, &IntegrationPluginLgSmartTv::stateChanged);
        m_tvList.insert(tvDevice, thing);


        if (!m_pluginTimer) {
            m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(5);
            connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginLgSmartTv::onPluginTimer);
        }

        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (thing->thingClassId() == webosTvThingClassId) {
        QHostAddress hostAddress = QHostAddress(thing->paramValue(webosTvThingHostAddressParamTypeId).toString());
        WebosConnection *webosConnection = new WebosConnection(this);
        webosConnection->setHostAddress(hostAddress);

        connect(webosConnection, &WebosConnection::connectedChanged, this, [thing](bool connected){
            thing->setStateValue(webosTvConnectedStateTypeId, connected);
        });

        m_webosTvs.insert(webosConnection, thing);
        webosConnection->connectTv();
    }
}

void IntegrationPluginLgSmartTv::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == lgSmartTvThingClassId) {
        if (!m_tvList.values().contains(thing))
            return;

        TvDevice *tvDevice= m_tvList.key(thing);
        qCDebug(dcLgSmartTv) << "Removing device" << thing->name();
        unpairTvDevice(thing);
        m_tvList.remove(tvDevice);
        delete tvDevice;

        if (m_tvList.isEmpty() && m_pluginTimer) {
            hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
            m_pluginTimer = nullptr;
        }
        return;
    }

    if (thing->thingClassId() == webosTvThingClassId) {
        WebosConnection *connection = m_webosTvs.key(thing);
        m_webosTvs.remove(connection);
        delete connection;
    }
}

void IntegrationPluginLgSmartTv::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == lgSmartTvThingClassId) {
        pairTvDevice(thing);
    }
}

void IntegrationPluginLgSmartTv::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == lgSmartTvThingClassId) {

        TvDevice * tvDevice = m_tvList.key(thing);
        if (!tvDevice->reachable()) {
            qCWarning(dcLgSmartTv) << "Device not reachable";
            return info->finish(Thing::ThingErrorHardwareNotAvailable);
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
            info->finish(Thing::ThingErrorActionTypeNotFound);
            return;
        }

        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply](){
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if(status != 200) {
                qCWarning(dcLgSmartTv) << "Action request error:" << status << reply->errorString();
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            info->finish(Thing::ThingErrorNoError);
        });
    }
}


void IntegrationPluginLgSmartTv::pairTvDevice(Thing *thing)
{
    qCDebug(dcLgSmartTv()) << "Send pair request TV" << thing->name();
    QHostAddress host = QHostAddress(thing->paramValue(lgSmartTvThingHostAddressParamTypeId).toString());
    int port = thing->paramValue(lgSmartTvThingPortParamTypeId).toInt();

    pluginStorage()->beginGroup(thing->id().toString());
    QString key = pluginStorage()->value("key").toString();
    pluginStorage()->endGroup();

    QPair<QNetworkRequest, QByteArray> request = TvDevice::createPairingRequest(host, port, key);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [this, thing, reply]() {
        TvDevice *tv = m_tvList.key(thing);
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "Pair TV request error:" << status << reply->errorString();
            tv->setPaired(false);
        } else {
            qCDebug(dcLgSmartTv) << "Paired TV successfully.";
            tv->setPaired(true);
            refreshTv(thing);
        }
    });
}

void IntegrationPluginLgSmartTv::unpairTvDevice(Thing *thing)
{
    QHostAddress host = QHostAddress(thing->paramValue(lgSmartTvThingHostAddressParamTypeId).toString());
    int port = thing->paramValue(lgSmartTvThingPortParamTypeId).toInt();
    QPair<QNetworkRequest, QByteArray> request = TvDevice::createEndPairingRequest(host, port);

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, this, [reply](){
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if(status != 200) {
            qCWarning(dcLgSmartTv) << "End pairing TV (thing deleted) request error:" << status << reply->errorString();
        } else {
            qCDebug(dcLgSmartTv) << "End pairing TV (thing deleted) successfully.";
        }
    });
}

void IntegrationPluginLgSmartTv::refreshTv(Thing *thing)
{
    TvDevice *tv = m_tvList.key(thing);
    // check volume information
    QNetworkReply *volumeReply = hardwareManager()->networkManager()->get(tv->createVolumeInformationRequest());
    connect(volumeReply, &QNetworkReply::finished, this, &IntegrationPluginLgSmartTv::onNetworkManagerReplyFinished);
    m_volumeInfoRequests.insert(volumeReply, thing);

}

void IntegrationPluginLgSmartTv::onPluginTimer()
{
    foreach (Thing *thing, m_tvList.values()) {
        TvDevice *tv = m_tvList.key(thing);
        if (tv->paired()) {
            refreshTv(thing);
        } else {
            pairTvDevice(thing);
        }
    }
}

void IntegrationPluginLgSmartTv::onNetworkManagerReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    reply->deleteLater();

    if (m_volumeInfoRequests.keys().contains(reply)) {
        Thing *thing = m_volumeInfoRequests.take(reply);
        TvDevice *tv = m_tvList.key(thing);
        if(status != 200) {
            tv->setReachable(false);
            qCWarning(dcLgSmartTv) << "Volume information request error:" << status << reply->errorString();
            if (status == 401) {
                qCDebug(dcLgSmartTv()) << status << reply->errorString();
                pairTvDevice(thing);
            }
        } else {
            tv->setReachable(true);
            tv->onVolumeInformationUpdate(reply->readAll());

            // Check channel information
            QNetworkReply *channelReply = hardwareManager()->networkManager()->get(tv->createChannelInformationRequest());
            connect(channelReply, &QNetworkReply::finished, this, &IntegrationPluginLgSmartTv::onNetworkManagerReplyFinished);
            m_channelInfoRequests.insert(channelReply, thing);
        }
    } else if (m_channelInfoRequests.keys().contains(reply)) {
        Thing *thing = m_channelInfoRequests.take(reply);
        TvDevice *tv = m_tvList.key(thing);
        if(status != 200) {
            tv->setReachable(false);
            qCWarning(dcLgSmartTv) << "Channel information request error:" << status << reply->errorString();
            if (status == 401) {
                qCDebug(dcLgSmartTv()) << status << reply->errorString();
                pairTvDevice(thing);
            }
        } else {
            tv->setReachable(true);
            tv->onChannelInformationUpdate(reply->readAll());
        }
    }
}

void IntegrationPluginLgSmartTv::stateChanged()
{
    TvDevice *tvDevice = static_cast<TvDevice*>(sender());
    Thing *thing = m_tvList.value(tvDevice);

    thing->setStateValue(lgSmartTvConnectedStateTypeId, tvDevice->reachable());
    thing->setStateValue(lgSmartTvTv3DModeStateTypeId, tvDevice->is3DMode());
    thing->setStateValue(lgSmartTvTvVolumeLevelStateTypeId, tvDevice->volumeLevel());
    thing->setStateValue(lgSmartTvTvMuteStateTypeId, tvDevice->mute());
    thing->setStateValue(lgSmartTvTvChannelTypeStateTypeId, tvDevice->channelType());
    thing->setStateValue(lgSmartTvTvChannelNameStateTypeId, tvDevice->channelName());
    thing->setStateValue(lgSmartTvTvChannelNumberStateTypeId, tvDevice->channelNumber());
    thing->setStateValue(lgSmartTvTvProgramNameStateTypeId, tvDevice->programName());
    thing->setStateValue(lgSmartTvTvInputSourceIndexStateTypeId, tvDevice->inputSourceIndex());
    thing->setStateValue(lgSmartTvTvInputSourceLabelNameStateTypeId, tvDevice->inputSourceLabelName());
}
