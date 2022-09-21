/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
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

#include "integrationpluginmystrom.h"
#include "plugininfo.h"

#include <network/networkaccessmanager.h>
#include <plugintimer.h>
#include <platform/platformzeroconfcontroller.h>
#include <network/zeroconf/zeroconfservicebrowser.h>

#include <QNetworkReply>
#include <QJsonDocument>
#include <QTimer>
#include <QUrlQuery>

// API doc:
// https://api.mystrom.ch/

QList<int> supportedPlugs {
    101, // Switch CH v1
    106, // Switch CH v2
    107  // Switch EU
};

IntegrationPluginMyStrom::IntegrationPluginMyStrom()
{
}

IntegrationPluginMyStrom::~IntegrationPluginMyStrom()
{
}

void IntegrationPluginMyStrom::init()
{
    m_zeroConf = hardwareManager()->zeroConfController()->createServiceBrowser("_hap._tcp");
}

void IntegrationPluginMyStrom::discoverThings(ThingDiscoveryInfo *info)
{
    QList<QNetworkReply*> *pendingReplies = new QList<QNetworkReply*>();
    connect(info, &ThingDiscoveryInfo::finished, this, [pendingReplies](){
        delete pendingReplies;
    });

    foreach (const ZeroConfServiceEntry &entry, m_zeroConf->serviceEntries()) {
        qCDebug(dcMyStrom()) << "Found myStrom device:" << entry;
        if (entry.protocol() != QAbstractSocket::IPv4Protocol) {
            continue;
        }
        QUrl infoUrl;
        infoUrl.setScheme("http");
        infoUrl.setHost(entry.hostAddress().toString());
        infoUrl.setPath("/api/v1/info");

        QNetworkRequest request(infoUrl);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        pendingReplies->append(reply);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [=](){
            if (reply->error() != QNetworkReply::NoError) {
                finishDiscoveryReply(reply, info, pendingReplies);
                return;
            }
            QByteArray data = reply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                finishDiscoveryReply(reply, info, pendingReplies);
                return;
            }
            qCDebug(dcMyStrom) << "Info response:" << qUtf8Printable(jsonDoc.toJson());

            QVariantMap deviceInfo = jsonDoc.toVariant().toMap();
            if (supportedPlugs.contains(deviceInfo.value("type").toInt())) {
                ThingDescriptor descriptor(switchThingClassId, entry.name(), entry.hostAddress().toString());
                descriptor.setParams({Param(switchThingIdParamTypeId, entry.txt("id"))});
                info->addThingDescriptor(descriptor);
            }
            finishDiscoveryReply(reply, info, pendingReplies);
        });
    }

    if (pendingReplies->isEmpty()) {
        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginMyStrom::setupThing(ThingSetupInfo *info)
{
    QUrl infoUrl = composeUrl(info->thing(), "/api/v1/info");
    if (infoUrl.isEmpty()) {
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Device cannot be found on the network."));
        return;
    }

    QNetworkRequest request(infoUrl);
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [=](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcMyStrom()) << "Error fetching device info from myStrom device" << info->thing()->name();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error fetching device information from myStrom device."));
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcMyStrom()) << "Error parsing JSON from myStrom device:" << error.errorString() << data;
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error processing response from myStrom device."));
            return;
        }

        qCDebug(dcMyStrom()) << "Device info:" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
        QVariantMap deviceInfo = jsonDoc.toVariant().toMap();
        if (!supportedPlugs.contains(deviceInfo.value("type").toInt())) {
            qCWarning(dcMyStrom()) << "This device does not seem to be a myStrom WiFi switch";
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("This device does not seem to be a myStrom WiFi switch."));
            return;
        }
        info->finish(Thing::ThingErrorNoError);

        info->thing()->setStateValue("connected", true);

        pluginStorage()->beginGroup(info->thing()->id().toString());
        pluginStorage()->setValue("cachedAddress", infoUrl.host());
        pluginStorage()->endGroup();
    });
}

