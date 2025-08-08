/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#include "integrationplugindynatrace.h"
#include "plugininfo.h"

#include <network/networkaccessmanager.h>

#include <QDebug>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QHostInfo>
#include <QMetaObject>

IntegrationPluginDynatrace::IntegrationPluginDynatrace()
{
}

void IntegrationPluginDynatrace::discoverThings(ThingDiscoveryInfo *info)
{
    // NOTE: QHostInfo::lookupHost will call in from another thread using the Funtor syntax!
    // https://bugreports.qt.io/browse/QTBUG-83073
    // Using the old school syntax...
    int id = QHostInfo::lookupHost("ufo.home", this, SLOT(resolveIds(const QHostInfo &)));
    m_asyncDiscoveries.insert(id, info);
}

void IntegrationPluginDynatrace::resolveIds(const QHostInfo &host)
{
    int id = host.lookupId();

    if (!m_asyncDiscoveries.contains(id)) {
        qCWarning(dcDynatrace()) << "Discvery result came in but request has vanished...";
        return;
    }

    ThingDiscoveryInfo *info = m_asyncDiscoveries.take(id);
    if (host.error() != QHostInfo::NoError) {
        qCDebug(dcDynatrace()) << "Lookup failed:" << host.errorString();
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An error happened discovering the UFO in the network."));
        return;
    }

    QList<QNetworkReply*> *pendingInfoRequests = new QList<QNetworkReply*>();

    foreach (QHostAddress address, host.addresses()) {
        qCDebug(dcDynatrace()) << "Found IP address" << address.toString();

        QNetworkRequest infoRequest("http://" + address.toString() + "/info");
        QNetworkReply *reply = hardwareManager()->networkManager()->get(infoRequest);

        pendingInfoRequests->append(reply);

        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [this, info, reply, address, pendingInfoRequests](){
            pendingInfoRequests->removeAll(reply);

            QJsonParseError error;
            QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
            if (error.error == QJsonParseError::NoError) {
                QString id = data.toVariant().toMap().value("ufoid").toString();

                ThingDescriptor descriptor(ufoThingClassId, "UFO", address.toString());
                ParamList params;
                params << Param(ufoThingIdParamTypeId, id);
                params << Param(ufoThingHostParamTypeId, address.toString());
                descriptor.setParams(params);

                Things existingUfos = myThings().filterByParam(ufoThingIdParamTypeId, id);
                if (!existingUfos.isEmpty()) {
                    descriptor.setThingId(existingUfos.first()->id());
                }

                info->addThingDescriptor(descriptor);
            }

            if (pendingInfoRequests->isEmpty()) {
                delete pendingInfoRequests;
                info->finish(Thing::ThingErrorNoError);
            }

        });
    }

    // In case we abort, make sure to clean up stuff
    connect(info, &ThingDiscoveryInfo::aborted, this, [pendingInfoRequests](){
        delete pendingInfoRequests;
    });
}

void IntegrationPluginDynatrace::setupThing(ThingSetupInfo *info)
{
    if (info->thing()->thingClassId() == ufoThingClassId) {
        QHostAddress address = QHostAddress(info->thing()->paramValue(ufoThingHostParamTypeId).toString());
        QString id = info->thing()->paramValue(ufoThingIdParamTypeId).toString();
        if(id.isEmpty()) { //Probably a manual Thing setup
            QUrl url;
            url.setScheme("http");
            url.setHost(address.toString());
            url.setPath("/info", QUrl::ParsingMode::TolerantMode);
            QNetworkRequest request;
            request.setUrl(url);
            QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
            connect(reply, &QNetworkReply::finished, this, [info, reply] {
                reply->deleteLater();

                QJsonParseError error;
                QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
                if (error.error != QJsonParseError::NoError) {
                    info->finish(Thing::ThingErrorSetupFailed, error.errorString());
                }
                QString id = data.toVariant().toMap().value("ufoid").toString();
                info->thing()->setParamValue(ufoThingIdParamTypeId, id);
                info->finish(Thing::ThingErrorNoError);
            });
        } else {
            // Discovery Thing setup or Things setup caused by nymea restart
            info->finish(Thing::ThingErrorNoError);
        }
    } else {
        qCWarning(dcDynatrace()) << "Setup thing: Unhandled ThingClassId" << info->thing()->thingClassId();
        info->finish(Thing::ThingErrorSetupFailed);
    }
}

