/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2024, nymea GmbH
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

#include "integrationpluginlogilink.h"
#include "plugininfo.h"
#include "plugintimer.h"

#include <network/networkaccessmanager.h>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QUrlQuery>
#include <QXmlStreamReader>

IntegrationPluginLogilink::IntegrationPluginLogilink()
{
}

IntegrationPluginLogilink::~IntegrationPluginLogilink()
{
    m_pollTimer->deleteLater();
}

void IntegrationPluginLogilink::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter the login credentials for your device."));
}

void IntegrationPluginLogilink::init()
{
    m_pollTimer = hardwareManager()->pluginTimerManager()->registerTimer(m_pollInterval);
    connect(m_pollTimer, &PluginTimer::timeout, this, &IntegrationPluginLogilink::refreshStates);
}

void IntegrationPluginLogilink::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{
    if (info->thingClassId() == pdu8p01ThingClassId) {
        QString ipAddress = info->params().paramValue(pdu8p01ThingIpv4AddressParamTypeId).toString();

        QNetworkRequest request;
        request.setUrl(QUrl(QString("http://%1/status.xml").arg(ipAddress)));
        request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username).arg(password).toUtf8().toBase64());
        qCDebug(dcLogilink()) << "ConfirmPairing fetching:" << request.url() << request.rawHeader("Authorization");
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [=](){
            if (reply->error() == QNetworkReply::NoError) {
                pluginStorage()->beginGroup(info->thingId().toString());
                pluginStorage()->setValue("username", username);
                pluginStorage()->setValue("password", password);
                pluginStorage()->endGroup();
                info->finish(Thing::ThingErrorNoError);
            } else {
                info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Wrong username or password."));
            }
        });
    } else {
        qCWarning(dcLogilink()) << "Unhandled ThingClass in confirmPairing" << info->thingClassId();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginLogilink::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcLogilink()) << "Setup" << thing->name();

    if (thing->thingClassId() == pdu8p01ThingClassId) {
        if (!m_pollTimer->running()) {
            m_pollTimer->start();
        }

        QString ipAddress = thing->paramValue(pdu8p01ThingIpv4AddressParamTypeId).toString();

        QNetworkRequest request;
        pluginStorage()->beginGroup(thing->id().toString());
        const QString username = pluginStorage()->value("username").toString();
        const QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();

        request.setUrl(QUrl(QString("http://%1/status.xml").arg(ipAddress)));
        request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username).arg(password).toUtf8().toBase64());
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [=](){
            if (reply->error() == QNetworkReply::NoError) {
                info->finish(Thing::ThingErrorNoError);

                if (myThings().filterByParentId(thing->id()).isEmpty()) {
                    // Creating sockets as child 'things'
                    ThingDescriptors descriptorList;
                    for (int i = 0; i < 8; i++) {
                        QString deviceName = thing->name() + " socket " + QString::number(i);
                        ThingDescriptor d(socketThingClassId, deviceName, thing->name(), thing->id());
                        d.setParams(ParamList() << Param(socketThingNumberParamTypeId, i));
                        descriptorList << d;
                    }
                    emit autoThingsAppeared(descriptorList);
                }
            } else if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
                info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Wrong username or password"));
            } else {
                info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Device not found"));
            }
        });

        return;
    }

    if (thing->thingClassId() == socketThingClassId) {
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    qCWarning(dcLogilink()) << "Unhandled ThingClass in setupDevice" << thing->thingClassId();
    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginLogilink::postSetupThing(Thing *thing)
{
    qCDebug(dcLogilink()) << "Post setup" << thing->name();
    if (thing->thingClassId() == pdu8p01ThingClassId) {
        getStates(thing);
    }
}

void IntegrationPluginLogilink::thingRemoved(Thing *thing)
{
    qCDebug(dcLogilink()) << "Thing removed" << thing->name();
    if (thing->thingClassId() == pdu8p01ThingClassId) {
        pluginStorage()->remove(thing->id().toString());
    }

    if (myThings().filterByThingClassId(pdu8p01ThingClassId).isEmpty()) {
        m_pollTimer->stop();
    }
}

void IntegrationPluginLogilink::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == socketThingClassId) {
        if (action.actionTypeId() == socketPowerActionTypeId) {

            Thing *parentDevice = myThings().findById(thing->parentId());
            auto ipAddress = parentDevice->paramValue(pdu8p01ThingIpv4AddressParamTypeId).toString();
            pluginStorage()->beginGroup(parentDevice->id().toString());
            QString username = pluginStorage()->value("username").toString();
            QString password = pluginStorage()->value("password").toString();
            pluginStorage()->endGroup();

            QUrl url(QString("http://%1/control_outlet.htm").arg(ipAddress));
            QUrlQuery query;
            query.addQueryItem("outlet" + thing->paramValue(socketThingNumberParamTypeId).toString(), "1");
            query.addQueryItem("op", action.param(socketPowerActionPowerParamTypeId).value().toBool() ? "0" : "1"); // op code 0 is on, 1 is off
            query.addQueryItem("submit", "Apply");
            url.setQuery(query);
            QNetworkRequest request(url);
            request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username, password).toUtf8().toBase64());

            QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
            qCDebug(dcLogilink()) << "Requesting:" << url.toString() << request.rawHeader("Authorization");
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [reply, info](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcLogilink()) << "Execute action failed:" << reply->error() << reply->errorString();
                    info->finish(Thing::ThingErrorHardwareFailure);
                    return;
                }
                info->finish(Thing::ThingErrorNoError);
            });
            return;
        }
        info->finish(Thing::ThingErrorActionTypeNotFound);
    }
    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginLogilink::refreshStates()
{
    foreach (Thing *thing, myThings().filterByThingClassId(pdu8p01ThingClassId)) {
        getStates(thing);
    }
}

