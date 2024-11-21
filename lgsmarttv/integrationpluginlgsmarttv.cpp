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
        qCDebug(dcLgSmartTv()) << "Start discovering LG UDAP 2.0 smart TVs...";
        UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices("udap:rootservice","UDAP/2.0");
        connect(reply, &UpnpDiscoveryReply::finished, reply, &UpnpDiscoveryReply::deleteLater);
        connect(reply, &UpnpDiscoveryReply::finished, info, [this, info, reply](){
            if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
                qCWarning(dcLgSmartTv()) << "Upnp discovery error" << reply->error();
                return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error discovering devices. Please check your network connection."));
            }

            foreach (UpnpDeviceDescriptor upnpDeviceDescriptor, reply->deviceDescriptors()) {
                if (!upnpDeviceDescriptor.friendlyName().contains("LG") || !upnpDeviceDescriptor.deviceType().contains("TV"))
                    continue;

                qCDebug(dcLgSmartTv) << upnpDeviceDescriptor;
                ThingDescriptor descriptor(lgSmartTvThingClassId, "LG Smart Tv", upnpDeviceDescriptor.modelName());
                ParamList params;
                params << Param(lgSmartTvThingNameParamTypeId, upnpDeviceDescriptor.friendlyName());
                params << Param(lgSmartTvThingUuidParamTypeId, upnpDeviceDescriptor.uuid());
                params << Param(lgSmartTvThingModelParamTypeId, upnpDeviceDescriptor.modelName());
                params << Param(lgSmartTvThingHostAddressParamTypeId, upnpDeviceDescriptor.hostAddress().toString());
                params << Param(lgSmartTvThingPortParamTypeId, upnpDeviceDescriptor.port());
                params << Param(lgSmartTvThingEventHandlerPortParamTypeId, TvEventHandler::getFreePort());
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
        qCDebug(dcLgSmartTv()) << "Start discovering WebOs smart TVs...";
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
    if (info->thingClassId() == lgSmartTvThingClassId) {
        QHostAddress host = QHostAddress(info->params().paramValue(lgSmartTvThingHostAddressParamTypeId).toString());
        quint16 port = info->params().paramValue(lgSmartTvThingPortParamTypeId).toUInt();
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
}

void IntegrationPluginLgSmartTv::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)

    if (info->thingClassId() == lgSmartTvThingClassId) {
        QHostAddress host = QHostAddress(info->params().paramValue(lgSmartTvThingHostAddressParamTypeId).toString());
        quint16 port = info->params().paramValue(lgSmartTvThingPortParamTypeId).toUInt();
        quint16 eventHandlerPort = info->params().paramValue(lgSmartTvThingEventHandlerPortParamTypeId).toUInt();
        QPair<QNetworkRequest, QByteArray> request = TvDevice::createPairingRequest(host, port, eventHandlerPort, secret);

        QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [=](){
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if(status != 200) {
                qCWarning(dcLgSmartTv) << "Pair TV request error:" << status << reply->errorString();
                return info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Error pairing TV. Please try again."));
            }

            // End pairing before calling setupDevice, which will always try to pair
            QPair<QNetworkRequest, QByteArray> request = TvDevice::createEndPairingRequest(host, port, eventHandlerPort);
            QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [this, info, reply, secret](){
                int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                if(status != 200) {
                    qCWarning(dcLgSmartTv) << "End pairing TV request error:" << status << reply->errorString();
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
}

void IntegrationPluginLgSmartTv::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == lgSmartTvThingClassId) {
        qCDebug(dcLgSmartTv()) << "Setup LG smart TV" << thing->name() << thing->params();
        QHostAddress address = QHostAddress(thing->paramValue(lgSmartTvThingHostAddressParamTypeId).toString());
        quint16 port = thing->paramValue(lgSmartTvThingPortParamTypeId).toUInt();
        quint16 eventHandlerPort = thing->paramValue(lgSmartTvThingEventHandlerPortParamTypeId).toUInt();
        TvDevice *tvDevice = new TvDevice(address, port, eventHandlerPort, this);
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
    } else if (thing->thingClassId() == webosTvThingClassId) {
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
        if (action.actionTypeId() == lgSmartTvIncreaseVolumeActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::VolUp);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvDecreaseVolumeActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::VolDown);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvMuteActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Mute);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvCommandChannelUpActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::ChannelUp);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvCommandChannelDownActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::ChannelDown);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvCommandPowerOffActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Power);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvNavigateActionTypeId) {
            TvDevice::RemoteKey key;
            QString navigateTo = action.paramValue(lgSmartTvNavigateActionToParamTypeId).toString();
            if (navigateTo == "up") {
                key = TvDevice::Up;
            } else if (navigateTo == "down") {
                key = TvDevice::Down;
            } else if (navigateTo == "left") {
                key = TvDevice::Left;
            } else if (navigateTo == "right") {
                key = TvDevice::Right;
            } else if (navigateTo == "enter") {
                key = TvDevice::Ok;
            } else if (navigateTo == "back") {
                key = TvDevice::Back;
            } else if (navigateTo == "menu") {
                key = TvDevice::Menu;
            } else if (navigateTo == "info") {
                key = TvDevice::Info;
            } else if (navigateTo == "home") {
                key = TvDevice::Home;
            }
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(key);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvPlayActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Play);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvPauseActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Pause);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvStopActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Stop);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvSkipNextActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::SkipForward);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvSkipBackActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::SkipBackward);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvFastForwardActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::FastForward);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvFastRewindActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Rewind);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvCommandInputSourceActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::ExternalInput);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvCommandExitActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::Exit);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvCommandMyAppsActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = tvDevice->createPressButtonRequest(TvDevice::MyApps);
            reply = hardwareManager()->networkManager()->post(request.first, request.second);
        } else if (action.actionTypeId() == lgSmartTvCommandProgramListActionTypeId) {
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

void IntegrationPluginLgSmartTv::browseThing(BrowseResult *result)
{
    if (result->thing()->thingClassId() == lgSmartTvThingClassId) {
        TvDevice *tv = m_tvList.key(result->thing());
        if (!tv) {
            result->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        tv->browse(result);
    }
}

void IntegrationPluginLgSmartTv::browserItem(BrowserItemResult *result)
{
    if (result->thing()->thingClassId() == lgSmartTvThingClassId) {
        TvDevice *tv = m_tvList.key(result->thing());
        if (!tv) {
            result->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        tv->browserItem(result);
    }
}


void IntegrationPluginLgSmartTv::pairTvDevice(Thing *thing)
{
    if (thing->thingClassId() == lgSmartTvThingClassId) {
        qCDebug(dcLgSmartTv()) << "Send pair request TV" << thing->name();
        QHostAddress host = QHostAddress(thing->paramValue(lgSmartTvThingHostAddressParamTypeId).toString());
        quint16 port = thing->paramValue(lgSmartTvThingPortParamTypeId).toUInt();
        quint16 eventHandlerPort = thing->paramValue(lgSmartTvThingEventHandlerPortParamTypeId).toUInt();

        pluginStorage()->beginGroup(thing->id().toString());
        QString key = pluginStorage()->value("key").toString();
        pluginStorage()->endGroup();

        QPair<QNetworkRequest, QByteArray> request = TvDevice::createPairingRequest(host, port, eventHandlerPort, key);
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
                loadAppList(thing);
                refreshTv(thing);
            }
        });
    }
}

