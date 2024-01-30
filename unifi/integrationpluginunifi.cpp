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

#include "integrationpluginunifi.h"
#include "plugininfo.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QNetworkCookieJar>

#include <hardwaremanager.h>
#include <network/networkaccessmanager.h>
#include <plugintimer.h>

IntegrationPluginUnifi::IntegrationPluginUnifi(QObject *parent) : IntegrationPlugin(parent)
{

}

IntegrationPluginUnifi::~IntegrationPluginUnifi()
{

}

void IntegrationPluginUnifi::init()
{
}

void IntegrationPluginUnifi::discoverThings(ThingDiscoveryInfo *info)
{
    Q_ASSERT_X(info->thingClassId() == clientThingClassId, "discoverDevices", "Invalid thing class in discovery");

    Things controllers = myThings().filterByThingClassId(controllerThingClassId);
    if (controllers.isEmpty()) {
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Please configure a UniFi controller first."));
        return;
    }

    connect(info, &ThingDiscoveryInfo::aborted, this, [this, info](){
        m_pendingDiscoveries.remove(info);
    });

    foreach (Thing *controller, controllers) {
        m_pendingDiscoveries[info].append(controller);
        QNetworkRequest request = createRequest(controller, "/api/self/sites");
        QNetworkReply *sitesReply = hardwareManager()->networkManager()->get(request);
        connect(sitesReply, &QNetworkReply::finished, sitesReply, &QNetworkReply::deleteLater);
        connect(sitesReply, &QNetworkReply::finished, info, [this, info, sitesReply, controller](){
            if (sitesReply->error() != QNetworkReply::NoError) {
                qCWarning(dcUnifi()) << "Error fetching sites";
                m_pendingDiscoveries[info].removeAll(controller);
                if (m_pendingDiscoveries[info].isEmpty()) {
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Fetching sites from controller failed."));
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
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error communicating with the controller."));
                }
                return;
            }

            if (jsonDoc.toVariant().toMap().value("meta").toMap().value("rc").toString() != "ok") {
                qCWarning(dcUnifi()) << "Controller did not responde with OK" << qUtf8Printable(jsonDoc.toJson());
                m_pendingDiscoveries[info].removeAll(controller);
                if (m_pendingDiscoveries[info].isEmpty()) {
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Fetching sites from controller failed."));
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
                                    ThingDescriptor d(clientThingClassId, name, clientVariant.toMap().value("mac").toString());
                                    ParamList params;
                                    params << Param(clientThingMacParamTypeId, clientVariant.toMap().value("mac").toString());
                                    params << Param(clientThingSiteParamTypeId, siteName);
                                    d.setParams(params);

                                    Thing *existingThing = myThings().findByParams(params);
                                    if (existingThing) {
                                        d.setThingId(existingThing->id());
                                    }

                                    d.setParentId(controller->id());

                                    info->addThingDescriptor(d);
                                }
                            }
                        }
                    }


                    if (m_pendingDiscoveries[info].isEmpty()) {
                        info->finish(hasError ? Thing::ThingErrorHardwareFailure : Thing::ThingErrorNoError);
                    }
                });
            }

        });
    }
}

void IntegrationPluginUnifi::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter your login credentials for the UniFi controller."));
}

