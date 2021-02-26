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

/*
Example reply for HOME and PRO:

  HOME: GET /daten.cfg: Length 13
    0: NET-CONTROL    ;
    1: Mo, 07.01.19 02:24:54;
    2: 18;
    3: ;
    4: 12;
    5: 1;
    6: Login: <b>user7</b> 10.10.10.128;
    7: 4.5 DE;
    8: 7648;
    9: 0;
    10: H;
    11: end;
    12: NET - Power Control

  ADV: GET /daten.cfg: Length 11
    0: 1546827738;
    1: 64;
    2: 0;
    3: 37;
    4: 1;
    5: 19789;
    6: 5;
    7: 24.7;
    8: 9;
    9: 2;
    10: end

  HUT: GET /daten.cfg: Length 18
    0: 1546830796;
    1: 93;
    2: 41;
    3: 194;
    4: 1;
    5: 12587;
    6: 14;
    7: 24.7;
    8: 9;
    9: 2;
    10: end;
    11: s;
    12: 1;
    13: 22.90;
    14: 44.4;
    15: 851;
    16: 1;
    17: 0.00;



  HOME/PRO GET strg.cfg: Length 60
    0: NET-PWRCTRL_04.5;     // thing name
    1: NET-CONTROL    ;      // hostname
    2: 10.10.10.132;         // IP
    3: 255.255.255.0;        // Netmask
    4: 10.10.10.1;           // Gateway
    5: 00:04:A3:0B:0C:3A;    // MAC
    6: 80;                   // Webcontrol port
    7: ;                     // Temp
    8: H;                    // Type
    9: ;                     // ?? (Skipped by upstream code)
    Following fields are repeated 8 times each, one for each socket
    10: Nr. 1;                // Name 1
    11: 1;                    // Stand
    12: 0;                    // Dis
    13: Anfangsstatus;        // Info
    14: ;                     // TK

    end;
    NET - Power Control"

  ADV: GET /strg.cfg: Length 58
    0: Nr.1;0;0;28;K;
    5: Nr.2;0;0;28;;
    10: Nr.3;1;0;27;;
    15: Nr.4;0;0;28;;
    20: Nr.5;0;0;28;;
    25: Nr.6;0;0;28;;
    30: Nr.7;0;0;0;;
    35: Nr.8;0;0;28;;
    40: end;
    41: 0;
    42: 0;
    43: 0;
    44: 1;
    45: 0;
    46: 0;
    47: 0;
    48: 1;
    49: 10122;
    50: 10123;
    51: 10124;
    52: 8657;
    53: 10126;
    54: 10127;
    55: 10128;
    56: 8659;

*/

#include "integrationpluginanel.h"
#include "plugininfo.h"
#include "plugintimer.h"
#include "discovery.h"

#include <network/networkaccessmanager.h>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QUrlQuery>

QHash<ThingClassId, StateTypeId> connectedStateTypeIdMap = {
    {netPwrCtlHomeThingClassId, netPwrCtlHomeConnectedStateTypeId},
    {netPwrCtlProThingClassId, netPwrCtlProConnectedStateTypeId},
    {netPwrCtlAdvThingClassId, netPwrCtlAdvConnectedStateTypeId},
    {netPwrCtlHutThingClassId, netPwrCtlHutConnectedStateTypeId},
    {socketThingClassId, socketConnectedStateTypeId}
};

QHash<ThingClassId, ParamTypeId> macAddressParamTypeIdMap = {
    {netPwrCtlHomeThingClassId, netPwrCtlHomeThingMacAddressParamTypeId},
    {netPwrCtlProThingClassId, netPwrCtlProThingMacAddressParamTypeId},
    {netPwrCtlAdvThingClassId, netPwrCtlAdvThingMacAddressParamTypeId},
    {netPwrCtlHutThingClassId, netPwrCtlHutThingMacAddressParamTypeId}
};

IntegrationPluginAnel::IntegrationPluginAnel()
{

}