void IntegrationPluginLgSmartTv::unpairTvDevice(Thing *thing)
{
    if (thing->thingClassId() == lgSmartTvThingClassId) {
        QHostAddress host = QHostAddress(thing->paramValue(lgSmartTvThingHostAddressParamTypeId).toString());
        quint16 port = thing->paramValue(lgSmartTvThingPortParamTypeId).toUInt();
        quint16 eventHandlerPort = thing->paramValue(lgSmartTvThingEventHandlerPortParamTypeId).toUInt();
        QPair<QNetworkRequest, QByteArray> request = TvDevice::createEndPairingRequest(host, port, eventHandlerPort);

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
}

void IntegrationPluginLgSmartTv::refreshTv(Thing *thing)
{
    TvDevice *tv = m_tvList.key(thing);
    // check volume information
    QNetworkReply *reply = hardwareManager()->networkManager()->get(tv->createVolumeInformationRequest());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [=](){
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
            if (status == 401) {
                pairTvDevice(thing);
            } else {
                tv->setReachable(false);
                qCWarning(dcLgSmartTv) << "Refresh tv information request error:" << status << reply->errorString();
            }
            return;
        }

        if (!tv->reachable()) {
            loadAppList(thing);
        }
        tv->setReachable(true);
        tv->onVolumeInformationUpdate(reply->readAll());

        //        QByteArray data = reply->readAll();
        //        qCDebug(dcLgSmartTv()) << "Refresh info received:" << data << qUtf8Printable(TvDevice::printXmlData(data));


        //        QNetworkReply *reply = hardwareManager()->networkManager()->get(tv->createAppListRequest());
        //        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        //        connect(reply, &QNetworkReply::finished, this, [reply, tv](){
        //            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        //            if (status != 200) {
        //                tv->setReachable(false);
        //                qCWarning(dcLgSmartTv) << "Refresh tv information request error:" << status << reply->errorString();
        //                return;
        //            }

        //            tv->setReachable(true);
        //            QByteArray data = reply->readAll();
        //            qCDebug(dcLgSmartTv()) << "Refresh info received:" << data << qUtf8Printable(TvDevice::printXmlData(data));


        //        tv->setReachable(true);
        //        tv->onVolumeInformationUpdate(reply->readAll());

        //        // Check channel information
        //        QNetworkReply *channelReply = hardwareManager()->networkManager()->get(tv->createChannelInformationRequest());
        //        connect(channelReply, &QNetworkReply::finished, this, &IntegrationPluginLgSmartTv::onNetworkManagerReplyFinished);
        //        m_channelInfoRequests.insert(channelReply, thing);
        //        });
    });

}

