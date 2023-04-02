/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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


#include "integrationpluginbosswerk.h"
#include "plugininfo.h"

#include <plugintimer.h>
#include <network/networkdevicediscovery.h>
#include <network/networkaccessmanager.h>
#include <network/networkdevicediscoveryreply.h>

#include <QNetworkReply>
#include <QAuthenticator>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QMetaEnum>

IntegrationPluginBosswerk::IntegrationPluginBosswerk()
{

}

IntegrationPluginBosswerk::~IntegrationPluginBosswerk()
{
}

void IntegrationPluginBosswerk::discoverThings(ThingDiscoveryInfo *info)
{    
    NetworkDeviceDiscoveryReply *discoveryReply = hardwareManager()->networkDeviceDiscovery()->discover();

    // This device doesn't give much information without login credentials. In order to identify it we'll
    // do an unauthorized GET call on it and compare the server header "HTTPD" as well as the static
    // Unauthorized page which looks unique enough.
    // If this proves to not be reliable enough, one more option would be to connect to TCP port 8899 which is open
    // and responds to a proprietary binary protocol which would need to be reverse engineered first.
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, discoveryReply, &NetworkDeviceDiscoveryReply::deleteLater);
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::networkDeviceInfoAdded, info, [=](const NetworkDeviceInfo &networkDeviceInfo){
        qCDebug(dcBosswerk()) << "Probing device" << networkDeviceInfo.address();

        QUrl url("http://" + networkDeviceInfo.address().toString() + "/status.html");
        QNetworkRequest request(url);

        QNetworkReply *probeReply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(probeReply, &QNetworkReply::finished, probeReply, &QNetworkReply::deleteLater);
        connect(probeReply, &QNetworkReply::finished, info, [=](){
            QByteArray data = probeReply->readAll();
            qCDebug(dcBosswerk()) << "Probe reply from" << networkDeviceInfo.address() << ":" << probeReply->rawHeaderPairs() << data;
            if (probeReply->header(QNetworkRequest::ServerHeader) == "HTTPD" && data == "<HTML><HEAD><TITLE>401 Unauthorized</TITLE></HEAD>\n<BODY BGCOLOR=\"#cc9999\"><H4>401 Unauthorized</H4>\nAuthorization required.\n</BODY></HTML>\n") {
                qCDebug(dcBosswerk()) << "Found bosswerk MI-300/600:" << networkDeviceInfo.address();

                ThingDescriptor descriptor(mix00ThingClassId, "MI-300/600", networkDeviceInfo.hostName());
                descriptor.setParams({Param(mix00ThingMacAddressParamTypeId, networkDeviceInfo.macAddress())});
                Thing *existingThing = myThings().findByParams({Param(mix00ThingMacAddressParamTypeId, networkDeviceInfo.macAddress())});
                if (existingThing) {
                    descriptor.setThingId(existingThing->id());
                }
                info->addThingDescriptor(descriptor);
            }
        });
    });

    QTimer *timeout = new QTimer(info);
    timeout->start(28000);
    connect(timeout, &QTimer::timeout, info, [=](){
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginBosswerk::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter your login credentials."));
}

void IntegrationPluginBosswerk::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    MacAddress mac(info->params().paramValue(mix00ThingMacAddressParamTypeId).toString());

    QHash<MacAddress, NetworkDeviceInfo> cache = hardwareManager()->networkDeviceDiscovery()->cache();

    if (!cache.contains(mac)) {
        qCWarning(dcBosswerk()) << "MAC" << mac << "not found in network device cache.";
        info->finish(Thing::ThingErrorItemNotFound, QT_TR_NOOP("An error happened in the network communication."));
        return;
    }

    NetworkDeviceInfo networkDeviceInfo = cache.value(mac);

    QUrl url("http://" + username + ":" + secret + "@" + networkDeviceInfo.address().toString() + "/status.html");
    QNetworkRequest request(url);

    qCDebug(dcBosswerk) << "Requesting" << request.url();

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [=](){        
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcBosswerk()) << "Error logging in at inverter" << reply->error();
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Failed to log in at the inverter."));
            return;
        }

        QByteArray payload = reply->readAll();
        qCDebug(dcBosswerk()) << "Reply data:" << payload;

        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", secret);
        pluginStorage()->endGroup();

        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginBosswerk::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    NetworkDeviceMonitor *monitor = m_deviceMonitors.take(thing);
    if (monitor) {
        hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(monitor);
    }
    PluginTimer *timer = m_timers.take(thing);
    if (timer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(timer);
    }

    monitor = hardwareManager()->networkDeviceDiscovery()->registerMonitor(MacAddress(thing->paramValue(mix00ThingMacAddressParamTypeId).toString()));
    m_deviceMonitors.insert(thing, monitor);

    timer = hardwareManager()->pluginTimerManager()->registerTimer(5);
    m_timers.insert(thing, timer);

    connect(monitor, &NetworkDeviceMonitor::reachableChanged, thing, [this, timer, thing](bool reachable) {
        thing->setStateValue(mix00ConnectedStateTypeId, reachable);
        if (reachable) {
            timer->start();
            pollDevice(thing);
        } else {
            timer->stop();
            thing->setStateValue(mix00CurrentPowerStateTypeId, 0);
        }
    });

    connect(timer, &PluginTimer::timeout, thing, [this, thing](){
        if (m_deviceMonitors.value(thing)->reachable()) {
            pollDevice(thing);
        }
    });

    pollDevice(thing);

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginBosswerk::thingRemoved(Thing *thing)
{
    hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_deviceMonitors.take(thing));
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_timers.take(thing));
}