IntegrationPluginAnel::~IntegrationPluginAnel()
{
}

void IntegrationPluginAnel::init()
{
    m_discovery = new Discovery(this);
    // Every time the discovery finish, we'll check if we need to update cached ip/port
    connect(m_discovery, &Discovery::finished, this, [=](bool error){
        if (error) {
            return;
        }
        foreach (Thing *thing, myThings()) {
            if (!macAddressParamTypeIdMap.contains(thing->thingClassId())) {
                continue;
            }
            QString macAddress = thing->paramValue(macAddressParamTypeIdMap.value(thing->thingClassId())).toString();

            // Upgrading configured things from old version where the MAC param was still holding an IP address instead
            if (!QHostAddress(macAddress).isNull()) {
                qCDebug(dcAnelElektronik()) << "Upgrading configuration for:" << thing->name();
                foreach (const Discovery::Result &result, m_discovery->results()) {
                    if (result.ipAddress == macAddress) {
                        macAddress = result.macAddress;
                        thing->setParamValue(macAddressParamTypeIdMap.value(thing->thingClassId()), macAddress);
                    }
                }
            }


            if (m_discovery->results().contains(macAddress)) {
                Discovery::Result result = m_discovery->results().value(macAddress);
                qCDebug(dcAnelElektronik()) << "Updating IP address for" << thing->name() << "to" << result.ipAddress << ":" << result.port;
                pluginStorage()->beginGroup(thing->id().toString());
                pluginStorage()->setValue("cachedAddress", result.ipAddress);
                pluginStorage()->setValue("cachedPort", result.port);
                pluginStorage()->endGroup();
            }
        }
    });
}

void IntegrationPluginAnel::discoverThings(ThingDiscoveryInfo *info)
{
    connect(m_discovery, &Discovery::finished, info, [=](bool error){
        if (error) {
            qCWarning(dcAnelElektronik()) << "Error sending discovery";
            //: Error discovering devices
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error sending data to the network."));
            return;
        }
        foreach (const Discovery::Result &result, m_discovery->results()) {
            ThingDescriptor d(info->thingClassId(), result.name, result.ipAddress);
            ParamTypeId macAddressParamTypeId = macAddressParamTypeIdMap.value(info->thingClassId());
            ParamList params;
            params << Param(macAddressParamTypeId, result.macAddress);

            d.setParams(params);
            foreach (Thing *existingThing, myThings().filterByThingClassId(info->thingClassId())) {
                if (existingThing->paramValue(macAddressParamTypeId).toString() == result.macAddress) {
                    qCDebug(dcAnelElektronik()) << "Already have" << result.macAddress << result.ipAddress << "in configured things.";
                    d.setThingId(existingThing->id());
                    break;
                }
            }
            info->addThingDescriptor(d);
        }
        info->finish(Thing::ThingErrorNoError);
    });
    m_discovery->discover();
}

void IntegrationPluginAnel::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter the login credentials for your NET-PWRCTRL device."));
}

void IntegrationPluginAnel::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{
    QString macAddress = info->params().paramValue(macAddressParamTypeIdMap.value(info->thingClassId())).toString();

    QString ipAddress = m_discovery->results().value(macAddress).ipAddress;
    int port = m_discovery->results().value(macAddress).port;

    QNetworkRequest request;
    request.setUrl(QUrl(QString("http://%1:%2/strg.cfg").arg(ipAddress).arg(port)));
    request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username).arg(password).toUtf8().toBase64());
    qCDebug(dcAnelElektronik()) << "ConfirmPairing fetching:" << request.url() << request.rawHeader("Authorization");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [=](){
        if (reply->error() == QNetworkReply::NoError) {
            pluginStorage()->beginGroup(info->thingId().toString());
            pluginStorage()->setValue("cachedAddress", ipAddress);
            pluginStorage()->setValue("cachedPort", port);
            pluginStorage()->setValue("username", username);
            pluginStorage()->setValue("password", password);
            pluginStorage()->endGroup();
            info->finish(Thing::ThingErrorNoError);
        } else {
            //: Error pairing thing
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Wrong username or password."));
        }
    });
}

