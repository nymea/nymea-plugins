/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Michael Zanetti <michael.zanetti@nymea.io>          *
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

#include "devicepluginunifi.h"
#include "plugininfo.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>

#include <hardwaremanager.h>
#include <network/networkaccessmanager.h>
#include <plugintimer.h>

DevicePluginUnifi::DevicePluginUnifi(QObject *parent) : DevicePlugin(parent)
{

}

DevicePluginUnifi::~DevicePluginUnifi()
{

}

void DevicePluginUnifi::init()
{
}

void DevicePluginUnifi::discoverDevices(DeviceDiscoveryInfo *info)
{
    Q_ASSERT_X(info->deviceClassId() == clientDeviceClassId, "discoverDevices", "Invalid device class in discovery");

    Devices controllers = myDevices().filterByDeviceClassId(controllerDeviceClassId);
    if (controllers.isEmpty()) {
        info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Please configure a UniFi controller first."));
        return;
    }

    connect(info, &DeviceDiscoveryInfo::aborted, this, [this, info](){
        m_pendingDiscoveries.remove(info);
    });

    foreach (Device *controller, controllers) {
        m_pendingDiscoveries[info].append(controller);
        QNetworkRequest request = createRequest(controller, "/api/self/sites");
        QNetworkReply *sitesReply = hardwareManager()->networkManager()->get(request);
        connect(sitesReply, &QNetworkReply::finished, sitesReply, &QNetworkReply::deleteLater);
        connect(sitesReply, &QNetworkReply::finished, info, [this, info, sitesReply, controller](){
            if (sitesReply->error() != QNetworkReply::NoError) {
                qCWarning(dcUnifi()) << "Error fetching sites";
                m_pendingDiscoveries[info].removeAll(controller);
                if (m_pendingDiscoveries[info].isEmpty()) {
                    info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Fetching sites from controller failed."));
                }
                return;
            }
            QByteArray data = sitesReply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcUnifi()) << "Error parsing data" << data;
                m_pendingDiscoveries[info].removeAll(controller);
                if (m_pendingDiscoveries[info].isEmpty()) {
                    info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error communicating with the controller."));
                }
                return;
            }

            if (jsonDoc.toVariant().toMap().value("meta").toMap().value("rc").toString() != "ok") {
                qCWarning(dcUnifi()) << "Controller did not responde with OK" << qUtf8Printable(jsonDoc.toJson());
                m_pendingDiscoveries[info].removeAll(controller);
                if (m_pendingDiscoveries[info].isEmpty()) {
                    info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Fetching sites from controller failed."));
                }
                return;
            }

            qCDebug(dcUnifi()) << "**** sites reply finished" << data;
            foreach (const QVariant &siteVariant, jsonDoc.toVariant().toMap().value("data").toList()) {
                qCDebug(dcUnifi()) << "Have site:" << siteVariant.toMap().value("_id").toString() << siteVariant.toMap().value("name").toString() << siteVariant.toMap().value("desc").toString();

                QString site = siteVariant.toMap().value("_id").toString();
                QString siteName = siteVariant.toMap().value("name").toString();
                QString siteDescription = siteVariant.toMap().value("desc").toString();
                QNetworkRequest request = createRequest(controller, QString("/api/s/%1/stat/sta/").arg(siteName));

                qCDebug(dcUnifi()) << "Fetching clients for site" << site << siteName << request.url();

                m_pendingSiteDiscoveries[controller].append(siteName);

                QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
                connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                connect(reply, &QNetworkReply::finished, info, [this, info, reply, controller, siteName](){
                    m_pendingSiteDiscoveries[controller].removeAll(siteName);
                    if (m_pendingSiteDiscoveries[controller].isEmpty()) {
                        m_pendingDiscoveries[info].removeAll(controller);
                    }

                    bool hasError = false;

                    if (reply->error() != QNetworkReply::NoError) {
                        qCWarning(dcUnifi()) << "Error fetching clients from site" << reply->error() << reply->errorString();
                        hasError = true;
                    } else {
                        QByteArray data = reply->readAll();
                        QJsonParseError error;
                        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                        if (error.error != QJsonParseError::NoError) {
                            qCWarning(dcUnifi()) << "Error parsing json for clients reply:" << error.errorString() << data;
                            hasError = true;
                        } else {
                            QVariantMap response = jsonDoc.toVariant().toMap();
                            if (response.value("meta").toMap().value("rc").toString() != "ok") {
                                qCWarning(dcUnifi()) << "Error response from controller:" << qUtf8Printable(jsonDoc.toJson());
                                hasError = true;
                            } else {
                                QVariantList clients = response.value("data").toList();
                                foreach (const QVariant &clientVariant, clients) {
//                                    qCDebug(dcUnifi()) << "client:" << qUtf8Printable(QJsonDocument::fromVariant(clientVariant).toJson());

                                    QString name = clientVariant.toMap().value("name").toString();
                                    if (name.isEmpty()) {
                                        name = clientVariant.toMap().value("hostname").toString();
                                    }
                                    if (name.isEmpty()) {
                                        name = clientVariant.toMap().value("oui").toString();
                                    }
                                    DeviceDescriptor d(clientDeviceClassId, name, clientVariant.toMap().value("mac").toString());
                                    ParamList params;
                                    params << Param(clientDeviceMacParamTypeId, clientVariant.toMap().value("mac").toString());
                                    params << Param(clientDeviceSiteParamTypeId, siteName);
                                    d.setParams(params);

                                    Device *existingDevice = myDevices().findByParams(params);
                                    if (existingDevice) {
                                        d.setDeviceId(existingDevice->id());
                                    }

                                    d.setParentDeviceId(controller->id());

                                    info->addDeviceDescriptor(d);
                                }
                            }
                        }
                    }


                    if (m_pendingDiscoveries[info].isEmpty()) {
                        info->finish(hasError ? Device::DeviceErrorHardwareFailure : Device::DeviceErrorNoError);
                    }
                });
            }

        });
    }
}