void IntegrationPluginLgSmartTv::loadAppList(Thing *thing)
{
    TvDevice *tv = m_tvList.key(thing);
    tv->clearAppList();

    // Note:: loading all apps (type 1) does not work, so load type 2 (premium) and typ 3 (myApps) sequentially
    QNetworkReply *reply = hardwareManager()->networkManager()->get(tv->createAppListCountRequest(2));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [=](){
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
            tv->setReachable(false);
            qCWarning(dcLgSmartTv) << "Get app count request error:" << status << reply->errorString();
            return;
        }

        tv->setReachable(true);

        QByteArray data = reply->readAll();
        int appCount = 0;
        QXmlStreamReader xml(data);
        while (!xml.atEnd() && !xml.hasError()) {
            xml.readNext();
            if (xml.name() == "number") {
                appCount = QVariant(xml.readElementText()).toInt();
            }
        }

        QNetworkReply *reply = hardwareManager()->networkManager()->get(tv->createAppListRequest(2, appCount));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, this, [=](){
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status != 200) {
                tv->setReachable(false);
                qCWarning(dcLgSmartTv) << "Get app list request error:" << status << reply->errorString();
                return;
            }

            QByteArray data = reply->readAll();

            // Parse app list
            QVariantList appList;
            QXmlStreamReader xml(qUtf8Printable(QString::fromLatin1(data)));
            while (xml.readNextStartElement() && !xml.atEnd()) {
                if (xml.name() == "dataList") {
                    while (xml.readNextStartElement()) {
                        QVariantMap appMap;
                        if (xml.name() == "data") {
                            qCDebug(dcLgSmartTv()) << "--------------------";
                            while (xml.readNextStartElement()) {
                                qCDebug(dcLgSmartTv()) << "-->" << xml.name() << xml.readElementText();
                                if (xml.name() == "name") {
                                    appMap.insert("name", xml.readElementText());
                                } else if (xml.name() == "icon") {
                                    appMap.insert("icon", xml.readElementText());
                                } if (xml.name() == "cpid") {
                                    appMap.insert("cpid", xml.readElementText());
                                }
                            }
                        }
                        appList.append(appMap);
                    }
                }
            }

            qCDebug(dcLgSmartTv()) << "Adding app list" << appList;
            tv->addAppList(appList);

            QNetworkReply *reply = hardwareManager()->networkManager()->get(tv->createAppListCountRequest(3));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, this, [=](){
                int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                if (status != 200) {
                    tv->setReachable(false);
                    qCWarning(dcLgSmartTv) << "Get app count request error:" << status << reply->errorString();
                    return;
                }

                QByteArray data = reply->readAll();

                int appCount = 0;
                QXmlStreamReader xml(data);
                while (!xml.atEnd() && !xml.hasError()) {
                    xml.readNext();
                    if (xml.name() == "number") {
                        appCount = QVariant(xml.readElementText()).toInt();
                    }
                }

                QNetworkReply *reply = hardwareManager()->networkManager()->get(tv->createAppListRequest(3, appCount));
                connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                connect(reply, &QNetworkReply::finished, this, [reply, tv](){
                    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                    if (status != 200) {
                        tv->setReachable(false);
                        qCWarning(dcLgSmartTv) << "Refresh tv information request error:" << status << reply->errorString();
                        return;
                    }

                    QByteArray data = reply->readAll();

                    // Parse app list
                    QVariantList appList;
                    QXmlStreamReader xml(QString::fromLatin1(data));
                    while (xml.readNextStartElement() && !xml.atEnd()) {
                        if (xml.name() == "dataList") {
                            while (xml.readNextStartElement()) {
                                if (xml.name() == "data") {
                                    QVariantMap appMap;
                                    while (xml.readNextStartElement()) {
                                        if (xml.name() == "name") {
                                            appMap.insert("name", xml.readElementText());
                                        } else if (xml.name() == "icon") {
                                            appMap.insert("icon", xml.readElementText());
                                        } if (xml.name() == "auid") {
                                            appMap.insert("auid", xml.readElementText());
                                        }
                                    }
                                    qCDebug(dcLgSmartTv()) << appMap;
                                    appList.append(appMap);
                                }
                            }
                        }
                    }

                    qCDebug(dcLgSmartTv()) << "Adding app list" << appList;
                    tv->addAppList(appList);
                });
            });
        });
    });
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
        if (status != 200) {
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
        if (status != 200) {
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
    thing->setStateValue(lgSmartTvVolumeLevelStateTypeId, tvDevice->volumeLevel());
    thing->setStateValue(lgSmartTvMuteStateTypeId, tvDevice->mute());
    thing->setStateValue(lgSmartTvTvChannelTypeStateTypeId, tvDevice->channelType());
    thing->setStateValue(lgSmartTvTvChannelNameStateTypeId, tvDevice->channelName());
    thing->setStateValue(lgSmartTvTvChannelNumberStateTypeId, tvDevice->channelNumber());
    thing->setStateValue(lgSmartTvTvProgramNameStateTypeId, tvDevice->programName());
    thing->setStateValue(lgSmartTvTvInputSourceIndexStateTypeId, tvDevice->inputSourceIndex());
    thing->setStateValue(lgSmartTvTvInputSourceLabelNameStateTypeId, tvDevice->inputSourceLabelName());
}