void IntegrationPluginDynatrace::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == ufoThingClassId) {
        thing->setStateValue(ufoConnectedStateTypeId, true);
        thing->setStateValue(ufoPowerStateTypeId, false);
        thing->setStateValue(ufoLogoStateTypeId, false);
        thing->setStateValue(ufoEffectTopStateTypeId, "None");
        thing->setStateValue(ufoEffectBottomStateTypeId, "None");

        QHostAddress address = QHostAddress(thing->paramValue(ufoThingHostParamTypeId).toString());
        Ufo *ufo = new Ufo(hardwareManager()->networkManager(), address, this);
        connect(ufo, &Ufo::connectionChanged, this, &IntegrationPluginDynatrace::onConnectionChanged);
        m_ufoConnections.insert(thing->id(), ufo);
        // Set all off
        ufo->setLogo(QColor(Qt::black), QColor(Qt::black), QColor(Qt::black), QColor(Qt::black));
        ufo->setBackgroundColor(true, true, true, true, QColor(Qt::black));
    }

    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this]() {
            foreach (Ufo *ufo, m_ufoConnections.values()) {
                ufo->getId();
            }
        });
    }
}

void IntegrationPluginDynatrace::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == ufoThingClassId) {
        Ufo *ufo = m_ufoConnections.value(thing->id());
        if (!ufo)
            return;
        if (action.actionTypeId() == ufoLogoActionTypeId) {
            bool power = action.param(ufoLogoActionLogoParamTypeId).value().toBool();
            thing->setStateValue(ufoLogoStateTypeId, power);
            if (power) {
                int brightness = thing->stateValue(ufoBrightnessActionBrightnessParamTypeId).toInt();
                QColor color = QColor(thing->stateValue(ufoLogoColorStateTypeId).toString());
                color.setHsv(color.hue(), color.saturation(), brightness*2.55);
                ufo->setLogo(color, color, color, color);
            } else {
                ufo->setLogo(QColor(Qt::black), QColor(Qt::black), QColor(Qt::black), QColor(Qt::black));
            }
            info->finish(Thing::ThingErrorNoError);

        } else if (action.actionTypeId() == ufoPowerActionTypeId) {
            bool power = action.param(ufoPowerActionPowerParamTypeId).value().toBool();
            thing->setStateValue(ufoPowerStateTypeId, power);
            if (power) {
                int brightness = thing->stateValue(ufoBrightnessActionBrightnessParamTypeId).toInt();
                QColor color = QColor(thing->stateValue(ufoColorStateTypeId).toString());
                color.setHsv(color.hue(), color.saturation(), brightness*2.55);
                thing->setStateValue(ufoLogoStateTypeId, true);
                ufo->setLogo(color, color, color, color);
                ufo->setBackgroundColor(true, true, true, true, color);
                thing->setStateValue(ufoEffectTopStateTypeId, "None");
                thing->setStateValue(ufoEffectBottomStateTypeId, "None");
                thing->setStateValue(ufoLogoColorStateTypeId, color);
                thing->setStateValue(ufoTopColorStateTypeId, color);
                thing->setStateValue(ufoBottomColorStateTypeId, color);
            } else {
                ufo->setLogo(QColor(Qt::black), QColor(Qt::black), QColor(Qt::black), QColor(Qt::black));
                thing->setStateValue(ufoLogoStateTypeId, false);
                ufo->setBackgroundColor(true, true, true, true, QColor(Qt::black));
                thing->setStateValue(ufoEffectTopStateTypeId, "None");
                thing->setStateValue(ufoEffectBottomStateTypeId, "None");
            }
            info->finish(Thing::ThingErrorNoError);

        } else if (action.actionTypeId() == ufoBrightnessActionTypeId) {
            int brightness = action.param(ufoBrightnessActionBrightnessParamTypeId).value().toInt();
            thing->setStateValue(ufoBrightnessStateTypeId, brightness);
            QColor color = QColor(thing->stateValue(ufoColorStateTypeId).toString());
            color.setHsv(color.hue(), color.saturation(), brightness*2.55);
            if (thing->stateValue(ufoLogoStateTypeId).toBool()) {
                ufo->setLogo(color, color, color, color);
            }
            ufo->setBackgroundColor(true, false, true, false, color);
            info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == ufoColorActionTypeId) {
            QColor color = QColor(action.param(ufoColorActionColorParamTypeId).value().toString());
            int brightness = thing->stateValue(ufoBrightnessStateTypeId).toInt();
            thing->setStateValue(ufoColorStateTypeId, color);
            thing->setStateValue(ufoLogoColorStateTypeId, color);
            thing->setStateValue(ufoTopColorStateTypeId, color);
            thing->setStateValue(ufoBottomColorStateTypeId, color);
            color.setHsv(color.hue(), color.saturation(), brightness*2.55);
            if (thing->stateValue(ufoLogoStateTypeId).toBool()) {
                ufo->setLogo(color, color, color, color);
            }
            ufo->setBackgroundColor(true, false, true, false, color);
            info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == ufoColorTemperatureActionTypeId) {
            int mired= thing->stateValue(ufoColorTemperatureActionColorTemperatureParamTypeId).toInt();
            thing->setStateValue(ufoColorTemperatureStateTypeId, mired);
            int brightness = thing->stateValue(ufoBrightnessActionBrightnessParamTypeId).toInt();
            QColor color(Qt::white);
            color.setBlue(static_cast<int>((mired-153)*0.73));
            color.setHsv(color.hue(), color.saturation(), brightness*2.55);
            if (thing->stateValue(ufoLogoStateTypeId).toBool()) {
                ufo->setLogo(color, color, color, color);
            }
            thing->setStateValue(ufoColorStateTypeId, color);
            thing->setStateValue(ufoLogoColorStateTypeId, color);
            thing->setStateValue(ufoTopColorStateTypeId, color);
            thing->setStateValue(ufoBottomColorStateTypeId, color);
            ufo->setBackgroundColor(true, false, true, false, color);
            info->finish(Thing::ThingErrorNoError);

        } else if (action.actionTypeId() == ufoEffectTopActionTypeId) {
            QString effect = action.param(ufoEffectTopActionEffectTopParamTypeId).value().toString();
            int brightness = thing->stateValue(ufoBrightnessActionBrightnessParamTypeId).toInt();
            QColor color = QColor(thing->stateValue(ufoColorStateTypeId).toString());
            color.setHsv(color.hue(), color.saturation(), brightness*2.55);
            if (effect == "None") {
                ufo->setBackgroundColor(true, true, false, false, color);
            } else if (effect == "Whirl") {
                ufo->startWhirl(true, false, color, 500, true);
            } else if (effect == "Morph") {
                ufo->startMorph(true, false, color, 250, 8);
            }
            info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == ufoEffectBottomActionTypeId) {
            QString effect = action.param(ufoEffectBottomActionEffectBottomParamTypeId).value().toString();
            int brightness = thing->stateValue(ufoBrightnessActionBrightnessParamTypeId).toInt();
            QColor color = QColor(thing->stateValue(ufoColorStateTypeId).toString());
            color.setHsv(color.hue(), color.saturation(), brightness*2.55);
            if (effect == "None") {
                ufo->setBackgroundColor(false, false, true, true, color);
            } else if (effect == "Whirl") {
                ufo->startWhirl(false, true, color, 500, true);
            } else if (effect == "Morph") {
                ufo->startMorph(false, true, color, 250, 8);
            }
            info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == ufoLogoColorActionTypeId) {
            QColor color = QColor(action.param(ufoLogoColorActionLogoColorParamTypeId).value().toString());
            int brightness = thing->stateValue(ufoBrightnessStateTypeId).toInt();
            thing->setStateValue(ufoLogoColorStateTypeId, color);
            thing->setStateValue(ufoLogoStateTypeId, true);
            color.setHsv(color.hue(), color.saturation(), brightness*2.55);
            ufo->setLogo(color, color, color, color);
            info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == ufoTopColorActionTypeId) {
            QColor color = QColor(action.param(ufoTopColorActionTopColorParamTypeId).value().toString());
            int brightness = thing->stateValue(ufoBrightnessStateTypeId).toInt();
            thing->setStateValue(ufoTopColorStateTypeId, color);
            thing->setStateValue(ufoPowerStateTypeId, true);
            color.setHsv(color.hue(), color.saturation(), brightness*2.55);
            ufo->setBackgroundColor(true, false, false, false, color);
            info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == ufoBottomColorActionTypeId) {
            QColor color = QColor(action.param(ufoBottomColorActionBottomColorParamTypeId).value().toString());
            int brightness = thing->stateValue(ufoBrightnessStateTypeId).toInt();
            thing->setStateValue(ufoBottomColorStateTypeId, color);
            thing->setStateValue(ufoPowerStateTypeId, true);
            color.setHsv(color.hue(), color.saturation(), brightness*2.55);
            ufo->setBackgroundColor(false, false, true, false, color);
            info->finish(Thing::ThingErrorNoError);
        } else {
            qCWarning(dcDynatrace()) << "Execute action: Unhandled actionTypeId";
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    } else {
        qCWarning(dcDynatrace()) << "Execute action: Unhandled ThingClass";
        info->finish(Thing::ThingErrorHardwareFailure);
    }
}

void IntegrationPluginDynatrace::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == ufoThingClassId) {
        if (m_ufoConnections.contains(thing->id())){
            Ufo *ufo = m_ufoConnections.take(thing->id());
            ufo->deleteLater();
        }
    }

    if (myThings().isEmpty() && m_pluginTimer) {
        m_pluginTimer->deleteLater();
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginDynatrace::getId(const QHostAddress &address)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());
    url.setPath("/info", QUrl::ParsingMode::TolerantMode);
    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError)
            return;

        QString id = data.toVariant().toMap().value("ufoid").toString();
        if (m_asyncSetup.contains(reply->url().host())) {
            ThingSetupInfo *info = m_asyncSetup.value(reply->url().host());
            info->finish(Thing::ThingErrorNoError);
        }
    });
}

void IntegrationPluginDynatrace::onConnectionChanged(bool connected)
{
    Ufo *ufo = static_cast<Ufo *>(sender());
    Thing *thing = myThings().findById(m_ufoConnections.key(ufo));
    if (!thing)
        return;

    thing->setStateValue(ufoConnectedStateTypeId, connected);
}