void DevicePluginUnifi::startPairing(DevicePairingInfo *info)
{
    info->finish(Device::DeviceErrorNoError, QT_TR_NOOP("Please enter your login credentials for the UniFi controller."));
}

void DevicePluginUnifi::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret)
{
    QString host = info->params().paramValue(controllerDeviceIpAddressParamTypeId).toString();
    QNetworkRequest request = createRequest(host, "/api/login");
    QVariantMap login;
    login.insert("username", username);
    login.insert("password", secret);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(login).toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, info, reply, username, secret](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcUnifi()) << "Network request error:" << reply->error() << reply->errorString();
            info->finish(Device::DeviceErrorHardwareFailure);
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcUnifi()) << "Error parsing JSON response from controller:" << error.errorString() << data;
            info->finish(Device::DeviceErrorHardwareFailure);
            return;
        }

        pluginStorage()->beginGroup(info->deviceId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", secret);
        pluginStorage()->endGroup();
        info->finish(Device::DeviceErrorNoError);
    });
}

void DevicePluginUnifi::setupDevice(DeviceSetupInfo *info)
{
    if (info->device()->deviceClassId() == controllerDeviceClassId) {
        QNetworkRequest request = createRequest(info->device(), "/api/login");
        QVariantMap login;
        pluginStorage()->beginGroup(info->device()->id().toString());
        login.insert("username", pluginStorage()->value("username").toString());
        login.insert("password", pluginStorage()->value("password").toString());
        pluginStorage()->endGroup();
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(login).toJson());

        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcUnifi()) << "Network request error:" << reply->error() << reply->errorString();
                info->finish(Device::DeviceErrorHardwareFailure);
                return;
            }

            QByteArray data = reply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcUnifi()) << "Error parsing JSON response from controller:" << error.errorString() << data;
                info->finish(Device::DeviceErrorHardwareFailure);
                return;
            }

            info->device()->setStateValue(controllerConnectedStateTypeId, true);
            info->finish(Device::DeviceErrorNoError);

        });
    }

    if (info->device()->deviceClassId() == clientDeviceClassId) {
        info->finish(Device::DeviceErrorNoError);
    }
}

