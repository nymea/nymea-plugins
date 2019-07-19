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

#include "devicepluginkodi.h"
#include "devices/device.h"
#include "plugininfo.h"
#include "network/upnp/upnpdiscovery.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "network/zeroconf/zeroconfserviceentry.h"
#include "network/networkaccessmanager.h"

#include <QNetworkRequest>
#include <QNetworkReply>

DevicePluginKodi::DevicePluginKodi()
{
//    Q_INIT_RESOURCE(images);
//    QFile file(":/images/nymea-logo.png");
//    if (!file.open(QIODevice::ReadOnly)) {
//        qCWarning(dcKodi) << "could not open" << file.fileName();
//        return;
//    }

//    QByteArray nymeaLogoByteArray = file.readAll();
//    if (nymeaLogoByteArray.isEmpty()) {
//        qCWarning(dcKodi) << "could not read" << file.fileName();
//        return;
//    }
    //    m_logo = nymeaLogoByteArray;
}

DevicePluginKodi::~DevicePluginKodi()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void DevicePluginKodi::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginKodi::onPluginTimer);
}

Device::DeviceSetupStatus DevicePluginKodi::setupDevice(Device *device)
{
    qCDebug(dcKodi) << "Setup Kodi device" << device->paramValue(kodiDeviceIpParamTypeId).toString();
    QString ipString = device->paramValue(kodiDeviceIpParamTypeId).toString();
    int port = device->paramValue(kodiDevicePortParamTypeId).toInt();
    int httpPort = device->paramValue(kodiDeviceHttpPortParamTypeId).toInt();
    Kodi *kodi= new Kodi(QHostAddress(ipString), port, httpPort, this);

    connect(kodi, &Kodi::connectionStatusChanged, this, &DevicePluginKodi::onConnectionChanged);
    connect(kodi, &Kodi::stateChanged, this, &DevicePluginKodi::onStateChanged);
    connect(kodi, &Kodi::actionExecuted, this, &DevicePluginKodi::onActionExecuted);
    connect(kodi, &Kodi::versionDataReceived, this, &DevicePluginKodi::versionDataReceived);
    connect(kodi, &Kodi::updateDataReceived, this, &DevicePluginKodi::onSetupFinished);
    connect(kodi, &Kodi::playbackStatusChanged, this, &DevicePluginKodi::onPlaybackStatusChanged);
    connect(kodi, &Kodi::browseResult, this, &DevicePluginKodi::browseRequestFinished);
    connect(kodi, &Kodi::browserItemResult, this, &DevicePluginKodi::browserItemRequestFinished);
    connect(kodi, &Kodi::browserItemActionExecuted, this, &DevicePluginKodi::onBrowserItemActionExecuted);

    connect(kodi, &Kodi::activePlayerChanged, device, [device](const QString &playerType){
        device->setStateValue(kodiPlayerTypeStateTypeId, playerType);
    });
    connect(kodi, &Kodi::mediaMetadataChanged, device, [this, device](const QString &title, const QString &artist, const QString &collection, const QString &artwork){
        device->setStateValue(kodiTitleStateTypeId, title);
        device->setStateValue(kodiArtistStateTypeId, artist);
        device->setStateValue(kodiCollectionStateTypeId, collection);


        QNetworkRequest request;
        QHostAddress hostAddr(device->paramValue(kodiDeviceIpParamTypeId).toString());
        QString addr;
        if (hostAddr.protocol() == QAbstractSocket::IPv4Protocol) {
            addr = hostAddr.toString();
        } else {
            addr = "[" + hostAddr.toString() + "]";
        }
        QString port = device->paramValue(kodiDeviceHttpPortParamTypeId).toString();

        request.setUrl(QUrl(QString("http://%1:%2/jsonrpc").arg(addr).arg(port)));
        qCDebug(dcKodi) << "Prepping file dl" << "http://" + addr + ":" + device->paramValue(kodiDevicePortParamTypeId).toString() + "/jsonrpc";
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QVariantMap map;
        map.insert("jsonrpc", "2.0");
        map.insert("method", "Files.PrepareDownload");
        map.insert("id", QString::number(123));
        QVariantMap params;
        params.insert("path", artwork);
        map.insert("params", params);
        QJsonDocument jsonDoc = QJsonDocument::fromVariant(map);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request, jsonDoc.toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, device, [device, reply, addr, port](){
            reply->deleteLater();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
            QString fileUrl = "http://" + addr + ":" + port + "/" + jsonDoc.toVariant().toMap().value("result").toMap().value("details").toMap().value("path").toString();
            qCDebug(dcKodi()) << "DL result:" << jsonDoc.toJson();
            qCDebug(dcKodi()) << "Resolved url:" << fileUrl;
            device->setStateValue(kodiArtworkStateTypeId, fileUrl);
        });

    });

    connect(kodi, &Kodi::shuffleChanged, device, [device](bool shuffle){
        device->setStateValue(kodiShuffleStateTypeId, shuffle);
    });
    connect(kodi, &Kodi::repeatChanged, device, [device](const QString &repeat){
        if (repeat == "one") {
            device->setStateValue(kodiRepeatStateTypeId, "One");
        } else if (repeat == "all") {
            device->setStateValue(kodiRepeatStateTypeId, "All");
        } else {
            device->setStateValue(kodiRepeatStateTypeId, "None");
        }
    });
    m_kodis.insert(kodi, device);
    m_asyncSetups.append(kodi);

    kodi->connectKodi();

    return Device::DeviceSetupStatusAsync;
}