void IntegrationPluginBosswerk::pollDevice(Thing *thing)
{
    NetworkDeviceMonitor *monitor = m_deviceMonitors.value(thing);

    pluginStorage()->beginGroup(thing->id().toString());
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();

    QUrl url("http://" + username + ":" + password + "@" + monitor->networkDeviceInfo().address().toString() + "/status.html");
    qCDebug(dcBosswerk()) << "Requesting" << url.toString();
    QNetworkRequest request(url);

    QNetworkReply *statusReply = hardwareManager()->networkManager()->get(request);
    connect(statusReply, &QNetworkReply::finished, statusReply, &QNetworkReply::deleteLater);
    connect(statusReply, &QNetworkReply::finished, thing, [=](){
        if (statusReply->error() != QNetworkReply::NoError) {
            qCWarning(dcBosswerk) << "Error polling" << thing->name() << ":" << statusReply->error() << statusReply->errorString();
            return;
        }
        QByteArray data = statusReply->readAll();

//        qCDebug(dcBosswerk) << "Status:" << qUtf8Printable(data);
        foreach (const QString &line, QString(data).split("\n")) {
            if (line.startsWith("var ")) {
                qCDebug(dcBosswerk()) << "Data line:" << line;
            }
            if (line.startsWith("var webdata_now_p")) {
                bool ok;
                double currentPower = line.split("\"").at(1).toDouble(&ok);
                if (ok) {
                    thing->setStateValue(mix00CurrentPowerStateTypeId, -currentPower);
                }
            }
            if (line.startsWith("var webdata_total_e")) {
                bool ok;
                double totalEnergyProduced = line.split("\"").at(1).toDouble(&ok);
                if (ok && totalEnergyProduced != 0) { // When it's not producing anything, it will also give 0 for the total. Ignoring that...
                    thing->setStateValue(mix00TotalEnergyProducedStateTypeId, totalEnergyProduced);
                }
            }
            if (line.startsWith("var cover_sta_rssi")) {
                bool ok;
                QString rssiString = line.split("\"").at(1);
                rssiString.remove("%");
                int rssi = rssiString.toInt(&ok);
                if (ok) {
                    thing->setStateValue(mix00SignalStrengthStateTypeId, rssi);
                }
            }
        }
    });
}