void IntegrationPluginMyStrom::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing)
    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [=]{

            foreach (Thing *thing, myThings().filterByThingClassId(switchThingClassId)) {
                QUrl url = composeUrl(thing, "/report");
                QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
                connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                connect(reply, &QNetworkReply::finished, thing, [reply, thing](){
                    if (reply->error() != QNetworkReply::NoError) {
                        qCWarning(dcMyStrom()) << "Error fetching report from myStrom device:" << reply->errorString();
                        thing->setStateValue(switchConnectedStateTypeId, false);
                        return;
                    }
                    QByteArray data = reply->readAll();
                    QJsonParseError error;
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                    if (error.error != QJsonParseError::NoError) {
                        qCWarning(dcMyStrom()) << "Error parsing JSON from myStrom device" << thing->name() << data;
                        return;
                    }
                    thing->setStateValue(switchConnectedStateTypeId, true);
                    qCDebug(dcMyStrom()) << "Switch report:" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
                    QVariantMap map = jsonDoc.toVariant().toMap();
                    thing->setStateValue(switchPowerStateTypeId, map.value("relay").toBool());
                    thing->setStateValue(switchCurrentPowerStateTypeId, map.value("power").toDouble());
                    double totalEnergyConsumed = thing->stateValue(switchTotalEnergyConsumedStateTypeId).toDouble();
                    double Ws = map.value("Ws").toDouble();
                    double kWh = Ws / 1000 / 3600;
                    totalEnergyConsumed += kWh;
                    thing->setStateValue(switchTotalEnergyConsumedStateTypeId, totalEnergyConsumed);
                });
            }
        });
    }
}

void IntegrationPluginMyStrom::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)
    if (myThings().isEmpty() && m_pluginTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}


void IntegrationPluginMyStrom::executeAction(ThingActionInfo *info)
{
    if (info->thing()->thingClassId() == switchThingClassId) {
        if (info->action().actionTypeId() == switchPowerActionTypeId) {
            bool power = info->action().paramValue(switchPowerActionPowerParamTypeId).toBool();
            QUrl powerUrl = composeUrl(info->thing(), "/relay");
            QUrlQuery query;
            query.addQueryItem("state", power ? "1" : "0");
            powerUrl.setQuery(query);

            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(powerUrl));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, this, [info, reply, power](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcMyStrom()) << "Error switching myStrom switch:" << reply->error() << reply->errorString();
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error communicating with myStrom device."));
                    return;
                }

                qCDebug(dcMyStrom()) << "Power action executed";
                info->thing()->setStateValue(switchPowerStateTypeId, power);
                info->finish(Thing::ThingErrorNoError);
            });
        }
    }
}

void IntegrationPluginMyStrom::finishDiscoveryReply(QNetworkReply *reply, ThingDiscoveryInfo *info, QList<QNetworkReply *> *pendingReplies)
{
    pendingReplies->removeAll(reply);
    if (pendingReplies->isEmpty()) {
        info->finish(Thing::ThingErrorNoError);
    }
}

QUrl IntegrationPluginMyStrom::composeUrl(Thing *thing, const QString &path)
{
    QHostAddress address;
    foreach (const ZeroConfServiceEntry &entry, m_zeroConf->serviceEntries()) {
        if (entry.protocol() == QAbstractSocket::IPv4Protocol && entry.txt("id") == thing->paramValue(switchThingIdParamTypeId).toString()) {
            address = entry.hostAddress();
            break;
        }
    }

    if (address.isNull()) {
        pluginStorage()->beginGroup(thing->id().toString());
        address = pluginStorage()->value("cachedAddress").toString();
        pluginStorage()->endGroup();
    }

    if (address.isNull()) {
        qCWarning(dcMyStrom()) << "Error finding myStrom device in the network";
        return QUrl();
    }

    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());
    url.setPath(path);
    return url;
}