void IntegrationPluginUnifi::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    QString host = info->params().paramValue(controllerThingIpAddressParamTypeId).toString();
    uint port = info->params().paramValue(controllerThingPortParamTypeId).toUInt();
    QString path;
    if (info->params().paramValue(controllerThingModeParamTypeId).toString() == "UniFi OS") {
        path = "/api/auth/login";
    } else {
        path = "/api/login";
    }
    QNetworkRequest request = createRequest(host, port, path);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QVariantMap login;
    login.insert("username", username);
    login.insert("password", secret);
    qCDebug(dcUnifi()) << "ConfirmPairing: Sending request:" << request.url().toString() << QJsonDocument::fromVariant(login).toJson();
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(login).toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, info, reply, username, secret](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcUnifi()) << "ConfirmPairing: Network request error:" << reply->error() << reply->errorString();
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcUnifi()) << "ConfirmPairing: Error parsing JSON response from controller:" << error.errorString() << data;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        qCDebug(dcUnifi()) << "ConfirmPairing succeeded";
        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", secret);
        pluginStorage()->endGroup();
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginUnifi::setupThing(ThingSetupInfo *info)
{
    if (info->thing()->thingClassId() == controllerThingClassId) {

        // If the user just completed the pairing process we already have a valid cookie in the networkAccessManager.
        if (info->isInitialSetup()) {
            info->finish(Thing::ThingErrorNoError);
            info->thing()->setStateValue(controllerConnectedStateTypeId, true);
            return;
        }

        // After a nymea startup, we'll have to login to obtain a cookie.
        QString host = info->thing()->params().paramValue(controllerThingIpAddressParamTypeId).toString();
        uint port = info->thing()->params().paramValue(controllerThingPortParamTypeId).toUInt();
        QString path;
        if (info->thing()->paramValue(controllerThingModeParamTypeId).toString() == "UniFi OS") {
            path = "/api/auth/login";
        } else {
            path = "/api/login";
        }
        QNetworkRequest request = createRequest(host, port, path);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QVariantMap login;
        pluginStorage()->beginGroup(info->thing()->id().toString());
        login.insert("username", pluginStorage()->value("username").toString());
        login.insert("password", pluginStorage()->value("password").toString());
        pluginStorage()->endGroup();
        qCDebug(dcUnifi()) << "SetupThing: Sending request:" << request.url().toString() << QJsonDocument::fromVariant(login).toJson();
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(login).toJson());

        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcUnifi()) << "SetupThing: Network request error:" << reply->error() << reply->errorString();
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            QByteArray data = reply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcUnifi()) << "SetupThing: Error parsing JSON response from controller:" << error.errorString() << data;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            qCDebug(dcUnifi()) << "SetupThing succeded";

            info->thing()->setStateValue(controllerConnectedStateTypeId, true);
            info->finish(Thing::ThingErrorNoError);

        });
    }

    if (info->thing()->thingClassId() == clientThingClassId) {
        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginUnifi::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == controllerThingClassId && !m_loginTimer) {
        // Let's refresh the login every minute
        m_loginTimer = hardwareManager()->pluginTimerManager()->registerTimer();
        connect(m_loginTimer, &PluginTimer::timeout, this, [this](){
            foreach (Thing *controller, myThings().filterByThingClassId(controllerThingClassId)) {
                QString host = controller->paramValue(controllerThingIpAddressParamTypeId).toString();
                uint port = controller->paramValue(controllerThingPortParamTypeId).toUInt();
                QString path;
                if (controller->paramValue(controllerThingModeParamTypeId).toString() == "UniFi OS") {
                    path = "/api/auth/login";
                } else {
                    path = "/api/login";
                }
                QNetworkRequest request = createRequest(host, port, path);
                request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                QVariantMap login;
                pluginStorage()->beginGroup(controller->id().toString());
                login.insert("username", pluginStorage()->value("username"));
                login.insert("password", pluginStorage()->value("password"));
                pluginStorage()->endGroup();
                qCDebug(dcUnifi()) << "Cookie KeepAlive: Sending request:" << request.url().toString();
                QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(login).toJson());
                connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            }
        });
    }

    if (thing->thingClassId() == clientThingClassId && !m_pollTimer) {
        m_pollTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
        connect(m_pollTimer, &PluginTimer::timeout, this, [this](){
            foreach (Thing *client, myThings().filterByThingClassId(clientThingClassId)) {
                Thing *controller = myThings().findById(client->parentId());
                QString mac = client->paramValue(clientThingMacParamTypeId).toString();
                QString site = client->paramValue(clientThingSiteParamTypeId).toString();
                QNetworkRequest request = createRequest(controller, QString("/api/s/%1/stat/sta/%2").arg(site).arg(mac));
                QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
                connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                connect(reply, &QNetworkReply::finished, client, [this, client, reply](){
                    if (reply->error() != QNetworkReply::NoError) {
                        // If the thing is not present we'll get an InvalidOperationError, silence that as it's expected but print other failures
                        if (reply->error() != QNetworkReply::ProtocolInvalidOperationError) {
                            qCDebug(dcUnifi()) << "Error fetching thing state from controller" << reply->error() << reply->errorString();
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

void IntegrationPluginUnifi::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)
    if (myThings().filterByThingClassId(controllerThingClassId).isEmpty() && m_loginTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_loginTimer);
        m_loginTimer = nullptr;
    }
    if (myThings().filterByThingClassId(clientThingClassId).isEmpty() && m_pollTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pollTimer);
        m_pollTimer = nullptr;
    }
}

QNetworkRequest IntegrationPluginUnifi::createRequest(const QString &address, uint port, const QString &path, const QString &prefix)
{
    QUrl url;
    url.setScheme("https");
    url.setHost(address);
    url.setPort(port);
    url.setPath(prefix + path);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(config);
    return request;
}

QNetworkRequest IntegrationPluginUnifi::createRequest(Thing *thing, const QString &path)
{
    QString ipAddress = thing->paramValue(controllerThingIpAddressParamTypeId).toString();
    uint port = thing->paramValue(controllerThingPortParamTypeId).toUInt();
    bool prefix = thing->paramValue(controllerThingModeParamTypeId).toString() == "UniFi OS";
    return createRequest(ipAddress, port, path, prefix ? "/proxy/network" : "");
}

void IntegrationPluginUnifi::markOffline(Thing *thing)
{
    uint gracePeriod = thing->setting(clientSettingsGracePeriodParamTypeId).toUInt();
    QDateTime lastSeenTime = QDateTime::fromMSecsSinceEpoch(thing->stateValue(clientLastSeenTimeStateTypeId).toInt() * 1000);
    if (lastSeenTime.addSecs(gracePeriod * 60) < QDateTime::currentDateTime()) {
        thing->setStateValue(clientIsPresentStateTypeId, false);
    }
}