void DevicePluginKodi::deviceRemoved(Device *device)
{
    Kodi *kodi = m_kodis.key(device);
    m_kodis.remove(kodi);
    qCDebug(dcKodi) << "Delete " << device->name();
    kodi->deleteLater();
}

Device::DeviceError DevicePluginKodi::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    Q_UNUSED(deviceClassId)

    ZeroConfServiceBrowser *serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_xbmc-jsonrpc._tcp");
    ZeroConfServiceBrowser *httpServiceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_http._tcp");
    QTimer::singleShot(5000, this, [this, serviceBrowser, httpServiceBrowser](){
        QHash<QString, DeviceDescriptor> descriptors;

        foreach (const ZeroConfServiceEntry avahiEntry, serviceBrowser->serviceEntries()) {

            QString uuid;
            foreach (const QString &txt, avahiEntry.txt()) {
                if (txt.startsWith("uuid")) {
                    uuid = txt.split("=").last();
                    break;
                }
            }

            if (descriptors.contains(uuid)) {
                // Might appear multiple times, IPv4 and IPv6
                continue;
            }

            qCDebug(dcKodi) << "Zeroconf entry:" << avahiEntry;
            DeviceDescriptor descriptor(kodiDeviceClassId, avahiEntry.name(), avahiEntry.hostName() + " (" + avahiEntry.hostAddress().toString() + ")");
            ParamList params;
            params << Param(kodiDeviceIpParamTypeId, avahiEntry.hostAddress().toString());
            params << Param(kodiDevicePortParamTypeId, avahiEntry.port());
            params << Param(kodiDeviceUuidParamTypeId, uuid);
            descriptor.setParams(params);

            Devices existing = myDevices().filterByParam(kodiDeviceUuidParamTypeId, uuid);
            if (existing.count() > 0) {
                descriptor.setDeviceId(existing.first()->id());
            }

            descriptors.insert(uuid, descriptor);
        }

        foreach (const ZeroConfServiceEntry avahiEntry, httpServiceBrowser->serviceEntries()) {
//            qCDebug(dcKodi) << "Zeroconf http entry:" << avahiEntry;
            QString uuid;
            foreach (const QString &txt, avahiEntry.txt()) {
                if (txt.startsWith("uuid")) {
                    uuid = txt.split("=").last();
                    break;
                }
            }
            if (!descriptors.contains(uuid)) {
                continue;
            }
            qCDebug(dcKodi()) << "Updating http parameter:" << avahiEntry.port();
            DeviceDescriptor descriptor = descriptors.value(uuid);
            ParamList params = descriptor.params();
            params << Param(kodiDeviceHttpPortParamTypeId, avahiEntry.port());
            descriptor.setParams(params);
            descriptors[uuid] = descriptor;
        }


        foreach (const DeviceDescriptor &d, descriptors.values()) {
            qCDebug(dcKodi()) << "Returning descritpor:" << d.params();
        }
        emit devicesDiscovered(kodiDeviceClassId, descriptors.values());
        serviceBrowser->deleteLater();
        httpServiceBrowser->deleteLater();
    });

    return Device::DeviceErrorAsync;
}