void DevicePluginUnifi::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == controllerDeviceClassId && !m_loginTimer) {
        // Let's refresh the login every minute
        m_loginTimer = hardwareManager()->pluginTimerManager()->registerTimer();
        connect(m_loginTimer, &PluginTimer::timeout, this, [this](){
            foreach (Device *controller, myDevices().filterByDeviceClassId(controllerDeviceClassId)) {
                QNetworkRequest request = createRequest(controller, "/api/login");
                QVariantMap login;
                pluginStorage()->beginGroup(controller->id().toString());
                login.insert("username", pluginStorage()->value("username"));
                login.insert("password", pluginStorage()->value("password"));
                pluginStorage()->endGroup();
                QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(login).toJson());
                connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            }
        });
    }

    if (device->deviceClassId() == clientDeviceClassId && !m_pollTimer) {
        m_pollTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
        connect(m_pollTimer, &PluginTimer::timeout, this, [this](){
            foreach (Device *client, myDevices().filterByDeviceClassId(clientDeviceClassId)) {
                Device *controller = myDevices().findById(client->parentId());
                QString mac = client->paramValue(clientDeviceMacParamTypeId).toString();
                QString site = client->paramValue(clientDeviceSiteParamTypeId).toString();
                QNetworkRequest request = createRequest(controller, QString("/api/s/%1/stat/sta/%2").arg(site).arg(mac));
                QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
                connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                connect(reply, &QNetworkReply::finished, client, [this, client, reply](){
                    if (reply->error() != QNetworkReply::NoError) {
                        // If the device is not present we'll get an InvalidOperationError, silence that as it's expected but print other failures
                        if (reply->error() != QNetworkReply::ProtocolInvalidOperationError) {
                            qCDebug(dcUnifi()) << "Error fetching device state from controller" << reply->error() << reply->errorString();
                        }
                        markOffline(client);
                        return;
                    }
                    QByteArray data = reply->readAll();
                    QJsonParseError error;
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                    if (error.error != QJsonParseError::NoError) {
                        qCWarning(dcUnifi()) << "Error parsing json from controller:" << error.error << error.errorString() << "\n" << data;
                        markOffline(client);
                        return;
                    }

//                    qCDebug(dcUnifi()) << "Client is present reply" << qUtf8Printable(jsonDoc.toJson());
                    QVariantList clientEntries = jsonDoc.toVariant().toMap().value("data").toList();
                    if (clientEntries.count() != 1) {
                        qCWarning(dcUnifi()) << "Client data not found in controller reply";
                        markOffline(client);
                        return;
                    }

                    QVariantMap clientData = clientEntries.first().toMap();

                    client->setStateValue(clientLastSeenTimeStateTypeId, clientData.value("last_seen").toInt());
                    client->setStateValue(clientIsPresentStateTypeId, true);
                });

            }
        });
    }
}

void DevicePluginUnifi::deviceRemoved(Device *device)
{
    Q_UNUSED(device)
    if (myDevices().filterByDeviceClassId(controllerDeviceClassId).isEmpty() && m_loginTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_loginTimer);
        m_loginTimer = nullptr;
    }
    if (myDevices().filterByDeviceClassId(clientDeviceClassId).isEmpty() && m_pollTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pollTimer);
        m_pollTimer = nullptr;
    }
}

QNetworkRequest DevicePluginUnifi::createRequest(const QString &address, const QString &path)
{
    QUrl url;
    url.setScheme("https");
    url.setHost(address);
    url.setPort(8443);
    url.setPath(path);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(config);
    return request;
}

QNetworkRequest DevicePluginUnifi::createRequest(Device *device, const QString &path)
{
    QString ipAddress = device->paramValue(controllerDeviceIpAddressParamTypeId).toString();
    return createRequest(ipAddress, path);
}

void DevicePluginUnifi::markOffline(Device *device)
{
    uint gracePeriod = device->setting(clientSettingsGracePeriodParamTypeId).toUInt();
    QDateTime lastSeenTime = QDateTime::fromMSecsSinceEpoch(device->stateValue(clientLastSeenTimeStateTypeId).toInt() * 1000);
    if (lastSeenTime.addSecs(gracePeriod * 60) < QDateTime::currentDateTime()) {
        device->setStateValue(clientIsPresentStateTypeId, false);
    }
}