void IntegrationPluginAnel::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == netPwrCtlHomeThingClassId
            || thing->thingClassId() == netPwrCtlProThingClassId) {
        setupHomeProDevice(info);
        return;
    }
    if (thing->thingClassId() == netPwrCtlAdvThingClassId
            || thing->thingClassId() == netPwrCtlHutThingClassId) {
        setupAdvDevice(info);
        return;
    }

    if (thing->thingClassId() == socketThingClassId) {
        qCDebug(dcAnelElektronik()) << "Setting up" << thing->name();
        if (!m_pollTimer) {
            m_pollTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
            connect(m_pollTimer, &PluginTimer::timeout, this, &IntegrationPluginAnel::refreshStates);
        }
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    qCWarning(dcAnelElektronik) << "Unhandled ThingClass in setupDevice" << thing->thingClassId();
    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginAnel::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing)
    if (!m_discoverTimer) {
        m_discoverTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_discoverTimer, &PluginTimer::timeout, m_discovery, &Discovery::discover);
    }
}

void IntegrationPluginAnel::thingRemoved(Thing *thing)
{
    qCDebug(dcAnelElektronik) << "Device removed" << thing->name();
    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pollTimer);
        m_pollTimer = nullptr;
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_discoverTimer);
        m_discoverTimer = nullptr;
    }
}

void IntegrationPluginAnel::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == socketThingClassId) {
        if (action.actionTypeId() == socketPowerActionTypeId) {

            Thing *parentDevice = myThings().findById(thing->parentId());

            pluginStorage()->beginGroup(parentDevice->id().toString());
            QString ipAddress = pluginStorage()->value("cachedAddress").toString();
            int port = pluginStorage()->value("cachedPort").toInt();
            QString username = pluginStorage()->value("username").toString();
            QString password = pluginStorage()->value("password").toString();
            pluginStorage()->endGroup();

            QUrl url(QString("http://%1:%2/ctrl.htm").arg(ipAddress).arg(port));
            QNetworkRequest request(url);
            request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username, password).toUtf8().toBase64());
            request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
            QByteArray data = QString("F%1=%2").arg(thing->paramValue(socketThingNumberParamTypeId).toString(), action.param(socketPowerActionPowerParamTypeId).value().toBool() == true ? "1" : "0").toUtf8();
            QNetworkReply *reply = hardwareManager()->networkManager()->post(request, data);
            qCDebug(dcAnelElektronik()) << "Requesting:" << url.toString() << request.rawHeader("Authorization");
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [reply, info](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcAnelElektronik()) << "Execute action failed:" << reply->error() << reply->errorString();
                    info->finish(Thing::ThingErrorHardwareFailure);
                    return;
                }
                qCDebug(dcAnelElektronik()) << "Execute action done.";
                info->finish(Thing::ThingErrorNoError);
            });
            return;
        }
    }
    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginAnel::refreshStates()
{
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() == netPwrCtlHomeThingClassId
                || thing->thingClassId() == netPwrCtlProThingClassId) {
            refreshHomePro(thing);
        }
        if (thing->thingClassId() == netPwrCtlAdvThingClassId
                || thing->thingClassId() == netPwrCtlHutThingClassId) {
            refreshAdv(thing);
        }
    }
}

void IntegrationPluginAnel::setConnectedState(Thing *thing, bool connected)
{
    thing->setStateValue(connectedStateTypeIdMap.value(thing->thingClassId()), connected);
    foreach (Thing *child, myThings()) {
        if (child->parentId() == thing->id()) {
            child->setStateValue(connectedStateTypeIdMap.value(child->thingClassId()), connected);
        }
    }
}