Device::DeviceError DevicePluginKodi::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == kodiDeviceClassId) {
        Kodi *kodi = m_kodis.key(device);

        // check connection state
        if (!kodi->connected()) {
            return Device::DeviceErrorHardwareNotAvailable;
        }

        int commandId = -1;
        if (action.actionTypeId() == kodiNotifyActionTypeId) {
            commandId = kodi->showNotification(
                        action.param(kodiNotifyActionTitleParamTypeId).value().toString(),
                        action.param(kodiNotifyActionBodyParamTypeId).value().toString(),
                        8000,
                        action.param(kodiNotifyActionTypeParamTypeId).value().toString());
        } else if (action.actionTypeId() == kodiVolumeActionTypeId) {
            commandId = kodi->setVolume(action.param(kodiVolumeActionVolumeParamTypeId).value().toInt());
        } else if (action.actionTypeId() == kodiMuteActionTypeId) {
            commandId = kodi->setMuted(action.param(kodiMuteActionMuteParamTypeId).value().toBool());
        } else if (action.actionTypeId() == kodiPressButtonActionTypeId) {
            commandId = kodi->pressButton(action.param(kodiPressButtonActionButtonParamTypeId).value().toString());
        } else if (action.actionTypeId() == kodiSystemActionTypeId) {
            commandId = kodi->systemCommand(action.param(kodiSystemActionSystemCommandParamTypeId).value().toString());
        } else if(action.actionTypeId() == kodiSkipBackActionTypeId) {
            commandId = kodi->pressButton("skipprevious");
        } else if(action.actionTypeId() == kodiFastRewindActionTypeId) {
            commandId = kodi->pressButton("rewind");
        } else if(action.actionTypeId() == kodiStopActionTypeId) {
            commandId = kodi->pressButton("stop");
        } else if(action.actionTypeId() == kodiPlayActionTypeId) {
            commandId = kodi->pressButton("play");
        } else if(action.actionTypeId() == kodiPauseActionTypeId) {
            commandId = kodi->pressButton("pause");
        } else if(action.actionTypeId() == kodiFastForwardActionTypeId) {
            commandId = kodi->pressButton("fastforward");
        } else if(action.actionTypeId() == kodiSkipNextActionTypeId) {
            commandId = kodi->pressButton("skipnext");
        } else if (action.actionTypeId() == kodiShuffleActionTypeId) {
            commandId = kodi->setShuffle(action.param(kodiShuffleActionShuffleParamTypeId).value().toBool());
        } else if (action.actionTypeId() == kodiRepeatActionTypeId) {
            QString repeat = action.param(kodiRepeatActionRepeatParamTypeId).value().toString();
            if (repeat == "One") {
                commandId = kodi->setRepeat("one");
            } else if (repeat == "All") {
                commandId = kodi->setRepeat("all");
            } else {
                commandId = kodi->setRepeat("off");
            }
        } else {
            qWarning(dcKodi()) << "Unhandled action type" << action.actionTypeId();
            return Device::DeviceErrorActionTypeNotFound;
        }
        m_pendingActions.insert(commandId, action.id());
        return Device::DeviceErrorAsync;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}

Device::BrowseResult DevicePluginKodi::browseDevice(Device *device, Device::BrowseResult result, const QString &itemId, const QLocale &locale)
{
    Q_UNUSED(locale)

    Kodi *kodi = m_kodis.key(device);
    if (!kodi) {
        result.status = Device::DeviceErrorHardwareNotAvailable;
        return result;
    }

    return kodi->browse(itemId, result);
}

Device::BrowserItemResult DevicePluginKodi::browserItem(Device *device, Device::BrowserItemResult result, const QString &itemId, const QLocale &locale)
{
    Q_UNUSED(locale)

    Kodi *kodi = m_kodis.key(device);
    if (!kodi) {
        result.status = Device::DeviceErrorHardwareNotAvailable;
        return result;
    }

    return kodi->browserItem(itemId, result);

}

Device::DeviceError DevicePluginKodi::executeBrowserItem(Device *device, const BrowserAction &browserAction)
{
    Kodi *kodi = m_kodis.key(device);
    if (!kodi) {
        return Device::DeviceErrorHardwareNotAvailable;
    }

    return kodi->launchBrowserItem(browserAction.itemId());
}

