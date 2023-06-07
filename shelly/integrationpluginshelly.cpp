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

#include "integrationpluginshelly.h"
#include "plugininfo.h"
#include "shellyjsonrpcclient.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QHostAddress>
#include <QJsonDocument>
#include <QColor>
#include <QNetworkInterface>

#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"
#include "network/mqtt/mqttprovider.h"
#include "network/mqtt/mqttchannel.h"

#include "plugintimer.h"

#include "qmath.h"

#include "network/zeroconf/zeroconfservicebrowser.h"
#include "platform/platformzeroconfcontroller.h"

#include <coap/coap.h>

// Maps update status strings: Shelly <-> nymea
static QHash<QString, QString> updateStatusMap = {
    {"idle", "idle"},
    {"pending", "available"},
    {"updating", "updating"},
    {"unknown", "idle"}
};

IntegrationPluginShelly::IntegrationPluginShelly()
{
}

IntegrationPluginShelly::~IntegrationPluginShelly()
{
}

void IntegrationPluginShelly::init()
{
    m_zeroconfBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_http._tcp");

    m_coap = new Coap(this);
    connect(m_coap, &Coap::multicastMessageReceived, this, &IntegrationPluginShelly::onMulticastMessageReceived);
    joinMulticastGroup();
}

void IntegrationPluginShelly::discoverThings(ThingDiscoveryInfo *info)
{
    foreach (const ZeroConfServiceEntry &entry, m_zeroconfBrowser->serviceEntries()) {
        qCDebug(dcShelly()) << "Have entry" << entry;
        QRegExp namePattern;
        if (info->thingClassId() == shelly1ThingClassId) {
            namePattern = QRegExp("^shelly1-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyPlus1ThingClassId) {
            namePattern = QRegExp("^ShellyPlus1-[0-9A-Z]+$");
        } else if (info->thingClassId() == shelly1pmThingClassId) {
            namePattern = QRegExp("^shelly1pm-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyPlus1pmThingClassId) {
            namePattern = QRegExp("^ShellyPlus1PM-[0-9A-Z]+$");
        } else if (info->thingClassId() == shelly1lThingClassId) {
            namePattern = QRegExp("^shelly1l-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyPlugThingClassId) {
            namePattern = QRegExp("^shellyplug(-s)?-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyPlusPlugThingClassId) {
            namePattern = QRegExp("^(ShellyPlusPlugS|ShellyPlug(US|IT|UK))-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyRgbw2ThingClassId) {
            namePattern = QRegExp("^shellyrgbw2-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyDimmerThingClassId) {
            namePattern = QRegExp("^(shellydimmer(2)?|ShellyVintage)-[0-9A-Z]+$");
        } else if (info->thingClassId() == shelly2ThingClassId) {
            namePattern = QRegExp("^shellyswitch-[0-9A-Z]+$");
        } else if (info->thingClassId() == shelly25ThingClassId) {
            namePattern = QRegExp("^(shellyswitch25|ShellyPlus2PM)-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyButton1ThingClassId) {
            namePattern = QRegExp("^shellybutton1-[0-9-A-Z]+$");
        } else if (info->thingClassId() == shellyEmThingClassId) {
            namePattern = QRegExp("^shellyem-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyEm3ThingClassId) {
            namePattern = QRegExp("^shellyem3-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyPro3EMThingClassId) {
            namePattern = QRegExp("^ShellyPro3EM-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyHTThingClassId) {
            namePattern = QRegExp("shellyht-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyI3ThingClassId) {
            namePattern = QRegExp("shellyix3-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyMotionThingClassId) {
            namePattern = QRegExp("shellymotionsensor-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyTrvThingClassId) {
            namePattern = QRegExp("shellytrv-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyFloodThingClassId) {
            namePattern = QRegExp("^shellyflood-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyFloodThingClassId) {
            namePattern = QRegExp("^shellysmoke-[0-9A-Z]+$");
        } else if (info->thingClassId() == shellyGasThingClassId) {
            namePattern = QRegExp("^shellygas-[0-9A-Z]+$");
        }
        if (!entry.name().contains(namePattern)) {
            continue;
        }

        ThingDescriptor descriptor(info->thingClassId(), entry.name(), entry.hostAddress().toString());
        ParamList params;
        ThingClass thingClass = supportedThings().findById(info->thingClassId());

        params << Param(thingClass.paramTypes().findByName("id").id(), entry.name());
        params << Param(thingClass.paramTypes().findByName("username").id(), "");
        params << Param(thingClass.paramTypes().findByName("password").id(), "");
        if (!thingClass.paramTypes().findByName("rollerMode").id().isNull()) {
            params << Param(thingClass.paramTypes().findByName("rollerMode").id(), false);
        }
        descriptor.setParams(params);

        Things existingThings = myThings().filterByParam(thingClass.paramTypes().findByName("id").id(), entry.name());
        if (existingThings.count() == 1) {
            qCInfo(dcShelly()) << "This existing shelly:" << entry;
            descriptor.setThingId(existingThings.first()->id());
        } else {
            qCInfo(dcShelly()) << "Found new shelly:" << entry;
        }

        info->addThingDescriptor(descriptor);
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginShelly::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter the password for your Shelly device. By default this is empty."));
}

void IntegrationPluginShelly::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{
    Q_UNUSED(username)

    qCDebug(dcShelly) << "Confirm pairing called";
    ThingClass thingClass = supportedThings().findById(info->thingClassId());
    QString shellyId = info->params().paramValue(thingClass.paramTypes().findByName("id").id()).toString();
    ZeroConfServiceEntry zeroConfEntry;
    foreach (const ZeroConfServiceEntry &entry, m_zeroconfBrowser->serviceEntries()) {
        if (entry.name() == shellyId) {
            zeroConfEntry = entry;
        }
    }
    QHostAddress address = zeroConfEntry.hostAddress();

    if (address.isNull()) {
        qCWarning(dcShelly()) << "Unable to determine Shelly's network address. Failed to set up device.";
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to find the thing in the network."));
        return;
    }

    ShellyJsonRpcClient *client = new ShellyJsonRpcClient(info);
    client->open(address, "admin", password, shellyId);
    connect(client, &ShellyJsonRpcClient::stateChanged, info, [info, client, this, password](QAbstractSocket::SocketState state) {
        qCDebug(dcShelly()) << "Websocket state changed:" << state;
        // GetDeviceInfo wouldn't require authentication if enabled, so if the setup is changed to fetch some info from GetDeviceInfo,
        // make sure to not just replace the GetStatus call, or authentication verification won't work any more.
        ShellyRpcReply *reply = client->sendRequest("Shelly.GetStatus");
        connect(reply, &ShellyRpcReply::finished, info, [info, client, this, password](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
            if (status != ShellyRpcReply::StatusSuccess) {
                qCWarning(dcShelly) << "Error during shelly paring";
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }
            qCDebug(dcShelly) << "Pairing successful!";
            pluginStorage()->beginGroup(info->thingId().toString());
            pluginStorage()->setValue("password", password);
            pluginStorage()->endGroup();

            info->finish(Thing::ThingErrorNoError);
        });
    });
}

void IntegrationPluginShelly::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (!thing->thingClass().paramTypes().findByName("id").id().isNull()) {

        QString shellyId = info->thing()->paramValue("id").toString();
        if (isGen2(shellyId)) {
            setupGen2(info);
        } else {
            setupGen1(info);
        }

        return;
    }

    setupShellyChild(info);
}

void IntegrationPluginShelly::postSetupThing(Thing *thing)
{
    if (!m_statusUpdateTimer) {
        m_statusUpdateTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_statusUpdateTimer, &PluginTimer::timeout, this, &IntegrationPluginShelly::updateStatus);
    }

    if (thing->parentId().isNull()) {
        if (isGen2(thing->paramValue("id").toString())) {
            fetchStatusGen2(thing);
        } else {
            fetchStatusGen1(thing);
        }
    }
}

void IntegrationPluginShelly::thingRemoved(Thing *thing)
{
    if (myThings().isEmpty() && m_statusUpdateTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_statusUpdateTimer);
        m_statusUpdateTimer = nullptr;
    }
    if (myThings().isEmpty() && m_reconfigureTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconfigureTimer);
        m_reconfigureTimer = nullptr;
    }
    if (m_rpcClients.contains(thing)) {
        m_rpcClients.remove(thing); // Deleted by parenting
    }

    if (thing->parentId().isNull()) { // Only parents (gen1 and gen2) store stuff in the storage
        pluginStorage()->beginGroup(thing->id().toString());
        pluginStorage()->remove("");
        pluginStorage()->endGroup();
    }

    qCDebug(dcShelly()) << "Device removed" << thing->name();
}

void IntegrationPluginShelly::executeAction(ThingActionInfo *info)
{
    // We'll always execute actions on the main gateway thing. If info->thing() has a parent, use that.
    Thing *thing = info->thing()->parentId().isNull() ? info->thing() : myThings().findById(info->thing()->parentId());
    Action action = info->action();
    QString shellyId = thing->paramValue("id").toString();

    QUrl url;
    url.setScheme("http");
    url.setHost(getIP(info->thing()).toString());
    if (!thing->paramValue("id").toString().isEmpty()) {
        url.setUserName(thing->paramValue("username").toString());
        url.setPassword(thing->paramValue("password").toString());
    }

    ActionType actionType = thing->thingClass().actionTypes().findById(action.actionTypeId());
    if (actionType.name() == "reboot") {
        if (isGen2(shellyId)) {
            ShellyRpcReply *reply = m_rpcClients.value(thing)->sendRequest("Shelly.Reboot");
            connect(reply, &ShellyRpcReply::finished, info, [info](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
                info->finish(status == ShellyRpcReply::StatusSuccess ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        } else {
            url.setPath("/reboot");
            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [info, reply](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcShelly()) << "Failed to execute reboot action:" << reply->error() << reply->errorString();
                }
                info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        }
        return;
    }

    if (actionType.name() == "performUpdate") {
        if (isGen2(shellyId)) {
            ShellyRpcReply *reply = m_rpcClients.value(thing)->sendRequest("Shelly.Update");
            connect(reply, &ShellyRpcReply::finished, info, [info](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
                info->finish(status == ShellyRpcReply::StatusSuccess ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        } else {
            url.setPath("/ota");
            QUrlQuery query;
            query.addQueryItem("update", "true");
            url.setQuery(query);
            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [info, reply](){
                info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        }
        return;
    }

    if (action.actionTypeId() == shellyRgbw2PowerActionTypeId) {
        ParamTypeId colorPowerParamTypeId = shellyRgbw2PowerActionPowerParamTypeId;
        bool on = action.param(colorPowerParamTypeId).value().toBool();
        url.setPath("/color/0");
        QUrlQuery query;
        query.addQueryItem("turn", on ? "on" : "off");
        url.setQuery(query);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply, on](){
            info->thing()->setStateValue("power", on);
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }


    if (action.actionTypeId() == shellyRgbw2ColorActionTypeId) {
        ParamTypeId colorParamTypeId = shellyRgbw2ColorActionColorParamTypeId;
        QColor color = action.param(colorParamTypeId).value().value<QColor>();
        url.setPath("/color/0");
        QUrlQuery query;
        query.addQueryItem("red", QString::number(color.red()));
        query.addQueryItem("green", QString::number(color.green()));
        query.addQueryItem("blue", QString::number(color.blue()));
        url.setQuery(query);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply, color](){
            info->thing()->setStateValue("color", color);
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (action.actionTypeId() == shellyRgbw2WhiteChannelActionTypeId) {
        uint whiteValue = action.paramValue(shellyRgbw2WhiteChannelActionWhiteChannelParamTypeId).toUInt();
        url.setPath("/color/0");
        QUrlQuery query;
        query.addQueryItem("white", QString::number(whiteValue));
        url.setQuery(query);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply, whiteValue](){
            info->thing()->setStateValue(shellyRgbw2WhiteChannelStateTypeId, whiteValue);
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (action.actionTypeId() == shellyRgbw2ColorTemperatureActionTypeId) {
        int ct = action.param(shellyRgbw2ColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
        url.setPath("/color/0");
        QUrlQuery query;
        query.addQueryItem("red", QString::number(qMin(255, ct * 255 / 50)));
        query.addQueryItem("green", "0");
        query.addQueryItem("blue", QString::number(qMax(0, ct - 50) * 255 / 50));
        query.addQueryItem("white", "255");
        url.setQuery(query);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply, ct](){
            info->thing()->setStateValue("colorTemperature", ct);
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (action.actionTypeId() == shellyRgbw2BrightnessActionTypeId) {
        ParamTypeId brightnessParamTypeId = shellyRgbw2BrightnessActionBrightnessParamTypeId;
        int brightness = action.param(brightnessParamTypeId).value().toInt();
        url.setPath("/color/0");
        QUrlQuery query;
        query.addQueryItem("gain", QString::number(brightness));
        url.setQuery(query);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply, brightness](){
            info->thing()->setStateValue("brightness", brightness);
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (action.actionTypeId() == shellyDimmerPowerActionTypeId) {
        ParamTypeId powerParamTypeId = shellyDimmerPowerActionPowerParamTypeId;
        bool on = action.param(powerParamTypeId).value().toBool();
        url.setPath("/light/0");
        QUrlQuery query;
        query.addQueryItem("turn", on ? "on" : "off");
        url.setQuery(query);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply, on](){
            info->thing()->setStateValue("power", on);
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (action.actionTypeId() == shellyDimmerBrightnessActionTypeId) {
        ParamTypeId brightnessParamTypeId = shellyDimmerBrightnessActionBrightnessParamTypeId;
        int brightness = action.param(brightnessParamTypeId).value().toInt();
        url.setPath("/light/0");
        QUrlQuery query;
        query.addQueryItem("brightness", QString::number(brightness));
        url.setQuery(query);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply, brightness](){
            info->thing()->setStateValue("brightness", brightness);
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (action.actionTypeId() == shellyTrvTargetTemperatureActionTypeId) {
        double targetValue = action.paramValue(shellyTrvTargetTemperatureActionTargetTemperatureParamTypeId).toDouble();
        url.setPath(QString("/thermostats/0"));
        QUrlQuery query;
        query.addQueryItem("target_t", QString::number(targetValue));
        query.addQueryItem("target_t_enabled", "true");
        url.setQuery(query);
        qCDebug(dcShelly()) << "Requesting:" << url;
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply, targetValue](){
            // The Shelly TRV seems to reply with OK, but then takes ages to actually set the value
            // If we send another value within that time frame, it will again reply with OK but just ognore it...
            // As a workaround we'll make nymea wait for a second until allowing to send the next action.
            info->thing()->setStateValue(shellyTrvTargetTemperatureStateTypeId, targetValue);
            Thing::ThingError status = reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure;
            QTimer::singleShot(1000, info, [info, status](){
                info->finish(status);
            });
        });
        return;
    }
    if (action.actionTypeId() == shellyTrvValvePositionActionTypeId) {
        int targetValue = action.paramValue(shellyTrvValvePositionActionValvePositionParamTypeId).toInt();
        url.setPath(QString("/thermostats/0"));
        QUrlQuery query;
        query.addQueryItem("pos", QString::number(targetValue));
        url.setQuery(query);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply, targetValue](){
            // The Shelly TRV seems to reply with OK, but then takes ages to actually set the value
            // If we send another value within that time frame, it will again reply with OK but just ognore it...
            // As a workaround we'll make nymea wait for a second until allowing to send the next action.
            info->thing()->setStateValue(shellyTrvValvePositionStateTypeId, targetValue);
            Thing::ThingError status = reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure;
            QTimer::singleShot(1000, info, [info, status](){
                info->finish(status);
            });
        });
        return;
    }
    if (action.actionTypeId() == shellyTrvBoostActionTypeId) {
        url.setPath(QString("/thermostats/0"));
        QUrlQuery query;
        query.addQueryItem("boost_minutes", thing->setting(shellyTrvSettingsBoostDurationParamTypeId).toString());
        url.setQuery(query);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply](){
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (action.actionTypeId() == shellyRollerOpenActionTypeId) {
        if (isGen2(shellyId)) {
            QVariantMap params;
            int channelNbr = info->thing()->paramValue(shellyRollerThingChannelParamTypeId).toInt() - 1;
            params.insert("id", channelNbr);
            ShellyRpcReply *reply = m_rpcClients.value(thing)->sendRequest("Cover.Open", params);
            connect(reply, &ShellyRpcReply::finished, info, [info](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
                info->finish(status == ShellyRpcReply::StatusSuccess ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        } else {
            url.setPath(QString("/roller/%1").arg(info->thing()->paramValue(shellyRollerThingChannelParamTypeId).toInt() - 1));
            QUrlQuery query;
            query.addQueryItem("go", "open");
            url.setQuery(query);
            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [info, reply](){
                info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        }
        return;
    }

    if (action.actionTypeId() == shellyRollerCloseActionTypeId) {
        if (isGen2(shellyId)) {
            QVariantMap params;
            int channelNbr = info->thing()->paramValue(shellyRollerThingChannelParamTypeId).toInt() - 1;
            params.insert("id", channelNbr);
            ShellyRpcReply *reply = m_rpcClients.value(thing)->sendRequest("Cover.Close", params);
            connect(reply, &ShellyRpcReply::finished, info, [info](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
                info->finish(status == ShellyRpcReply::StatusSuccess ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        } else {
            url.setPath(QString("/roller/%1").arg(info->thing()->paramValue(shellyRollerThingChannelParamTypeId).toInt() - 1));
            QUrlQuery query;
            query.addQueryItem("go", "close");
            url.setQuery(query);
            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [info, reply](){
                info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        }
        return;
    }

    if (action.actionTypeId() == shellyRollerStopActionTypeId) {
        if (isGen2(shellyId)) {
            QVariantMap params;
            int channelNbr = info->thing()->paramValue(shellyRollerThingChannelParamTypeId).toInt() - 1;
            params.insert("id", channelNbr);
            ShellyRpcReply *reply = m_rpcClients.value(thing)->sendRequest("Cover.Stop", params);
            connect(reply, &ShellyRpcReply::finished, info, [info](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
                info->finish(status == ShellyRpcReply::StatusSuccess ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        } else {
            url.setPath(QString("/roller/%1").arg(info->thing()->paramValue(shellyRollerThingChannelParamTypeId).toInt() - 1));
            QUrlQuery query;
            query.addQueryItem("go", "stop");
            url.setQuery(query);
            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [info, reply](){
                info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        }
        return;
    }

    if (action.actionTypeId() == shellyRollerCalibrateActionTypeId) {
        if (isGen2(shellyId)) {
            QVariantMap params;
            int channelNbr = info->thing()->paramValue(shellyRollerThingChannelParamTypeId).toInt() - 1;
            params.insert("id", channelNbr);
            ShellyRpcReply *reply = m_rpcClients.value(thing)->sendRequest("Cover.Calibrate", params);
            connect(reply, &ShellyRpcReply::finished, info, [info](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
                info->finish(status == ShellyRpcReply::StatusSuccess ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        } else {
            url.setPath(QString("/roller/%1/calibrate").arg(info->thing()->paramValue(shellyRollerThingChannelParamTypeId).toInt() - 1));
            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [info, reply](){
                info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        }
        return;
    }

    if (action.actionTypeId() == shellyRollerPercentageActionTypeId) {
        if (isGen2(shellyId)) {
            QVariantMap params;
            int channelNbr = info->thing()->paramValue(shellyRollerThingChannelParamTypeId).toInt() - 1;
            int positionTarget = info->action().paramValue(shellyRollerPercentageActionPercentageParamTypeId).toInt();
            params.insert("id", channelNbr);
            params.insert("pos", positionTarget);
            ShellyRpcReply *reply = m_rpcClients.value(thing)->sendRequest("Cover.GoToPosition", params);
            connect(reply, &ShellyRpcReply::finished, info, [info](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
                info->finish(status == ShellyRpcReply::StatusSuccess ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        } else {
            url.setPath(QString("/roller/%1").arg(info->thing()->paramValue(shellyRollerThingChannelParamTypeId).toInt() - 1));
            QUrlQuery query;
            query.addQueryItem("go", "to_pos");
            query.addQueryItem("roller_pos", info->action().paramValue(shellyRollerPercentageActionPercentageParamTypeId).toString());
            url.setQuery(query);
            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [info, reply](){
                info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        }
        return;
    }

    if (action.actionTypeId() == shellyEmResetActionTypeId || action.actionTypeId() == shellyEm3ResetActionTypeId) {
        url.setPath("/reset_data");
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply](){
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (action.actionTypeId() == shellyGasSelfTestActionTypeId) {
        url.setPath("/self_test");
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply](){
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (action.actionTypeId() == shellyGasMuteActionTypeId) {
        url.setPath("/mute");
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply](){
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (action.actionTypeId() == shellyGasUnmuteActionTypeId) {
        url.setPath("/unmute");
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply](){
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (action.actionTypeId() == shellyGasOpenValveActionTypeId) {
        url.setPath("/settings/valve/0");
        QUrlQuery query;
        query.addQueryItem("go", "open");
        url.setQuery(query);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply](){
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (action.actionTypeId() == shellyGasCloseValveActionTypeId) {
        url.setPath("/settings/valve/0");
        QUrlQuery query;
        query.addQueryItem("go", "close");
        url.setQuery(query);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply](){
            info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        });
        return;
    }

    if (actionType.name() == "power") {
        int relay = 1;
        QHash<ActionTypeId, int> actionChannelMap = {
            {shelly25Channel1ActionTypeId, 1},
            {shelly25Channel2ActionTypeId, 2}
        };
        if (!thing->thingClass().paramTypes().findByName("channel").id().isNull()) {
            relay = thing->paramValue("channel").toInt();
        } else if (actionChannelMap.contains(action.actionTypeId())) {
            relay = actionChannelMap.value(action.actionTypeId());
        }

        ParamTypeId powerParamTypeId = actionType.id();
        bool on = action.param(powerParamTypeId).value().toBool();

        if (isGen2(shellyId)) {
            QVariantMap params;
            params.insert("id", relay - 1);
            params.insert("on", on);
            ShellyRpcReply *reply = m_rpcClients.value(thing)->sendRequest("Switch.Set", params); // Switch.Set not supported by Shelly Plus 2PM in shutter mode; will return error "No handler for Switch.Set"
            connect(reply, &ShellyRpcReply::finished, info, [info](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
                info->finish(status == ShellyRpcReply::StatusSuccess ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        } else {
            url.setPath(QString("/relay/%1").arg(relay - 1));
            QUrlQuery query;
            query.addQueryItem("turn", on ? "on" : "off");
            url.setQuery(query);
            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [info, reply, on](){
                info->thing()->setStateValue("power", on);
                info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
        }
        return;
    }

    qCWarning(dcShelly()) << "Unhandled execute action" << info->action().actionTypeId() << "call for device" << thing;
}

void IntegrationPluginShelly::joinMulticastGroup()
{
    if (m_coap->joinMulticastGroup()) {
        qCInfo(dcShelly()) << "Joined CoIoT multicast group";
    } else {
        qCWarning(dcShelly()) << "Failed to join CoIoT multicast group. Retrying in 5 seconds...";
        // FIXME: It would probably be better to monitor the network interfaces and re-join if necessary
        QTimer::singleShot(5000, m_coap, [this](){
            joinMulticastGroup();
        });
    }
}

void IntegrationPluginShelly::onMulticastMessageReceived(const QHostAddress &source, const CoapPdu &pdu)
{
    Q_UNUSED(source)
//    qCDebug(dcShelly()) << "Multicast message received" << source << pdu;
    if (pdu.reqRspCode() != 0x1e) {
        // Not a shelly CoIoT status message (ReqRsp code "0.30")
        return;
    }
    if (!pdu.hasOption(static_cast<CoapOption::Option>(3321))) {
        qCDebug(dcShelly()) << "Received a Shelly CoIoT status message but dev id option is missing.";
        return;
    }

    QByteArray deviceId = pdu.option(static_cast<CoapOption::Option>(3321)).data();
    QStringList parts = QString(deviceId).split("#");
    if (parts.length() != 3) {
        qCDebug(dcShelly) << "Unexpected deviceId option format";
        return;
    }

    QString shellyId = parts.at(1);
    Thing *thing = nullptr;
    foreach (Thing *t, myThings()) {
        if (t->paramValue("id").toString().endsWith(shellyId)) {
            thing = t;
            break;
        }
    }
    if (!thing) {
        qCDebug(dcShelly()) << "Received a status update message for a shelly we don't know.";
        return;
    }

    qCDebug(dcShelly()) << "Status update message for" << thing->name();
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(pdu.payload(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcShelly()) << "JSON parse error in CoIoT status report:" << error.errorString();
        return;
    }

    qCDebug(dcShelly) << "CoIoT multicast message for" << thing->name() << ":" << qUtf8Printable(jsonDoc.toJson());
    QVariantMap map = jsonDoc.toVariant().toMap();

    thing->setStateValue("connected", true);
    foreach (Thing *child, myThings().filterByParentId(thing->id())) {
        child->setStateValue("connected", true);
    }
    // Remember when we recieved the last update
    thing->setProperty("lastCoIoTMessage", QDateTime::currentDateTime());


    // Some states are calculated from multiple values in the list and we'll need to keep them temporarily
    int red = 0, green = 0, blue = 0, white = 0;
    QString inputEvent1String, inputEvent2String, inputEvent3String;
    int inputEvent1Count = 0, inputEvent2Count = 0, inputEvent3Count = 0;

    foreach (const QVariant &entry, map.value("G").toList()) {
        int id = entry.toList().at(1).toInt();
        QString value = entry.toList().at(2).toString();
        switch (id) {
        case 1101: // power (on/off) for channel 1
            if (thing->hasState("power")) {
                thing->setStateValue("power", value.toInt() == 1);
            } else if (thing->hasState("channel1")) {
                thing->setStateValue("channel1", value.toInt() == 1);
            }
            break;
        case 1103: // Roller position
            foreach (Thing *roller, myThings().filterByParentId(thing->id()).filterByInterface("extendedshutter")) {
                roller->setStateValue(shellyRollerPercentageStateTypeId, 100 - value.toUInt());
            }
            break;
        case 1105:
            thing->setStateValue("valveState", value);
            break;
        case 1201: // power (on/off) for channel 2
            thing->setStateValue("channel2", value.toInt() == 1);
            break;
        case 2101: { // input state for channel 1
            int channel = 1;
            bool on = value.toInt() == 1;
            if (thing->thingClassId() == shellyI3ThingClassId) {
                thing->setStateValue(shellyI3Input1StateTypeId, on);
                break;
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellySwitchThingClassId).filterByParam(shellySwitchThingChannelParamTypeId, channel)) {
                if (child->stateValue(shellySwitchPowerStateTypeId).toBool() != on) {
                    child->setStateValue(shellySwitchPowerStateTypeId, on);
                    emit emitEvent(Event(shellySwitchPressedEventTypeId, child->id()));
                }
            }
            break;
        }
        case 2102: // input event for channel 1
            inputEvent1String = value;
            break;
        case 2103:
            inputEvent1Count = value.toInt();
            break;
        case 2201: { // input state for channel 2
            int channel = 2;
            bool on = value.toInt() == 1;
            if (thing->thingClassId() == shellyI3ThingClassId) {
                thing->setStateValue(shellyI3Input2StateTypeId, on);
                break;
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellySwitchThingClassId).filterByParam(shellySwitchThingChannelParamTypeId, channel)) {
                if (child->stateValue(shellySwitchPowerStateTypeId).toBool() != on) {
                    child->setStateValue(shellySwitchPowerStateTypeId, on);
                    emit emitEvent(Event(shellySwitchPressedEventTypeId, child->id()));
                }
            }
            break;
        }
        case 2202: // input event for channel 2
            inputEvent2String = value;
            break;
        case 2203:
            inputEvent2Count = value.toInt();
            break;
        case 2301: // Input state for channel 3
            thing->setStateValue(shellyI3Input1StateTypeId, value.toInt() == 1);
            break;
        case 2302: // Input event for channel 3
            inputEvent3String = value;
            break;
        case 2303:
            inputEvent3Count = value.toInt();
            break;
        case 3101:
            thing->setStateValue("temperature", value.toDouble());
            break;
        case 3103: // This is target tempererature for the TRV, but humidity for other sensors
            if (thing->thingClassId() == shellyTrvThingClassId) {
                thing->setStateValue("targetTemperature", value.toDouble());
            } else {
                thing->setStateValue("humidity", value.toDouble());
            }
            break;
        case 3106:
            thing->setStateValue("lightIntensity", value.toInt());
            break;
        case 3107:
            thing->setStateValue("gasLevel", value.toInt());
            break;
        case 3111:
            if (value.toInt() == -1) { // When connected to power surce
                thing->setStateValue("batteryLevel", 100);
            } else {
                thing->setStateValue("batteryLevel", value.toInt());
            }
            thing->setStateValue("batteryCritical", thing->stateValue("batteryLevel").toUInt() < 10);
            break;
        case 3113:
            thing->setStateValue("sensorOperation", value);
            break;
        case 3114:
            thing->setStateValue("selfTest", value);
            break;
        case 3121:
            thing->setStateValue("valvePosition", value.toUInt());
            thing->setStateValue("heatingOn", value.toUInt() > 0);
            break;
        case 3122:
            thing->setStateValue("boost", value.toUInt() > 0);
            break;
        case 4101: // power meter for channel 1
            if (thing->hasState("currentPower")) {
                thing->setStateValue("currentPower", value);
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyPowerMeterChannelThingClassId).filterByParam(shellyPowerMeterChannelThingChannelParamTypeId, 1)) {
                child->setStateValue(shellyPowerMeterChannelCurrentPowerStateTypeId, value.toDouble());
            }
            break;
        case 4201: // power meter for channel 2
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyPowerMeterChannelThingClassId).filterByParam(shellyPowerMeterChannelThingChannelParamTypeId, 2)) {
                child->setStateValue(shellyPowerMeterChannelCurrentPowerStateTypeId, value.toDouble());
            }
            break;
        case 4102: // roller current power
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyRollerThingClassId).filterByParam(shellyRollerThingChannelParamTypeId, 1)) {
                child->setStateValue(shellyRollerCurrentPowerStateTypeId, value);
            }
            break;
        case 4103: // totalEnergyConsumed channel 1
            if (thing->hasState("totalEnergyConsumed")) {
                thing->setStateValue("totalEnergyConsumed", value.toDouble() / 60 / 1000);
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyPowerMeterChannelThingClassId).filterByParam(shellyPowerMeterChannelThingChannelParamTypeId, 1)) {
                child->setStateValue(shellyPowerMeterChannelTotalEnergyConsumedStateTypeId, value.toDouble() / 60 / 1000); // Wmin -> kWh
            }
            break;
        case 4203: // totalEnergyConsumed channel 2
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyPowerMeterChannelThingClassId).filterByParam(shellyPowerMeterChannelThingChannelParamTypeId, 2)) {
                child->setStateValue(shellyPowerMeterChannelTotalEnergyConsumedStateTypeId, value.toDouble() / 60 / 1000); // Wmin -> kWh
            }
            break;
        case 4105:
            // 3EM has a state on its own, EM has a child thing per channel
            if (thing->hasState("currentPowerPhaseA")) {
                thing->setStateValue("currentPowerPhaseA", value.toDouble());
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyEmChannelThingClassId).filterByParam(shellyEmChannelThingChannelParamTypeId, 1)) {
                child->setStateValue(shellyEmChannelCurrentPowerStateTypeId, value.toDouble());
            }
            break;
        case 4205:
            if (thing->hasState("currentPowerPhaseB")) {
                thing->setStateValue("currentPowerPhaseB", value.toDouble());
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyEmChannelThingClassId).filterByParam(shellyEmChannelThingChannelParamTypeId, 2)) {
                child->setStateValue(shellyEmChannelCurrentPowerStateTypeId, value.toDouble());
            }
            break;
        case 4305:
            if (thing->hasState("currentPowerPhaseC")) {
                thing->setStateValue("currentPowerPhaseC", value.toDouble());
            }
            break;
        case 4106:
            // 3EM has a state on its own, EM has a child thing per channel
            if (thing->hasState("energyConsumedPhaseA")) {
                thing->setStateValue("energyConsumedPhaseA", value.toDouble() / 1000);
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyEmChannelThingClassId).filterByParam(shellyEmChannelThingChannelParamTypeId, 1)) {
                child->setStateValue(shellyEmChannelTotalEnergyConsumedStateTypeId, value.toDouble() / 1000);
            }
            break;
        case 4206:
            // 3EM has a state on its own, EM has a child thing per channel
            if (thing->hasState("energyConsumedPhaseB")) {
                thing->setStateValue("energyConsumedPhaseB", value.toDouble() / 1000);
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyEmChannelThingClassId).filterByParam(shellyEmChannelThingChannelParamTypeId, 2)) {
                child->setStateValue(shellyEmChannelTotalEnergyConsumedStateTypeId, value.toDouble() / 1000);
            }
            break;
        case 4306:
            if (thing->hasState("energyConsumedPhaseC")) {
                thing->setStateValue("energyConsumedPhaseC", value.toDouble() / 1000);
            }
            break;
        case 4107:
            if (thing->hasState("energyProducedPhaseA")) {
                thing->setStateValue("energyProducedPhaseA", value.toDouble() / 1000);
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyEmChannelThingClassId).filterByParam(shellyEmChannelThingChannelParamTypeId, 1)) {
                child->setStateValue(shellyEmChannelTotalEnergyProducedStateTypeId, value.toDouble() / 1000);
            }
            break;
        case 4207:
            if (thing->hasState("energyProducedPhaseB")) {
                thing->setStateValue("energyProducedPhaseB", value.toDouble() / 1000);
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyEmChannelThingClassId).filterByParam(shellyEmChannelThingChannelParamTypeId, 2)) {
                child->setStateValue(shellyEmChannelTotalEnergyProducedStateTypeId, value.toDouble() / 1000);
            }
            break;
        case 4307:
            if (thing->hasState("energyProducedPhaseC")) {
                thing->setStateValue("energyProducedPhaseC", value.toDouble() / 1000);
            }
            break;
        case 4108:
            if (thing->hasState("voltagePhaseA")) {
                thing->setStateValue("voltagePhaseA", value.toDouble());
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyEmChannelThingClassId).filterByParam(shellyEmChannelThingChannelParamTypeId, 1)) {
                child->setStateValue(shellyEmChannelVoltagePhaseAStateTypeId, value.toDouble() / 1000);
            }
            break;
        case 4208:
            if (thing->hasState("voltagePhaseB")) {
                thing->setStateValue("voltagePhaseB", value.toDouble());
            }
            foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyEmChannelThingClassId).filterByParam(shellyEmChannelThingChannelParamTypeId, 2)) {
                child->setStateValue(shellyEmChannelVoltagePhaseAStateTypeId, value.toDouble() / 1000);
            }
            break;
        case 4308:
            if (thing->hasState("voltagePhaseC")) {
                thing->setStateValue("voltagePhaseC", value.toDouble());
            }
            break;
        case 4109:
            if (thing->hasState("currentPhaseA")) {
                thing->setStateValue("currentPhaseA", value.toDouble());
            }
            break;
        case 4209:
            if (thing->hasState("currentPhaseB")) {
                thing->setStateValue("currentPhaseB", value.toDouble());
            }
            break;
        case 4309:
            if (thing->hasState("currentPhaseC")) {
                thing->setStateValue("currentPhaseC", value.toDouble());
            }
            break;
        case 4110:
            if (thing->hasState("powerFactorPhaseA")) {
                thing->setStateValue("powerFactorPhaseA", value.toDouble());
            }
            break;
        case 4210:
            if (thing->hasState("powerFactorPhaseB")) {
                thing->setStateValue("powerFactorPhaseB", value.toDouble());
            }
            break;
        case 4310:
            if (thing->hasState("powerFactorPhaseC")) {
                thing->setStateValue("powerFactorPhaseC", value.toDouble());
            }
            break;
        case 5101: // dimmable lights brightness
        case 5102: // rgb lights gain
            thing->setStateValue("brightness", value.toInt());
            break;
        case 5105:
            red = value.toInt();
            break;
        case 5106:
            green = value.toInt();
            break;
        case 5107:
            blue = value.toInt();
            break;
        case 5108:
            white = value.toInt();
            break;
        case 6105:
            thing->setStateValue("fireDetected", value.toInt() == 1);
            break;
        case 6106:
            thing->setStateValue("waterDetected", value.toInt() == 1);
            break;
        case 6107:
            thing->setStateValue("isPresent", value.toInt() == 1);
            break;
        case 6108:
            thing->setStateValue("gas", value);
            break;
        case 6110:
            thing->setStateValue("vibration", value.toInt() == 1);
            break;
        }
    }
    if (thing->thingClassId() == shellyEm3ThingClassId) {
        thing->setStateValue(shellyEm3CurrentPowerStateTypeId,
                             thing->stateValue(shellyEm3CurrentPowerPhaseAStateTypeId).toDouble() +
                             thing->stateValue(shellyEm3CurrentPowerPhaseBStateTypeId).toDouble() +
                             thing->stateValue(shellyEm3CurrentPowerPhaseCStateTypeId).toDouble());
        double totalConsumption = thing->stateValue(shellyEm3EnergyConsumedPhaseAStateTypeId).toDouble() +
                thing->stateValue(shellyEm3EnergyConsumedPhaseBStateTypeId).toDouble() +
                thing->stateValue(shellyEm3EnergyConsumedPhaseCStateTypeId).toDouble();
        if (totalConsumption >= 0) {
            thing->setStateValue(shellyEm3TotalEnergyConsumedStateTypeId, totalConsumption);
        } else {
            // There seems to be a bug in the Shelly 3EM that occationally gives -0.001 for the totals.
            qCWarning(dcShelly()) << "Detected negative value on shelly total consumption counter. Ignoring value." << qUtf8Printable(jsonDoc.toJson());
        }
        double totalProduction = thing->stateValue(shellyEm3EnergyProducedPhaseAStateTypeId).toDouble() +
                thing->stateValue(shellyEm3EnergyProducedPhaseBStateTypeId).toDouble() +
                thing->stateValue(shellyEm3EnergyProducedPhaseCStateTypeId).toDouble();
        if (totalProduction >= 0) {
            thing->setStateValue(shellyEm3TotalEnergyProducedStateTypeId, totalProduction);
        } else {
            // There seems to be a bug in the Shelly 3EM that occationally gives -0.001 for the totals.
            qCWarning(dcShelly()) << "Detected negative value on shelly total production counter. Ignoring value." << qUtf8Printable(jsonDoc.toJson());
        }
    }
    if (thing->thingClassId() == shellyEmThingClassId) {
        foreach (Thing *child, myThings().filterByParentId(thing->id()).filterByThingClassId(shellyEmChannelThingClassId)) {
            double power = child->stateValue(shellyEmChannelCurrentPowerStateTypeId).toDouble();
            double voltage = child->stateValue(shellyEmChannelVoltagePhaseAStateTypeId).toDouble();
            if (qFuzzyCompare(voltage, 0) == false) {
                double calcCurrent = power/voltage;
                child->setStateValue(shellyEmChannelCurrentPhaseAStateTypeId, calcCurrent);
            } else {
                child->setStateValue(shellyEmChannelCurrentPhaseAStateTypeId, 0);
            }
        }
    }
    if (thing->thingClassId() == shellyRgbw2ThingClassId) {
        thing->setStateValue(shellyRgbw2ColorStateTypeId, QColor(red, green, blue));
        thing->setStateValue(shellyRgbw2WhiteChannelStateTypeId, white);
    }

    handleInputEvent(thing, "1", inputEvent1String, inputEvent1Count);
    handleInputEvent(thing, "2", inputEvent2String, inputEvent2Count);
    handleInputEvent(thing, "3", inputEvent3String, inputEvent3Count);

    if (thing->thingClassId() == shelly2ThingClassId || thing->thingClassId() == shelly25ThingClassId) {
        foreach (Thing *roller, myThings().filterByInterface("extendedshutter").filterByParentId(thing->id())) {
            bool moving = thing->stateValue("channel1").toBool() || thing->stateValue("channel2").toBool();
            roller->setStateValue(shellyRollerMovingStateTypeId, moving);
        }
    }

    // Fetching info about signal strength, battery level for sleepy devices as they may be still awake when sending us something.
    if (thing->thingClassId() == shellyFloodThingClassId ||
            thing->thingClassId() == shellyTrvThingClassId) {
        fetchStatusGen1(thing);
    }
}

void IntegrationPluginShelly::updateStatus()
{
    foreach (Thing *thing, myThings().filterByParentId(ThingId())) {
        if (!thing->setupComplete()) {
            continue;
        }

        if (isGen2(thing->paramValue("id").toString())) {
            fetchStatusGen2(thing);
        } else {
            //Skipping sleepy devices, as they won't reply to cyclic requests.
            if (thing->thingClassId() == shellyFloodThingClassId
                    || thing->thingClassId() == shellyTrvThingClassId) {
                continue;
            }

            fetchStatusGen1(thing);
        }
    }
}

void IntegrationPluginShelly::fetchStatusGen1(Thing *thing)
{
    QHostAddress address = getIP(thing);
    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());
    url.setPath("/status");
    url.setUserName(thing->paramValue("username").toString());
    url.setPassword(thing->paramValue("password").toString());
    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [this, thing, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcShelly()) << "Unable to update status for" << thing->name() << reply->error() << reply->errorString();
            if (!thing->hasState("batteryLevel")) {
                thing->setStateValue("connected", false);
                foreach (Thing *child, myThings().filterByParentId(thing->id())) {
                    child->setStateValue("connected", false);
                }
            }
            return;
        }

        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcShelly()) << "Failed to parse status reply for" << thing->name() << error.error << error.errorString();
            return;
        }

        qCDebug(dcShelly()) << "status reply:" << qUtf8Printable(jsonDoc.toJson());

        QVariantMap map = jsonDoc.toVariant().toMap();

        QVariantMap wifiMap = map.value("wifi_sta").toMap();
        int rssi = wifiMap.value("rssi").toInt();
        int signalStrength = qMin(100, qMax(0, (rssi + 100) * 2));
        thing->setStateValue("signalStrength", signalStrength);
        foreach (Thing *child, myThings().filterByParentId(thing->id())) {
            child->setStateValue("signalStrength", signalStrength);
        }


        QVariantMap updateMap = map.value("update").toMap();
        thing->setStateValue("currentVersion", updateMap.value("old_version").toString());
        thing->setStateValue("availableVersion", updateMap.value("new_version").toString());
        thing->setStateValue("updateStatus", updateStatusMap.value(updateMap.value("status").toString()));


        // Sometimes, some shellies just stop to send CoIoT messages until they are rebooted...
        // If communication to the shelly per se works fine, but we didn't receive anything in more than a minute,
        // let's reconfigure coap and reboot the shelly
        if (thing->property("lastCoIoTMessage").toDateTime().addSecs(10 * 60) < QDateTime::currentDateTime()) {
            qCInfo(dcShelly()) << "Shelly" << thing->name() << "didn't send us a CoIoT message in a minute. Reconfiguring CoIoT and rebooting it.";
            QUrlQuery query;
            QHostAddress address = getIP(thing);
            query.addQueryItem("coiot_enable", "true");
            if (thing->paramValue("coapMode").toString() == "unicast") {
                QHostAddress ourAddress;
                foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
                    foreach (const QNetworkAddressEntry &addressEntry, interface.addressEntries()) {
                        if (address.isInSubnet(addressEntry.ip(), addressEntry.prefixLength())) {
                            ourAddress = addressEntry.ip();
                            break;
                        }
                    }
                }
                if (!ourAddress.isNull()) {
                    query.addQueryItem("coiot_peer", ourAddress.toString() + ":5683");
                } else {
                    qCWarning(dcShelly) << "Unable to determine a matching interface for CoIoT unicast. Falling back to multicast.";
                    query.addQueryItem("coiot_peer", "mcast");
                }
            } else {
                query.addQueryItem("coiot_peer", "mcast");
            }
            QNetworkRequest setCoIoTRequest = createHttpRequest(thing, "/settings", query);
            QNetworkReply *reply = hardwareManager()->networkManager()->get(setCoIoTRequest);
            connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, thing, [this, thing, reply](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcShelly()) << "Failed to reconfigure coap on shelly" << thing->name();
                }

                QNetworkRequest rebootRequest = createHttpRequest(thing, "/reboot");
                QNetworkReply *reply = hardwareManager()->networkManager()->get(rebootRequest);
                connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
                connect(reply, &QNetworkReply::finished, thing, [reply](){
                    if (reply->error() != QNetworkReply::NoError) {
                        qCWarning(dcShelly()) << "Failed to send reboot request to shelly.";
                    }
                });
            });
        }
    });
}

void IntegrationPluginShelly::fetchStatusGen2(Thing *thing)
{
    ShellyJsonRpcClient *client = m_rpcClients.value(thing);
    ShellyRpcReply *statusReply = client->sendRequest("Shelly.GetStatus");
    connect(statusReply, &ShellyRpcReply::finished, thing, [thing, this](ShellyRpcReply::Status status, const QVariantMap &response){
        if (status != ShellyRpcReply::StatusSuccess) {
            qCWarning(dcShelly()) << "Error updating status from shelly:" << status;
            return;
        }
        qCDebug(dcShelly()) << thing->name() << "Status reply:" << response;
        int signalStrength = qMin(100, qMax(0, (response.value("wifi").toMap().value("rssi").toInt() + 100) * 2));
        thing->setStateValue("connected", true);
        thing->setStateValue("signalStrength", signalStrength);
        foreach (Thing *child, myThings().filterByParentId(thing->id())) {
            child->setStateValue("connected", true);
            child->setStateValue("signalStrength", signalStrength);
        }
    });

    ShellyRpcReply *infoReply = client->sendRequest("Shelly.GetDeviceInfo");
    connect(infoReply, &ShellyRpcReply::finished, thing, [thing](ShellyRpcReply::Status status, const QVariantMap &response){
        if (status != ShellyRpcReply::StatusSuccess) {
            qCWarning(dcShelly()) << "Error updating device info from shelly:" << status;
            return;
        }
        qCDebug(dcShelly()) << thing->name() << "GetDeviceInfo reply:" << response;
        thing->setStateValue("currentVersion", response.value("ver").toString());
    });
    ShellyRpcReply *updateReply = client->sendRequest("Shelly.CheckForUpdate");
    connect(updateReply, &ShellyRpcReply::finished, thing, [thing](ShellyRpcReply::Status status, const QVariantMap &response){
        if (status != ShellyRpcReply::StatusSuccess) {
            qCWarning(dcShelly()) << "Error chcking for updates from shelly:" << status;
            return;
        }
        qCDebug(dcShelly()) << thing->name() << "CheckForUpdate reply:" << response;
        if (response.contains("stable")) {
            thing->setStateValue("availableVersion", response.value("stable").toMap().value("version").toString());
            thing->setStateValue("updateStatus", "available");
        } else {
            thing->setStateValue("availableVersion", "");
            thing->setStateValue("updateStatus", "idle");
        }
    });

}

void IntegrationPluginShelly::setupGen1(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QHostAddress address = getIP(thing);

    if (address.isNull()) {
        qCWarning(dcShelly()) << "Unable to determine Shelly's network address. Failed to set up device.";
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to find the thing in the network."));
        return;
    }

    QString shellyId = info->thing()->paramValue("id").toString();

    bool rollerMode = false;
    if (info->thing()->thingClassId() == shelly2ThingClassId || info->thing()->thingClassId() == shelly25ThingClassId) {
        rollerMode = info->thing()->paramValue("rollerMode").toBool();
    }

    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());
    url.setPort(80);
    url.setPath("/settings");
    if (!thing->paramValue("username").toString().isEmpty()) {
        url.setUserName(info->thing()->paramValue("username").toString());
        url.setPassword(info->thing()->paramValue("password").toString());
    }

    QUrlQuery query;
    query.addQueryItem("coiot_enable", "true");
    if (thing->paramValue("coapMode").toString() == "unicast") {
        QHostAddress ourAddress;
        foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
            foreach (const QNetworkAddressEntry &addressEntry, interface.addressEntries()) {
                if (address.isInSubnet(addressEntry.ip(), addressEntry.prefixLength())) {
                    ourAddress = addressEntry.ip();
                    break;
                }
            }
        }
        if (!ourAddress.isNull()) {
            query.addQueryItem("coiot_peer", ourAddress.toString() + ":5683");
        } else {
            qCWarning(dcShelly) << "Unable to determine a matching interface for CoIoT unicast. Falling back to multicast.";
            query.addQueryItem("coiot_peer", "mcast");
        }
    } else {
        query.addQueryItem("coiot_peer", "mcast");
    }

    // Make sure the shelly 2.5 is in the mode we expect it to be (roller or relay)
    if (info->thing()->thingClassId() == shelly25ThingClassId || info->thing()->thingClassId() == shelly2ThingClassId) {
        query.addQueryItem("mode", rollerMode ? "roller" : "relay");
    }

    url.setQuery(query);
    QNetworkRequest request(url);

    qCDebug(dcShelly()) << "Connecting to" << url.toString();
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, info, reply, address, rollerMode](){
        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(dcShelly) << "Error connecting to shelly:" << reply->error() << reply->errorString();
            if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
                info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Username and password not set correctly."));
            } else {
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error connecting to Shelly device."));
            }
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcShelly()) << "Error parsing settings reply" << error.errorString() << "\n" << data;
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unexpected data received from Shelly device."));
            return;
        }
        qCDebug(dcShelly()) << "Settings data" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
        QVariantMap settingsMap = jsonDoc.toVariant().toMap();

        if (info->thing()->thingClassId() == shellyPlugThingClassId) {
            info->thing()->setSettingValue(shellyPlugSettingsDefaultStateParamTypeId, settingsMap.value("relays").toList().first().toMap().value("default_state").toString());
        } else if (info->thing()->thingClassId() == shellyButton1ThingClassId) {
            info->thing()->setSettingValue(shellyButton1SettingsRemainAwakeParamTypeId, settingsMap.value("remain_awake").toInt());
            info->thing()->setSettingValue(shellyButton1SettingsStatusLedEnabledParamTypeId, !settingsMap.value("led_status_disable").toBool());
            info->thing()->setSettingValue(shellyButton1SettingsLongpushMaxDurationParamTypeId, settingsMap.value("longpush_duration_ms").toMap().value("max").toUInt());
            info->thing()->setSettingValue(shellyButton1SettingsMultipushTimeBetweenPushesParamTypeId, settingsMap.value("multipush_time_between_pushes_ms").toMap().value("max").toUInt());
        } else  if (info->thing()->thingClassId() == shellyI3ThingClassId) {
            info->thing()->setSettingValue(shellyI3SettingsLongpushMinDurationParamTypeId, settingsMap.value("longpush_duration_ms").toMap().value("min").toUInt());
            info->thing()->setSettingValue(shellyI3SettingsLongpushMaxDurationParamTypeId, settingsMap.value("longpush_duration_ms").toMap().value("max").toUInt());
            info->thing()->setSettingValue(shellyI3SettingsMultipushTimeBetweenPushesParamTypeId, settingsMap.value("multipush_time_between_pushes_ms").toMap().value("max").toUInt());
        } else if (info->thing()->thingClassId() == shellyTrvThingClassId) {
            info->thing()->setSettingValue(shellyTrvSettingsChildLockParamTypeId, settingsMap.value("child_lock").toBool());
            info->thing()->setSettingValue(shellyTrvSettingsDisplayFlippedParamTypeId, settingsMap.value("display").toMap().value("flipped").toBool());
            info->thing()->setSettingValue(shellyTrvSettingsDisplayBrightnessParamTypeId, settingsMap.value("display").toMap().value("brightness").toUInt());
        } else if (info->thing()->thingClassId() == shellyGasThingClassId) {
            info->thing()->setSettingValue(shellyGasSettingsBuzzerVolumeParamTypeId, settingsMap.value("set_volume").toUInt());
        } else if (info->thing()->thingClassId() == shellyFloodThingClassId) {
            info->thing()->setSettingValue(shellyFloodSettingsRainSensorParamTypeId, settingsMap.value("rain_sensor").toBool());
        }

        ThingDescriptors autoChilds;

        // Autogenerate some childs if this thing has no childs yet
        if (myThings().filterByParentId(info->thing()->id()).isEmpty()) {
            // Always create the switch thing if we don't have one yet for shellies with input (1, 1pm etc)
            if (info->thing()->thingClassId() == shelly1ThingClassId
                    || info->thing()->thingClassId() == shelly1pmThingClassId) {
                ThingDescriptor switchChild(shellySwitchThingClassId, info->thing()->name() + " switch", QString(), info->thing()->id());
                switchChild.setParams(ParamList() << Param(shellySwitchThingChannelParamTypeId, 1));
                autoChilds.append(switchChild);
            }

            // Create 2 switches for some that have 2
            if (info->thing()->thingClassId() == shelly2ThingClassId
                    || info->thing()->thingClassId() == shelly25ThingClassId
                    || (info->thing()->thingClassId() == shellyDimmerThingClassId && info->thing()->paramValue(shellyDimmerThingIdParamTypeId).toString().startsWith("shellydimmer")) // Don't create chids for shelly vintage
                    || info->thing()->thingClassId() == shelly1lThingClassId) {
                ThingDescriptor switchChild(shellySwitchThingClassId, info->thing()->name() + " switch 1", QString(), info->thing()->id());
                switchChild.setParams(ParamList() << Param(shellySwitchThingChannelParamTypeId, 1));
                autoChilds.append(switchChild);
                ThingDescriptor switch2Child(shellySwitchThingClassId, info->thing()->name() + " switch 2", QString(), info->thing()->id());
                switch2Child.setParams(ParamList() << Param(shellySwitchThingChannelParamTypeId, 2));
                autoChilds.append(switch2Child);
            }

            if (rollerMode) {
                ThingDescriptor rollerShutterChild(shellyRollerThingClassId, info->thing()->name() + " connected shutter", QString(), info->thing()->id());
                rollerShutterChild.setParams(ParamList() << Param(shellyRollerThingChannelParamTypeId, 1));
                autoChilds.append(rollerShutterChild);

            // Create 2 measurement channels for shelly 2.5 (unless in roller mode)
            } else if (info->thing()->thingClassId() == shelly25ThingClassId) {
                ThingDescriptor channelChild(shellyPowerMeterChannelThingClassId, info->thing()->name() + " channel 1", QString(), info->thing()->id());
                channelChild.setParams(ParamList() << Param(shellyPowerMeterChannelThingChannelParamTypeId, 1));
                autoChilds.append(channelChild);
                ThingDescriptor channel2Child(shellyPowerMeterChannelThingClassId, info->thing()->name() + " channel 2", QString(), info->thing()->id());
                channel2Child.setParams(ParamList() << Param(shellyPowerMeterChannelThingChannelParamTypeId, 2));
                autoChilds.append(channel2Child);
            }

            if (info->thing()->thingClassId() == shellyEmThingClassId) {
                ThingDescriptor channelChild(shellyEmChannelThingClassId, info->thing()->name() + " channel 1", QString(), info->thing()->id());
                channelChild.setParams(ParamList() << Param(shellyEmChannelThingChannelParamTypeId, 1));
                autoChilds.append(channelChild);
                ThingDescriptor channel2Child(shellyEmChannelThingClassId, info->thing()->name() + " channel 2", QString(), info->thing()->id());
                channel2Child.setParams(ParamList() << Param(shellyEmChannelThingChannelParamTypeId, 2));
                autoChilds.append(channel2Child);
            }

        }

        info->finish(Thing::ThingErrorNoError);
        info->thing()->setStateValue("connected", true);

        emit autoThingsAppeared(autoChilds);

        // Make sure authentication is enalbed if the user wants it
        QString username = info->thing()->paramValue("username").toString();
        QString password = info->thing()->paramValue("password").toString();
        if (!username.isEmpty()) {
            QUrl url;
            url.setScheme("http");
            url.setHost(address.toString());
            url.setPort(80);
            url.setPath("/settings/login");
            url.setUserName(username);
            url.setPassword(password);

            QUrlQuery query;
            query.addQueryItem("username", username);
            query.addQueryItem("password", password);
            query.addQueryItem("enabled", "true");

            url.setQuery(query);

            QNetworkRequest request(url);
            qCDebug(dcShelly()) << "Enabling auth" << username << password;
            QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        }
    });

    // For testing and debugging, introspect the coap API. Allows introspecting the coap api on the device
//    url.clear();
//    url.setScheme("coap");
//    url.setHost(address.toString());
//    url.setPath("/cit/d");

//    CoapRequest coapRequest(url);
//    CoapReply *coapReply = m_coap->get(coapRequest);
//    qCDebug(dcShelly) << "Coap request" << url;
//    connect(coapReply, &CoapReply::finished, thing, [=](){
//        qCDebug(dcShelly) << "Coap reply" << coapReply->error() << qUtf8Printable(QJsonDocument::fromJson(coapReply->payload()).toJson());
//    });


    // Handle thing settings of gateway devices
    if (info->thing()->thingClassId() == shellyPlugThingClassId ||
            info->thing()->thingClassId() == shellyButton1ThingClassId ||
            info->thing()->thingClassId() == shellyI3ThingClassId ||
            info->thing()->thingClassId() == shellyTrvThingClassId ||
            info->thing()->thingClassId() == shellyGasThingClassId) {
        connect(info->thing(), &Thing::settingChanged, this, [this, thing, shellyId](const ParamTypeId &settingTypeId, const QVariant &value) {

            pluginStorage()->beginGroup(thing->id().toString());
            QString address = pluginStorage()->value("cachedAddress").toString();
            pluginStorage()->endGroup();

            QUrl url;
            url.setScheme("http");
            url.setHost(address);
            url.setPort(80);
            url.setUserName(thing->paramValue("username").toString());
            url.setPassword(thing->paramValue("password").toString());

            QUrlQuery query;
            if (settingTypeId == shellyPlugSettingsDefaultStateParamTypeId) {
                url.setPath("/settings/relay/0");
                query.addQueryItem("default_state", value.toString());
            } else if (settingTypeId == shellyButton1SettingsRemainAwakeParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("remain_awake", value.toString());
            } else if (settingTypeId == shellyButton1SettingsStatusLedEnabledParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("led_status_disable", value.toBool() ? "false" : "true");
            } else if (settingTypeId == shellyI3SettingsLongpushMinDurationParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("longpush_duration_ms_min", value.toString());
            } else if (settingTypeId == shellyButton1SettingsLongpushMaxDurationParamTypeId
                       || settingTypeId == shellyI3SettingsLongpushMaxDurationParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("longpush_duration_ms_max", value.toString());
            } else if (settingTypeId == shellyButton1SettingsMultipushTimeBetweenPushesParamTypeId
                       || settingTypeId == shellyI3SettingsMultipushTimeBetweenPushesParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("multipush_time_between_pushes_ms_max", value.toString());
            } else if (settingTypeId == shellyTrvSettingsChildLockParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("child_lock", value.toString());
            } else if (settingTypeId == shellyTrvSettingsDisplayBrightnessParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("display_brightness", value.toString());
            } else if (settingTypeId == shellyTrvSettingsDisplayFlippedParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("display_flipped", value.toString());
            } else if (settingTypeId == shellyGasSettingsBuzzerVolumeParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("set_volume", value.toString());
            } else if (settingTypeId == shellyFloodSettingsRainSensorParamTypeId) {
                url.setPath("/settings");
                query.addQueryItem("rain_sensor", value.toString());
            }

            url.setQuery(query);

            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            qCDebug(dcShelly()) << "Setting configuration:" << url.toString();
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, [reply](){
                qCDebug(dcShelly) << "Set config reply:" << reply->error() << reply->errorString() << reply->readAll();
            });
        });
    }
}

void IntegrationPluginShelly::setupGen2(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QHostAddress address = getIP(thing);
    QString shellyId = info->thing()->paramValue("id").toString();

    if (address.isNull()) {
        qCWarning(dcShelly()) << "Unable to determine Shelly's network address. Failed to set up device.";
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to find the thing in the network."));
        return;
    }

    pluginStorage()->beginGroup(thing->id().toString());
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();

    ShellyJsonRpcClient *client = new ShellyJsonRpcClient(info->thing());
    client->open(address, "admin", password, shellyId);
    connect(client, &ShellyJsonRpcClient::stateChanged, info, [info, client, this](QAbstractSocket::SocketState state) {
        qCDebug(dcShelly()) << "Websocket state changed:" << state;
        // GetDeviceInfo wouldn't require authentication if enabled, so if the setup is changed to fetch some info from GetDeviceInfo,
        // make sure to not just replace the GetStatus call, or authentication verification won't work any more.
        ShellyRpcReply *reply = client->sendRequest("Shelly.GetStatus");
        connect(reply, &ShellyRpcReply::finished, info, [info, client, this](ShellyRpcReply::Status status, const QVariantMap &response){
            if (status != ShellyRpcReply::StatusSuccess) {
                qCWarning(dcShelly) << "Error during shelly setup";
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }
            qCDebug(dcShelly) << "Init response:" << response;
            m_rpcClients.insert(info->thing(), client);

            if (info->thing()->thingClassId() == shellyPlus1pmThingClassId || info->thing()->thingClassId() == shellyPlus1ThingClassId) {

                info->finish(Thing::ThingErrorNoError);

                if (myThings().filterByParentId(info->thing()->id()).count() == 0) {
                    ThingDescriptor switchChild(shellySwitchThingClassId, info->thing()->name() + " switch", QString(), info->thing()->id());
                    switchChild.setParams(ParamList() << Param(shellySwitchThingChannelParamTypeId, 1));
                    emit autoThingsAppeared({switchChild});
                }
                return;
            }

            if (info->thing()->thingClassId() == shellyPlus25ThingClassId) {
                // Make sure the shelly plus 2PM is in the mode we expect it to be (roller/cover or relay/switch)
                bool rollerMode = info->thing()->paramValue("rollerMode").toBool();
                QVariantMap params;
                if(rollerMode) {
                    params.insert("name", "cover");
                } else {
                    params.insert("name", "switch");
                }
                ShellyRpcReply *reply2 = client->sendRequest("Shelly.SetProfile", params);
                connect(reply2, &ShellyRpcReply::finished, info, [this, info, rollerMode](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
                    if (status != ShellyRpcReply::StatusSuccess) {
                        qCWarning(dcShelly) << "Error during shelly setup";
                        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unable to configure shelly device."));
                        return;
                    }
                    info->finish(Thing::ThingErrorNoError);

                    if (myThings().filterByParentId(info->thing()->id()).count() == 0) {
                        ThingDescriptors children;
                        ThingDescriptor switchChild(shellySwitchThingClassId, info->thing()->name() + " switch 1", QString(), info->thing()->id());
                        switchChild.setParams(ParamList() << Param(shellySwitchThingChannelParamTypeId, 1));
                        children.append(switchChild);
                        ThingDescriptor switch2Child(shellySwitchThingClassId, info->thing()->name() + " switch 2", QString(), info->thing()->id());
                        switch2Child.setParams(ParamList() << Param(shellySwitchThingChannelParamTypeId, 2));
                        children.append(switch2Child);

                        if (rollerMode == true) {
                            ThingDescriptor rollerShutterChild(shellyRollerThingClassId, info->thing()->name() + " connected shutter", QString(), info->thing()->id());
                            rollerShutterChild.setParams(ParamList() << Param(shellyRollerThingChannelParamTypeId, 1));
                            children.append(rollerShutterChild);
                        // Create 2 measurement channels for Shelly Plus 2PM (unless in roller mode)
                        }  else {
                            ThingDescriptor channelChild(shellyPowerMeterChannelThingClassId, info->thing()->name() + " channel 1", QString(), info->thing()->id());
                            channelChild.setParams(ParamList() << Param(shellyPowerMeterChannelThingChannelParamTypeId, 1));
                            children.append(channelChild);
                            ThingDescriptor channel2Child(shellyPowerMeterChannelThingClassId, info->thing()->name() + " channel 2", QString(), info->thing()->id());
                            channel2Child.setParams(ParamList() << Param(shellyPowerMeterChannelThingChannelParamTypeId, 2));
                            children.append(channel2Child);
                        }
                        emit autoThingsAppeared(children);
                    }
                });

                return;
            }

            if (info->thing()->thingClassId() == shellyPlusPlugThingClassId) {
                // Set default state & led mode of the Plus Plug (S)
                QString defaultState = "off";
                QString ledMode = "switch";
                defaultState = info->thing()->setting("defaultState").toString();
                QVariantMap config;
                config.insert("initial_state", defaultState);
                QVariantMap params;
                params.insert("id", 0);
                params.insert("config", config);

                ShellyRpcReply *reply2 = client->sendRequest("Switch.SetConfig", params);
                connect(reply2, &ShellyRpcReply::finished, info, [info](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
                    if (status != ShellyRpcReply::StatusSuccess) {
                        qCWarning(dcShelly) << "Error during shelly setup";
                        info->finish(Thing::ThingErrorHardwareFailure);
                        return;
                    }
                    info->finish(Thing::ThingErrorNoError);
                });

                ledMode = info->thing()->setting("ledMode").toString();
                QVariantMap leds;
                leds.insert("mode", ledMode);
                QVariantMap config2;
                config2.insert("leds", leds);
                QVariantMap params2;
                params2.insert("config", config2);

                ShellyRpcReply *reply3 = client->sendRequest("PLUGS_UI.SetConfig", params2);
                connect(reply3, &ShellyRpcReply::finished, info, [info](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
                    if (status != ShellyRpcReply::StatusSuccess) {
                        qCWarning(dcShelly) << "Error during shelly setup";
                        info->finish(Thing::ThingErrorHardwareFailure);
                        return;
                    }
                });
                return;

            }

            if (info->thing()->thingClassId() == shellyPro3EMThingClassId) {
                info->finish(Thing::ThingErrorNoError);
                return;
            }
        });
    });

    connect(client, &ShellyJsonRpcClient::stateChanged, thing, [thing, client, password, shellyId, this](QAbstractSocket::SocketState state) {
        thing->setStateValue("connected", state == QAbstractSocket::ConnectedState);
        foreach (Thing *child, myThings().filterByParentId(thing->id())) {
            child->setStateValue("connected", state == QAbstractSocket::ConnectedState);
        }

        if (state == QAbstractSocket::UnconnectedState) {
            QTimer::singleShot(1000, thing, [this, client, thing, password, shellyId](){
                client->open(getIP(thing), "admin", password, shellyId);
            });
        } else {
            if (thing->setupStatus() == Thing::ThingSetupStatusComplete) {
                fetchStatusGen2(thing);
            }
        }
    });
    connect(client, &ShellyJsonRpcClient::notificationReceived, thing, [thing, this](const QVariantMap &notification){
        qCDebug(dcShelly) << "notification received" << qUtf8Printable(QJsonDocument::fromVariant(notification).toJson());
        if (notification.contains("switch:0")) {
            QVariantMap switch0 = notification.value("switch:0").toMap();
            if (switch0.contains("apower") && thing->hasState("currentPower")) { // for shellyplus1pm
                thing->setStateValue("currentPower", switch0.value("apower").toDouble());
            }
            Thing *parentThing = myThings().filterByParentId(thing->id()).findByParams({Param(shellyPowerMeterChannelThingChannelParamTypeId, 1)});
            if (parentThing) {
                if (switch0.contains("apower")) {
                    parentThing->setStateValue("currentPower", switch0.value("apower").toDouble());
                }
                if (switch0.contains("aenergy")) {
                    parentThing->setStateValue("totalEnergyConsumed", notification.value("switch:0").toMap().value("aenergy").toMap().value("total").toDouble() / 1000);
                }
            } else {
                if (switch0.contains("aenergy") && thing->hasState("totalEnergyConsumed")) { // for shellyplus1pm
                    thing->setStateValue("totalEnergyConsumed", notification.value("switch:0").toMap().value("aenergy").toMap().value("total").toDouble() / 1000);
                }
            }
            if (switch0.contains("output") && thing->hasState("power")) { // for shellyplus1pm
                thing->setStateValue("power", switch0.value("output").toBool());
            } else if (switch0.contains("output") && thing->hasState("channel1")) { // for shellyplus2pm
                thing->setStateValue("channel1", switch0.value("output").toBool());
            }
        }
        if (notification.contains("switch:1")) {
            QVariantMap switch1 = notification.value("switch:1").toMap();
            Thing *parentThing = myThings().filterByParentId(thing->id()).findByParams({Param(shellyPowerMeterChannelThingChannelParamTypeId, 2)});
            if (parentThing) {
                if (switch1.contains("apower")) {
                    parentThing->setStateValue("currentPower", switch1.value("apower").toDouble());
                }
                if (switch1.contains("aenergy")) {
                    parentThing->setStateValue("totalEnergyConsumed", notification.value("switch:1").toMap().value("aenergy").toMap().value("total").toDouble() / 1000);
                }
            }
            if (switch1.contains("output") && thing->hasState("channel2")) { // for shellyplus2pm
                thing->setStateValue("channel2", switch1.value("output").toBool());
            }
        }
        if (notification.contains("cover:0")) {
            QVariantMap cover0 = notification.value("cover:0").toMap();
            Thing *t = myThings().filterByParentId(thing->id()).findByParams({Param(shellyRollerThingChannelParamTypeId, 1)});
            if (cover0.contains("apower") && t) {
                t->setStateValue("currentPower", cover0.value("apower").toDouble());
            }
            if (cover0.contains("aenergy") && t) {
                t->setStateValue("totalEnergyConsumed", notification.value("cover:0").toMap().value("aenergy").toMap().value("total").toDouble());
            }
            if (cover0.contains("current_pos") && t) {
                t->setStateValue("percentage", notification.value("cover:0").toMap().value("current_pos").toInt());
            }
            if (cover0.contains("state") && t) {
                QString coverState = notification.value("cover:0").toMap().value("state").toString();
                bool movingBool = false;
                if (coverState == "opening" || coverState == "closing" || coverState == "calibrating") {
                    movingBool = true;
                }
                t->setStateValue("moving", movingBool);
            }
            if (cover0.contains("output") && thing->hasState("channel1")) { // for shellyplus2pm
                thing->setStateValue("power", cover0.value("output").toBool());
            }
        }
        if (notification.contains("input:0")) {
            QVariantMap input0 = notification.value("input:0").toMap();
            Thing *t = myThings().filterByParentId(thing->id()).findByParams({Param(shellySwitchThingChannelParamTypeId, 1)});
            if (t) {
                t->setStateValue("power", input0.value("state").toBool());
                t->emitEvent("pressed");
            }
        }
        if (notification.contains("input:1")) {
            QVariantMap input1 = notification.value("input:1").toMap();
            Thing *t = myThings().filterByParentId(thing->id()).findByParams({Param(shellySwitchThingChannelParamTypeId, 2)});
            if (t) {
                t->setStateValue("power", input1.value("state").toBool());
                t->emitEvent("pressed");
            }
        }
        if (notification.contains("em:0")) {
            QVariantMap em0 = notification.value("em:0").toMap();
            thing->setStateValue(shellyPro3EMCurrentPowerPhaseAStateTypeId, em0.value("a_act_power").toDouble());
            thing->setStateValue(shellyPro3EMVoltagePhaseAStateTypeId, em0.value("a_voltage").toDouble());
            thing->setStateValue(shellyPro3EMCurrentPhaseAStateTypeId, em0.value("a_current").toDouble());
            thing->setStateValue(shellyPro3EMCurrentPowerPhaseBStateTypeId, em0.value("b_act_power").toDouble());
            thing->setStateValue(shellyPro3EMVoltagePhaseBStateTypeId, em0.value("b_voltage").toDouble());
            thing->setStateValue(shellyPro3EMCurrentPhaseCStateTypeId, em0.value("c_current").toDouble());
            thing->setStateValue(shellyPro3EMCurrentPowerPhaseCStateTypeId, em0.value("c_act_power").toDouble());
            thing->setStateValue(shellyPro3EMVoltagePhaseCStateTypeId, em0.value("c_voltage").toDouble());
            thing->setStateValue(shellyPro3EMCurrentPhaseCStateTypeId, em0.value("c_current").toDouble());

            thing->setStateValue(shellyPro3EMCurrentPowerStateTypeId, em0.value("total_act_power").toDouble());
        }
        if (notification.contains("emdata:0")) {
            QVariantMap emdata0 = notification.value("emdata:0").toMap();
            thing->setStateValue(shellyPro3EMEnergyConsumedPhaseAStateTypeId, emdata0.value("a_total_act_energy").toDouble() / 1000);
            thing->setStateValue(shellyPro3EMEnergyProducedPhaseAStateTypeId, emdata0.value("a_total_act_ret_energy").toDouble() / 1000);
            thing->setStateValue(shellyPro3EMEnergyConsumedPhaseBStateTypeId, emdata0.value("b_total_act_energy").toDouble() / 1000);
            thing->setStateValue(shellyPro3EMEnergyProducedPhaseBStateTypeId, emdata0.value("b_total_act_ret_energy").toDouble() / 1000);
            thing->setStateValue(shellyPro3EMEnergyConsumedPhaseCStateTypeId, emdata0.value("c_total_act_energy").toDouble() / 1000);
            thing->setStateValue(shellyPro3EMEnergyProducedPhaseCStateTypeId, emdata0.value("c_total_act_ret_energy").toDouble() / 1000);
            thing->setStateValue(shellyPro3EMTotalEnergyConsumedStateTypeId, emdata0.value("total_act").toDouble() / 1000);
            thing->setStateValue(shellyPro3EMTotalEnergyProducedStateTypeId, emdata0.value("total_act_ret").toDouble() / 1000);
        }
    });

    // Handle thing settings of devices
    if (info->thing()->thingClassId() == shellyPlusPlugThingClassId) {
        connect(info->thing(), &Thing::settingChanged, this, [thing, client, shellyId](const ParamTypeId &settingTypeId, const QVariant &value) {
            if (settingTypeId == shellyPlusPlugSettingsDefaultStateParamTypeId) { // this works
                QString defaultState = value.toString();
                QVariantMap config;
                config.insert("initial_state", defaultState);
                QVariantMap params;
                params.insert("id", 0);
                params.insert("config", config);

                ShellyRpcReply *reply3 = client->sendRequest("Switch.SetConfig", params);
                connect(reply3, &ShellyRpcReply::finished, thing, [](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
                    if (status != ShellyRpcReply::StatusSuccess) {
                        qCWarning(dcShelly) << "Error setting new value";
                        return;
                    }
                });
            };
            if (settingTypeId == shellyPlusPlugSettingsLedModeParamTypeId) { // this gives a segmentation fault
                QString ledMode = value.toString();
                QVariantMap leds;
                leds.insert("mode", ledMode);
                QVariantMap config;
                config.insert("leds", leds);
                QVariantMap params;
                params.insert("config", config);

                ShellyRpcReply *reply3 = client->sendRequest("PLUGS_UI.SetConfig", params);
                connect(reply3, &ShellyRpcReply::finished, thing, [](ShellyRpcReply::Status status, const QVariantMap &/*response*/){
                    if (status != ShellyRpcReply::StatusSuccess) {
                        qCWarning(dcShelly) << "Error setting LED mode";
                        return;
                    }
                });
            }

        });
    }

}

void IntegrationPluginShelly::setupShellyChild(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcShelly()) << "Setting up shelly child:" << info->thing()->name();

    Thing *parent = myThings().findById(thing->parentId());
    Q_ASSERT_X(parent != nullptr, "Shelly::setupChild", "Child has no parent!");
    if (!parent->setupComplete()) {
        qCDebug(dcShelly()) << "Parent for" << info->thing()->name() << "is not set up yet... Waiting...";
        // If the parent isn't set up yet, wait for it...
        connect(parent, &Thing::setupStatusChanged, info, [=](){
            qCDebug(dcShelly()) << "Setup for" << parent->name() << "Completed. Continuing with setup of child" << info->thing()->name();
            if (parent->setupStatus() == Thing::ThingSetupStatusComplete) {
                setupShellyChild(info);
            } else if (parent->setupStatus() == Thing::ThingSetupStatusFailed) {
                info->finish(Thing::ThingErrorHardwareFailure);
            }
        });
        return;
    }

    qCDebug(dcShelly()) << "Parent for" << info->thing()->name() << "is set up. Finishing child setup.";

    // Connect to settings changes to store them to the thing
    connect(info->thing(), &Thing::settingChanged, this, [this, thing, parent](const ParamTypeId &paramTypeId, const QVariant &value){
        if (parent->paramValue("id").toString().contains("Plus")) {
            ShellyJsonRpcClient *client = m_rpcClients.value(parent);
            QVariantMap params;
            params.insert("id", thing->paramValue("channel").toInt() - 1);

            if (paramTypeId == shellySwitchSettingsButtonTypeParamTypeId) {
                QVariantMap inputConfig;
                if (value == "toggle" || value == "edge") {
                    inputConfig.insert("type", "switch");
                } else {
                    inputConfig.insert("type", "button");
                }
                params["config"] = inputConfig;
                client->sendRequest("Input.SetConfig", params);

                QVariantMap switchConfig;
                switchConfig.insert("in_mode", value.toString().replace("toggle", "follow").replace("edge", "flip"));
                params["config"] = switchConfig;
                client->sendRequest("Switch.SetConfig", params);

            } else if (paramTypeId == shellySwitchSettingsInvertButtonParamTypeId) {
                QVariantMap config;
                config.insert("invert", value.toBool());
                params.insert("config", config);
                client->sendRequest("Input.SetConfig", params);
            }
        } else {
            pluginStorage()->beginGroup(parent->id().toString());
            QString address = pluginStorage()->value("cachedAddress").toString();
            pluginStorage()->endGroup();

            QUrl url;
            url.setScheme("http");
            url.setHost(address);
            url.setPort(80);
            url.setPath(QString("/settings/relay/%0").arg(thing->paramValue("channel").toInt() - 1));
            url.setUserName(parent->paramValue("username").toString());
            url.setPassword(parent->paramValue("password").toString());

            QUrlQuery query;
            if (paramTypeId == shellySwitchSettingsButtonTypeParamTypeId) {
                query.addQueryItem("btn_type", value.toString());
            }
            if (paramTypeId == shellySwitchSettingsInvertButtonParamTypeId) {
                query.addQueryItem("btn_reverse", value.toBool() ? "1" : "0");
            }

            url.setQuery(query);

            qCDebug(dcShelly) << "Setting configuration:" << url.toString();

            QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, [reply](){
                qCDebug(dcShelly) << "Set config reply:" << reply->error() << reply->errorString() << reply->readAll();
            });
        }
    });

    info->finish(Thing::ThingErrorNoError);
}

QHostAddress IntegrationPluginShelly::getIP(Thing *thing) const
{
    Thing *d = thing;
    if (!thing->parentId().isNull()) {
        d = myThings().findById(thing->parentId());
    }

    QString shellyId = d->paramValue("id").toString();

    ZeroConfServiceEntry zeroConfEntry;
    foreach (const ZeroConfServiceEntry &entry, m_zeroconfBrowser->serviceEntries()) {
        if (entry.name() == shellyId) {
            zeroConfEntry = entry;
        }
    }
    QHostAddress address;
    pluginStorage()->beginGroup(d->id().toString());
    if (zeroConfEntry.isValid()) {
        qCDebug(dcShelly()) << "Shelly device found on mDNS. Using" << zeroConfEntry.hostAddress() << "and caching it.";
        address = zeroConfEntry.hostAddress();
        pluginStorage()->setValue("cachedAddress", address.toString());
    } else if (pluginStorage()->contains("cachedAddress")){
        address = QHostAddress(pluginStorage()->value("cachedAddress").toString());
        qCDebug(dcShelly()) << "Could not find Shelly thing on mDNS. Trying cached address:" << address;
    } else {
        qCWarning(dcShelly()) << "Unable to determine IP address of shelly device:" << shellyId;
    }
    pluginStorage()->endGroup();

    return address;
}

bool IntegrationPluginShelly::isGen2(const QString &shellyId) const
{
    return shellyId.contains("Plus")
                || shellyId.contains("Pro")
                || shellyId.startsWith("ShellyPlug") // Plus plug variants don't have Plus in the name, but are camelcased as opposed to 1st gen plugs
                ;
}

void IntegrationPluginShelly::handleInputEvent(Thing *thing, const QString &buttonName, const QString &inputEventString, int inputEventCount)
{
    pluginStorage()->beginGroup(thing->id().toString());
    pluginStorage()->beginGroup(buttonName);
    int oldInputCount = pluginStorage()->value("inputCount", 0).toInt();
    pluginStorage()->setValue("inputCount", inputEventCount);
    pluginStorage()->endGroup();
    pluginStorage()->endGroup();

    if (oldInputCount == inputEventCount) {
        return; // already known.
    }

    ParamTypeId pressedButtonNameParamTypeId = thing->thingClass().eventTypes().findByName("pressed").paramTypes().findByName("buttonName").id();
    ParamTypeId longPressedButtonNameParamTypeId = thing->thingClass().eventTypes().findByName("longPressed").paramTypes().findByName("buttonName").id();
    ParamTypeId pressedCountParamTypeId = thing->thingClass().eventTypes().findByName("pressed").paramTypes().findByName("count").id();

    if (inputEventString == "S") {
        thing->emitEvent("pressed", ParamList() << Param(pressedButtonNameParamTypeId, buttonName) << Param(pressedCountParamTypeId, 1));
    } else if (inputEventString == "L") {
        thing->emitEvent("longPressed", ParamList() << Param(longPressedButtonNameParamTypeId, buttonName));
    } else if (inputEventString == "SS") {
        thing->emitEvent("pressed", ParamList() << Param(pressedButtonNameParamTypeId, buttonName) << Param(pressedCountParamTypeId, 2));
    } else if (inputEventString == "SSS") {
        thing->emitEvent("pressed", ParamList() << Param(pressedButtonNameParamTypeId, buttonName) << Param(pressedCountParamTypeId, 3));
    } else if (inputEventString == "SL") {
        thing->emitEvent("pressed", ParamList() << Param(pressedButtonNameParamTypeId, buttonName) << Param(pressedCountParamTypeId, 1));
        thing->emitEvent("longPressed", ParamList() << Param(longPressedButtonNameParamTypeId, buttonName));
    } else if (inputEventString == "LS") {
        thing->emitEvent("longPressed", ParamList() << Param(longPressedButtonNameParamTypeId, buttonName));
        thing->emitEvent("pressed", ParamList() << Param(pressedButtonNameParamTypeId, buttonName) << Param(pressedCountParamTypeId, 1));
    } else {
        qCDebug(dcShelly()) << "Invalid button code from shelly" << thing->name() << inputEventString;
    }
}

QNetworkRequest IntegrationPluginShelly::createHttpRequest(Thing *thing, const QString &path, const QUrlQuery &urlQuery)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(getIP(thing).toString());
    url.setPort(80);
    url.setPath(path);
    if (!thing->paramValue("username").toString().isEmpty()) {
        url.setUserName(thing->paramValue("username").toString());
        url.setPassword(thing->paramValue("password").toString());
    }
    url.setQuery(urlQuery);
    return QNetworkRequest(url);
}

QVariantMap IntegrationPluginShelly::createRpcRequest(const QString &method)
{
    QVariantMap map;
    map.insert("src", "nymea");
    map.insert("id", 1);
    map.insert("method", method);
    return map;
}