void IntegrationPluginAnel::setupHomeProDevice(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    QString macAddress = thing->paramValue(macAddressParamTypeIdMap.value(thing->thingClassId())).toString();

    // Run a discovery and wait for it to finish before setting it up.
    m_discovery->discover();
    connect(m_discovery, &Discovery::finished, info, [=](){
        pluginStorage()->beginGroup(thing->id().toString());
        QString ipAddress = pluginStorage()->value("cachedAddress").toString();
        int port = pluginStorage()->value("cachedPort").toInt();
        QString username = pluginStorage()->value("username").toString();
        QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();

        QNetworkRequest request;
        request.setUrl(QUrl(QString("http://%1:%2/strg.cfg").arg(ipAddress).arg(port)));
        request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username).arg(password).toUtf8().toBase64());
        qCDebug(dcAnelElektronik()) << "SetupDevice fetching:" << request.url() << request.rawHeader("Authorization") << username << password;
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
            StateTypeId connectedStateTypeId = connectedStateTypeIdMap.value(info->thing()->thingClassId());
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcAnelElektronik()) << "Error fetching state for" << info->thing()->name() << reply->error() << reply->errorString();
                info->thing()->setStateValue(connectedStateTypeId, false);
                //: Error setting up thing
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The thing rejected our connection. Please check the configured network ports."));
                return;
            }
            info->thing()->setStateValue(connectedStateTypeId, true);

            QByteArray data = reply->readAll();

            QStringList parts = QString(data).split(';');

            int startIndex = parts.indexOf("end") - 58;
            if (startIndex < 0 || !parts.at(startIndex).startsWith("NET-PWRCTRL") || parts.length() < 60) {
                qCWarning(dcAnelElektronik()) << "Bad data from panel:" << data << "Length:" << parts.length();
                //: Error setting up thing
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unexpected data received from NET-PWRCTL device. Perhaps it's running an old firmware?"));
                return;
            }

            // At this point we're done with gathering information about the panel. Setup defintely succeeded for the gateway thing
            info->finish(Thing::ThingErrorNoError);

            // If we haven't set up childs for this gateway yet, let's do it now
            foreach (Thing *child, myThings()) {
                if (child->parentId() == info->thing()->id()) {
                    // Already have childs for this panel. We're done here
                    return;
                }
            }

            // Lets add the child devices now
            int childs = -1;
            QString type = parts.at(startIndex + 8);
            if (type == "H") {
                childs = 3;
            } else {
                childs = 8;
            }

            QList<ThingDescriptor> descriptorList;
            for (int i = 0; i < childs; i++) {
                QString deviceName = parts.at(startIndex + 10 + i);
                ThingDescriptor d(socketThingClassId, deviceName, info->thing()->name(), info->thing()->id());
                d.setParams(ParamList() << Param(socketThingNumberParamTypeId, i));
                descriptorList << d;
            }
            emit autoThingsAppeared(descriptorList);
        });

    });
}