Device::DeviceError DevicePluginKodi::executeBrowserItemAction(Device *device, const BrowserItemAction &browserItemAction)
{
    Kodi *kodi = m_kodis.key(device);
    if (!kodi) {
        return Device::DeviceErrorHardwareNotAvailable;
    }

    int id = kodi->executeBrowserItemAction(browserItemAction.itemId(), browserItemAction.actionTypeId());
    if (id == -1) {
        return Device::DeviceErrorHardwareFailure;
    }
    m_pendingBrowserItemActions.insert(id, browserItemAction.id());
    return Device::DeviceErrorAsync;
}

void DevicePluginKodi::onPluginTimer()
{
    foreach (Kodi *kodi, m_kodis.keys()) {
        if (!kodi->connected()) {
            kodi->connectKodi();
            continue;
        }
    }
}

void DevicePluginKodi::onConnectionChanged()
{
    Kodi *kodi = static_cast<Kodi *>(sender());
    Device *device = m_kodis.value(kodi);

    if (kodi->connected()) {
        // if this is the first setup, check version
        if (m_asyncSetups.contains(kodi)) {
            m_asyncSetups.removeAll(kodi);
            kodi->checkVersion();
        }
    }

    device->setStateValue(kodiConnectedStateTypeId, kodi->connected());
}

void DevicePluginKodi::onStateChanged()
{
    Kodi *kodi = static_cast<Kodi *>(sender());
    Device *device = m_kodis.value(kodi);

    // set device state values
    device->setStateValue(kodiVolumeStateTypeId, kodi->volume());
    device->setStateValue(kodiMuteStateTypeId, kodi->muted());
}

void DevicePluginKodi::onActionExecuted(int actionId, bool success)
{
    if (!m_pendingActions.contains(actionId)) {
        return;
    }
    emit actionExecutionFinished(m_pendingActions.take(actionId), success ? Device::DeviceErrorNoError : Device::DeviceErrorInvalidParameter);
}

void DevicePluginKodi::onBrowserItemActionExecuted(int actionId, bool success)
{
    if (!m_pendingBrowserItemActions.contains(actionId)) {
        return;
    }
    emit browserItemActionExecutionFinished(m_pendingBrowserItemActions.take(actionId), success ? Device::DeviceErrorNoError : Device::DeviceErrorHardwareFailure);
}

void DevicePluginKodi::versionDataReceived(const QVariantMap &data)
{
    Kodi *kodi = static_cast<Kodi *>(sender());
    Device *device = m_kodis.value(kodi);

    QVariantMap version = data.value("version").toMap();
    QString apiVersion = QString("%1.%2.%3").arg(version.value("major").toString()).arg(version.value("minor").toString()).arg(version.value("patch").toString());
    qCDebug(dcKodi) << "API Version:" << apiVersion;

    if (version.value("major").toInt() < 6) {
        qCWarning(dcKodi) << "incompatible api version:" << apiVersion;
        emit deviceSetupFinished(device, Device::DeviceSetupStatusFailure);
        return;
    }
    kodi->update();
}

void DevicePluginKodi::onSetupFinished(const QVariantMap &data)
{
    Kodi *kodi = static_cast<Kodi *>(sender());
    Device *device = m_kodis.value(kodi);

    QVariantMap version = data.value("version").toMap();
    QString kodiVersion = QString("%1.%2 (%3)").arg(version.value("major").toString()).arg(version.value("minor").toString()).arg(version.value("tag").toString());
    qCDebug(dcKodi) << "Version:" << kodiVersion;

    emit deviceSetupFinished(device, Device::DeviceSetupStatusSuccess);

    kodi->showNotification("nymea", tr("Connected"), 2000, "info");
}

void DevicePluginKodi::onPlaybackStatusChanged(const QString &playbackStatus)
{
    Kodi *kodi = static_cast<Kodi *>(sender());
    Device *device = m_kodis.value(kodi);
    device->setStateValue(kodiPlaybackStatusStateTypeId, playbackStatus);
    // legacy events
    if (playbackStatus == "Playing") {
        emit emitEvent(Event(kodiOnPlayerPlayEventTypeId, device->id()));
    } else if (playbackStatus == "Paused") {
        emit emitEvent(Event(kodiOnPlayerPauseEventTypeId, device->id()));
    } else {
        emit emitEvent(Event(kodiOnPlayerStopEventTypeId, device->id()));
    }
}