void IntegrationPluginLogilink::getStates(Thing *thing)
{
    auto ipAddress = thing->paramValue(pdu8p01ThingIpv4AddressParamTypeId).toString();
    pluginStorage()->beginGroup(thing->id().toString());
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();

    QUrl url(QString("http://%1/status.xml").arg(ipAddress));

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username, password).toUtf8().toBase64());
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [this, thing, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcLogilink()) << "Error fetching stats for" << thing->name() << reply->errorString();
            thing->setStateValue(pdu8p01ConnectedStateTypeId, false);
            foreach (auto child, myThings().filterByParentId(thing->id())) {
                child->setStateValue(socketConnectedStateTypeId, false);
            }
            return;
        }
        QXmlStreamReader xml;
        xml.addData(reply->readAll());
        if (xml.hasError()) {
            qCDebug(dcLogilink()) << "XML Error:" << xml.errorString();
            return;
        }
        thing->setStateValue(pdu8p01ConnectedStateTypeId, true);
        if (xml.readNextStartElement()) {
            if (xml.name() == "response") {
                qCDebug(dcLogilink()) << "XML contains response";
            } else {
                qCWarning(dcLogilink()) << "xml name" << xml.name();
            }
            while(xml.readNextStartElement()) {
                qCDebug(dcLogilink()) << "XML name" << xml.name();
                if (xml.name() == "curBan") {
                    auto current = xml.readElementText().toDouble();
                    qCDebug(dcLogilink()) << "Current" << current;
                    thing->setStateValue(pdu8p01TotalLoadStateTypeId, current);
                } else if (xml.name() == "statBan") {
                    auto status = xml.readElementText();
                    qCDebug(dcLogilink()) << "Status" << status;
                    thing->setStateValue(pdu8p01StatusStateTypeId, status);
                } else if (xml.name() == "tempBan") {
                    auto temperature = xml.readElementText().toDouble();
                    qCDebug(dcLogilink()) << "Temperature" << temperature;
                    thing->setStateValue(pdu8p01TemperatureStateTypeId, temperature);
                } else if (xml.name() == "humBan") {
                    auto humidity = xml.readElementText().toDouble();
                    qCDebug(dcLogilink()) << "hummidity" << humidity;
                    thing->setStateValue(pdu8p01HumidityStateTypeId, humidity);
                } else if (xml.name().startsWith("outletStat")){
                    int socketNumber = xml.name().right(1).toInt();
                    bool socketValue = xml.readElementText().startsWith("on");
                    auto socketThing = myThings().filterByParentId(thing->id())
                            .filterByParam(socketThingNumberParamTypeId, socketNumber)
                            .first();
                    if (!socketThing) {
                        // Socket not yet setup
                        continue;
                    }
                    qCDebug(dcLogilink()) << "Socket" << socketNumber << socketValue;
                    socketThing->setStateValue(socketPowerStateTypeId, socketValue);
                    socketThing->setStateValue(socketConnectedStateTypeId, true);
                } else {
                    xml.skipCurrentElement();
                }
            }
        }
    });
}