void IntegrationPluginAnel::setupAdvDevice(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    QString macAddress = thing->paramValue(macAddressParamTypeIdMap.value(thing->thingClassId())).toString();

    // Run a discovery and wait for it to finish before trying to connect
    m_discovery->discover();
    connect(m_discovery, &Discovery::finished, info, [=](){
        pluginStorage()->beginGroup(thing->id().toString());
        QString ipAddress = pluginStorage()->value("cachedAddress").toString();
        int port = pluginStorage()->value("cachedPort").toInt();
        QString username = pluginStorage()->value("username").toString();
        QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();

        QNetworkRequest request;
        request.setUrl(QUrl(QString("http://%1:%2/strg.cfg").arg(ipAddress).arg(port)));
        request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username).arg(password).toUtf8().toBase64());
        qCDebug(dcAnelElektronik()) << "SetupDevice fetching:" << request.url() << request.rawHeader("Authorization");
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [this, info, reply]() {
            StateTypeId connectedStateTypeId = connectedStateTypeIdMap.value(info->thing()->thingClassId());
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcAnelElektronik()) << "Error fetching state for" << info->thing()->name() << reply->error() << reply->errorString();
                info->thing()->setStateValue(connectedStateTypeId, false);
                //: Error setting up thing
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The thing rejected our connection. Please check the configured network ports."));
                return;
            }
            info->thing()->setStateValue(connectedStateTypeId, true);

            QByteArray data = reply->readAll();

            QStringList parts = QString(data).split(';');

            int startIndex = parts.indexOf("end") - 40;
            if (startIndex < 0 || parts.length() < 58) {
                qCWarning(dcAnelElektronik()) << "Bad data from panel:" << data << "Length:" << parts.length();
                //: Error setting up thing
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unexpected data received from NET-PWRCTL device. Perhaps it's running an old firmware?"));
                return;
            }

            // At this point we're done with gathering information about the panel. Setup defintely succeeded for the gateway thing
            info->finish(Thing::ThingErrorNoError);

            // If we haven't set up childs for this gateway yet, let's do it now
            foreach (Thing *child, myThings()) {
                if (child->parentId() == info->thing()->id()) {
                    // Already have childs for this panel. We're done here
                    return;
                }
            }

            QList<ThingDescriptor> descriptorList;
            for (int i = 0; i < 8; i++) {
                QString deviceName = parts.at(startIndex + (i * 5));
                ThingDescriptor d(socketThingClassId, deviceName, info->thing()->name(), info->thing()->id());
                d.setParams(ParamList() << Param(socketThingNumberParamTypeId, i));
                descriptorList << d;
            }
            emit autoThingsAppeared(descriptorList);
        });
    });
}

void IntegrationPluginAnel::refreshHomePro(Thing *thing)
{
    pluginStorage()->beginGroup(thing->id().toString());
    QString ipAddress = pluginStorage()->value("cachedAddress").toString();
    int port = pluginStorage()->value("cachedPort").toInt();
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();

    QUrl url(QString("http://%1:%2/strg.cfg").arg(ipAddress).arg(port));

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username, password).toUtf8().toBase64());
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [this, thing, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcAnelElektronik()) << "Error fetching state for" << thing->name();
            setConnectedState(thing, false);
            return;
        }
        QByteArray data = reply->readAll();
//        qCDebug(dcAnelElektronik()) << "States reply:" << data;

        QStringList parts = QString(data).split(';');
        int startIndex = parts.indexOf("end") - 58;
        if (startIndex < 0 || !parts.at(startIndex).startsWith("NET-PWRCTRL")) {
            qCWarning(dcAnelElektronik()) << "Bad data from Panel" << thing->name() << data;
            // This happens sometimes when multiple clients are talking to the panel... Just ignore it...
            return;
        }
        setConnectedState(thing, true);

        // The temp sensor seems to have a bit of jitter. Reduce sample rate to avoid spamming the log
        quint32 samples = thing->property("tempSamples").toUInt();
        if (samples % 15 == 0 && thing->thingClassId() == netPwrCtlProThingClassId) {
            bool ok;
            double tempValue = parts.at(startIndex + 7).toDouble(&ok);
            if (ok) {
                thing->setStateValue(netPwrCtlProTemperatureStateTypeId, tempValue);
            } else {
                qCWarning(dcAnelElektronik()) << "Error reading temperature value from data:" << parts.at(startIndex + 7);
            }
        }
        thing->setProperty("tempSamples", ++samples);

        foreach (Thing *child, myThings()) {
            if (child->parentId() == thing->id()) {
                int number = child->paramValue(socketThingNumberParamTypeId).toInt();
                child->setStateValue(socketPowerStateTypeId, parts.value(startIndex + 20 + number).toInt() == 1);
            }
        }
    });
}

void IntegrationPluginAnel::refreshAdv(Thing *thing)
{
    pluginStorage()->beginGroup(thing->id().toString());
    QString ipAddress = pluginStorage()->value("cachedAddress").toString();
    int port = pluginStorage()->value("cachedPort").toInt();
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();

    QUrl url(QString("http://%1:%2/strg.cfg").arg(ipAddress).arg(port));

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username, password).toUtf8().toBase64());
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [this, thing, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcAnelElektronik()) << "Error fetching state for" << thing->name();
            setConnectedState(thing, false);
            return;
        }
        QByteArray data = reply->readAll();
//        qCDebug(dcAnelElektronik()) << "States reply:" << data;

        QStringList parts = QString(data).split(';');
        int startIndex = parts.indexOf("end") - 40;
        if (startIndex < 0 || parts.count() < 58) {
            qCWarning(dcAnelElektronik()) << "Bad data from Panel" << thing->name() << data << "Length:" << parts.length();
            // This happens sometimes when multiple clients are talking to the panel... Just ignore it...
            return;
        }
        setConnectedState(thing, true);

        foreach (Thing *child, myThings()) {
            if (child->parentId() == thing->id()) {
                int number = child->paramValue(socketThingNumberParamTypeId).toInt();
                child->setStateValue(socketPowerStateTypeId, parts.value(startIndex + (number * 5) + 1).toInt() == 1);
            }
        }

        // The temp sensor seems to have a bit of jitter. Reduce sample rate to avoid spamming the log
        quint32 samples = thing->property("tempSamples").toUInt();
        if (samples % 15 == 0) {
            refreshAdvTemp(thing);
        }
        thing->setProperty("tempSamples", ++samples);

    });
}

void IntegrationPluginAnel::refreshAdvTemp(Thing *thing)
{
    pluginStorage()->beginGroup(thing->id().toString());
    QString ipAddress = pluginStorage()->value("cachedAddress").toString();
    int port = pluginStorage()->value("cachedPort").toInt();
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();

    QUrl url(QString("http://%1:%2/daten.cfg").arg(ipAddress).arg(port));

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username, password).toUtf8().toBase64());
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [this, thing, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcAnelElektronik()) << "Error fetching temp for" << thing->name();
            setConnectedState(thing, false);
            return;
        }
        QByteArray data = reply->readAll();
        qCDebug(dcAnelElektronik()) << "Temp reply:" << data;

        QStringList parts = QString(data).split(';');
        int startIndex = parts.indexOf("end") - 10;
        if (startIndex < 0) {
            qCWarning(dcAnelElektronik()) << "Bad data from Panel" << thing->name() << data << "Length:" << parts.length();
            // This happens sometimes when multiple clients are talking to the panel... Just ignore it...
            return;
        }

        if (thing->thingClassId() == netPwrCtlAdvThingClassId) {
            bool ok;
            double temp = parts.at(7).toDouble(&ok);
            if (ok) {
                thing->setStateValue(netPwrCtlAdvTemperatureStateTypeId, temp);
            } else {
                qCWarning(dcAnelElektronik()) << "Error fetching temperature value:" << data;
            }
        } else { // HUT
            if (parts.length() < 18) {
                qCWarning(dcAnelElektronik()) << "Data too short for HUT device" << data;
                return;
            }
            bool ok;
            double temp = parts.at(13).toDouble(&ok);
            if (ok) {
                thing->setStateValue(netPwrCtlHutTemperatureStateTypeId, temp);
            } else {
                qCWarning(dcAnelElektronik()) << "Error fetching temperature value:" << data;
            }
            double humidity = parts.at(14).toDouble(&ok);
            if (ok) {
                thing->setStateValue(netPwrCtlHutHumidityStateTypeId, humidity);
            } else {
                qCWarning(dcAnelElektronik()) << "Error fetching humidity value:" << data;
            }
            int lux = parts.at(15).toInt(&ok);
            if (ok) {
                thing->setStateValue(netPwrCtlHutLightIntensityStateTypeId, lux);
            } else {
                qCWarning(dcAnelElektronik()) << "Error fetching light intensity value:" << data;
            }
        }
    });
}
