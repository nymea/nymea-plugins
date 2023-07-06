/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2023, nymea GmbH
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


#include "integrationplugineasee.h"
#include "plugininfo.h"
#include "signalrconnection.h"

#include <network/networkaccessmanager.h>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QWebSocket>

QString apiEndpoint = "https://api.easee.com/api";
QString streamEndpoint = "http://streams.easee.com/hubs/chargers";

IntegrationPluginEasee::IntegrationPluginEasee()
{

}

IntegrationPluginEasee::~IntegrationPluginEasee()
{
}

void IntegrationPluginEasee::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    QNetworkRequest request(QUrl(QString("%1/%2").arg(apiEndpoint).arg("accounts/login")));
    request.setRawHeader("accept", "application/json");
    request.setRawHeader("content-type", "application/*+json");
    QVariantMap body;
    body.insert("userName", username);
    body.insert("password", secret);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, info, reply, username, secret](){
        qCDebug(dcEasee) << "auth reply finished" << reply->error();

        if (reply->error() == QNetworkReply::ProtocolInvalidOperationError) {
            qCWarning(dcEasee) << "Authentication failed. Looks like a wrong password";
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Authentication failed. Please try again."));
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcEasee) << "Unable to connect to the Easee server";
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unable to contact the easee server. Please try again later."));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcEasee) << "Unable to parse json:" << error.errorString() << data;
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unable to process the response from easee. Please try again later."));
            return;
        }

        QVariantMap map = jsonDoc.toVariant().toMap();
        QByteArray accessToken = map.value("accessToken").toByteArray();
        int expiresIn = map.value("expiresIn").toInt();
        QByteArray refreshToken = map.value("refreshToken").toByteArray();

        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("accessToken", accessToken);
        pluginStorage()->setValue("expiry", QDateTime::currentDateTime().addSecs(expiresIn));
        pluginStorage()->setValue("refreshToken", refreshToken);

        // FIXME: the refresh_token api call seems to not work... So we'll store user/pass in the config for now
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", secret);

        pluginStorage()->endGroup();

        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginEasee::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == accountThingClassId) {
        pluginStorage()->beginGroup(info->thing()->id().toString());
        QByteArray accessToken = pluginStorage()->value("accessToken").toByteArray();
        QByteArray refreshToken = pluginStorage()->value("refreshToken").toByteArray();
        QDateTime expiry = pluginStorage()->value("expiry").toDateTime();
        pluginStorage()->endGroup();

        if (expiry < QDateTime::currentDateTime()) {
            QNetworkReply *reply = this->refreshToken(thing);
            connect(reply, &QNetworkReply::finished, info, [=](){
                setupThing(info);
            });
            return;
        }

        QNetworkRequest request = createRequest(thing, "accounts/profile");
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, thing, [this, thing, reply](){
            qCDebug(dcEasee) << "profile info finished" << reply->error();
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcEasee) << "Unable to contact easee server...";
                thing->setStateValue(accountConnectedStateTypeId, false);
                thing->setStateValue(accountLoggedInStateTypeId, false);
                return;
            }

            QByteArray data = reply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcEasee) << "Unable to parse json:" << error.errorString() << data;
                thing->setStateValue(accountConnectedStateTypeId, false);
                thing->setStateValue(accountLoggedInStateTypeId, false);
                return;
            }

            thing->setStateValue(accountConnectedStateTypeId, true);
            thing->setStateValue(accountLoggedInStateTypeId, true);

            QVariantMap map = jsonDoc.toVariant().toMap();
            qCDebug(dcEasee) << "Profile reply:" << data;

            refreshProducts(thing);

        });
    }

    if (thing->thingClassId() == chargerThingClassId) {
        // We'll need a cache of the maxChargingCurrent as sometimes need that before a executeAction is finished
        // initializing it to make sure there's always a value in it.
        m_desiredMax[info->thing()] = thing->stateValue(chargerMaxChargingCurrentStateTypeId).toUInt();
    }

    info->finish(Thing::ThingErrorNoError);

}

void IntegrationPluginEasee::postSetupThing(Thing *thing)
{
    if (!m_timer) {
        m_timer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_timer, &PluginTimer::timeout, [this](){
            foreach (Thing *t, myThings()) {
                if (t->thingClassId() == accountThingClassId) {

                    // Refreshing the token if it is about to expire
                    pluginStorage()->beginGroup(t->id().toString());
                    QByteArray accessToken = pluginStorage()->value("accessToken").toByteArray();
                    QByteArray refreshToken = pluginStorage()->value("refreshToken").toByteArray();
                    QDateTime expiry = pluginStorage()->value("expiry").toDateTime();
                    pluginStorage()->endGroup();
                    if (expiry < QDateTime::currentDateTime().addSecs(120)) {
                        this->refreshToken(t);
                    }

                    // Refreshing the products
                    refreshProducts(t);
                }
            }
        });
    }


    if (thing->thingClassId() == accountThingClassId) {
        pluginStorage()->beginGroup(thing->id().toString());
        QByteArray accessToken = pluginStorage()->value("accessToken").toByteArray();
        QDateTime expiry = pluginStorage()->value("expiry").toDateTime();
        pluginStorage()->endGroup();

        qCDebug(dcEasee()) << "Access token:" << accessToken;
        qCDebug(dcEasee()) << "Token expiry:" << expiry;

        SignalRConnection *signalR = new SignalRConnection(QUrl(streamEndpoint), accessToken, hardwareManager()->networkManager(), thing);
        m_signalRConnections.insert(thing, signalR);

        connect(signalR, &SignalRConnection::connectionStateChanged, thing, [=](bool connected){
            foreach (Thing *charger, myThings().filterByParentId(thing->id())) {
                if (connected) {
                    signalR->subscribe(charger->paramValue(chargerThingIdParamTypeId).toString());
                } else {
                    charger->setStateValue(chargerConnectedStateTypeId, false);
                }
            }
        });

        connect(signalR, &SignalRConnection::dataReceived, thing, [=](const QVariantMap &data){
            if (data.value("target").toString() != "ProductUpdate") {
                qCWarning(dcEasee()) << "Unhandled SignalR notification:" << data;
                return;
            }

            foreach (const QVariant &argumentVariant, data.value("arguments").toList()) {
                QVariantMap arg = argumentVariant.toMap();
                QString chargerId = arg.value("mid").toString();
                ObservationPoint dataId = static_cast<ObservationPoint>(arg.value("id").toUInt());
                QVariant value = arg.value("value");
                Thing *charger = myThings().filterByParentId(thing->id()).findByParams({Param(chargerThingIdParamTypeId, chargerId)});
                if (!charger) {
                    qCWarning(dcEasee()) << "Cannot find charger" << chargerId;
                    continue;
                }
                qCDebug(dcEasee()) << "SignalR data point:" << dataId << value;

                switch (dataId) {
                case ObservationPointTotalPower:
                    charger->setStateValue(chargerCurrentPowerStateTypeId, value.toDouble() * 1000);
                    break;
                case ObservationPointSessionEnergy:
                    charger->setStateValue(chargerSessionEnergyStateTypeId, value.toDouble());
                    break;
                case ObservationPointLifetimeEnergy:
                    charger->setStateValue(chargerTotalEnergyConsumedStateTypeId, value.toDouble());
                    break;
                case ObservationPointWiFiRSSI:
                    charger->setStateValue(chargerSignalStrengthStateTypeId, qMin(100, qMax(0, ((value.toInt() + 100) * 2))));
                    break;
                case ObservationPointPilotMode: {
                    QString mode = value.toString();
                    qCDebug(dcEasee()) << "CP mode:" << mode;
                    if (mode == "A") {
                        charger->setStateValue(chargerPluggedInStateTypeId, false);
                    } else if (mode == "B" || mode == "C") {
                        charger->setStateValue(chargerPluggedInStateTypeId, true);
                    }
                    break;
                }
                case ObservationPointOutputPhase:
                    charger->setStateValue(chargerPhaseCountStateTypeId, value.toUInt() == 30 ? 3 : 1);
                    break;
                case ObservationPointChargerOpMode:
                    // 2: charging disabled, 3: enabled and charging, 4: enabled but not charging
                    charger->setStateValue(chargerChargingStateTypeId, value.toUInt() == 3);
                    charger->setStateValue(chargerPowerStateTypeId, value.toUInt() >= 3);
                    break;
                case ObservationPointDynamicChargerCurrent:
                    // May give us 0 when pausing charging etc, ignoring that.
                    if (value.toUInt() > 0) {
                        charger->setStateValue(chargerMaxChargingCurrentStateTypeId, value.toUInt());
                        // Updating the desired value when it is changed by the wallbox (e.g. through app)
                        m_desiredMax[charger] = value.toUInt();
                    }
                    break;
                case ObservationPointMaxChargerCurrent:
                    m_wallboxMax[chargerId] = value.toUInt();
                    charger->setStateMinMaxValues(chargerMaxChargingCurrentStateTypeId, 6, qMin(m_wallboxMax.value(chargerId), m_cableRating.value(chargerId, 32)));
                    break;
                case ObservationPointCableRating:
                    m_cableRating[chargerId] = value.toUInt();
                    charger->setStateMinMaxValues(chargerMaxChargingCurrentStateTypeId, 6, qMin(m_wallboxMax.value(chargerId, 32), value.toUInt()));
                    break;
                case ObservationPointConnectedToCloud:
                    charger->setStateValue(chargerConnectedStateTypeId, value.toString() == "True" || value.toString() == "1");
                    break;


                default:
                    break;
                }
            }
        });
    }
}

void IntegrationPluginEasee::thingRemoved(Thing *thing)
{
    pluginStorage()->beginGroup(thing->id().toString());
    pluginStorage()->remove("");
    pluginStorage()->endGroup();

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_timer);
        m_timer = nullptr;
    }

    if (thing->thingClassId() == chargerThingClassId) {
        m_desiredMax.take(thing);
    }
}

void IntegrationPluginEasee::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == chargerThingClassId) {
        Thing *parentThing = myThings().findById(thing->parentId());
        QString chargerId = thing->paramValue(chargerThingIdParamTypeId).toString();
        if (info->action().actionTypeId() == chargerPowerActionTypeId) {
            bool power = info->action().paramValue(chargerPowerActionPowerParamTypeId).toBool();
            QString actionPath = power ? "start_charging" : "pause_charging";
            QNetworkRequest request = createRequest(parentThing, QString("chargers/%1/commands/%2").arg(chargerId).arg(actionPath));
            qCDebug(dcEasee()) << "Setting power:" << request.url().toString();
            QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QByteArray());
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [=](){
                qCDebug(dcEasee()) << "Reply" << reply->error();
                if (reply->error() == QNetworkReply::NoError) {
                    info->thing()->setStateValue(chargerPowerStateTypeId, power);
                }
                info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);

                // resume/start charging will for some reason reset the dynamicChargerCurrent... We'll have to re-write ours.
                if (power) {
                    uint maxChargingCurrent = m_desiredMax[info->thing()];
                    QVariantMap data;
                    data.insert("dynamicChargerCurrent", maxChargingCurrent);
                    QNetworkRequest request = createRequest(parentThing, QString("chargers/%1/settings").arg(chargerId));
                    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact));
                    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                }
            });
            return;
        }
        if (info->action().actionTypeId() == chargerMaxChargingCurrentActionTypeId) {
            uint maxChargingCurrent = info->action().paramValue(chargerMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt();
            // We'll need this for resume_charging as that call will for some reason reset it to the max of 32A, so we'll need to immediately write this one again.
            m_desiredMax[info->thing()] = maxChargingCurrent;
            QNetworkRequest request = createRequest(parentThing, QString("chargers/%1/settings").arg(chargerId));
            QVariantMap data;
            data.insert("dynamicChargerCurrent", maxChargingCurrent);
            qCDebug(dcEasee()) << "Setting max current:" << request.url().toString() << QJsonDocument::fromVariant(data).toJson();
            QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [reply, info, maxChargingCurrent](){
                qCDebug(dcEasee()) << "Set dynamicaChargerCurrent reply" << reply->error();
                if (reply->error() == QNetworkReply::NoError) {
                    info->thing()->setStateValue(chargerMaxChargingCurrentStateTypeId, maxChargingCurrent);
                }
                info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
            return;
        }
        if (info->action().actionTypeId() == chargerDesiredPhaseCountActionTypeId) {
            uint desiredPhaseCount = info->action().paramValue(chargerDesiredPhaseCountActionDesiredPhaseCountParamTypeId).toUInt();
            bool wasOn = thing->stateValue(chargerPowerStateTypeId).toBool();
            bool oldMaxCurrent = m_desiredMax.value(info->thing());
            if (desiredPhaseCount == thing->stateValue(chargerPhaseCountStateTypeId)) {
                qCInfo(dcEasee()) << "effective phases already equals desired ones...";
                info->finish(Thing::ThingErrorNoError);
                return;
            }
            qCDebug(dcEasee()) << "Pausing charging";
            QNetworkRequest request = createRequest(parentThing, QString("chargers/%1/commands/pause_charging").arg(chargerId));
            QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QByteArray());
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [=](){

                QNetworkRequest request = createRequest(parentThing, QString("chargers/%1/settings").arg(chargerId));

                QVariantMap data;
                data.insert("phaseMode", desiredPhaseCount == 1 ? PhaseModeLockedTo1Phase : PhaseModeLockedTo3Phase);
                qCDebug(dcEasee()) << "Setting single phase charging:" << request.url().toString() << QJsonDocument::fromVariant(data).toJson();
                QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact));
                connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                connect(reply, &QNetworkReply::finished, info, [=](){
                    qCDebug(dcEasee()) << "Set phaseMode reply" << reply->error();
                    if (reply->error() == QNetworkReply::NoError) {
                        info->thing()->setStateValue(chargerDesiredPhaseCountStateTypeId, desiredPhaseCount);
                    }
                    info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);

                    if (wasOn) {
                        qCDebug(dcEasee()) << "Resuming charging";
                        QNetworkRequest request = createRequest(parentThing, QString("chargers/%1/commands/resume_charging").arg(chargerId));
                        QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QByteArray());
                        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                        connect(reply, &QNetworkReply::finished, info, [=](){
                            qCDebug(dcEasee()) << "Restoring max charger current";
                            QNetworkRequest request = createRequest(parentThing, QString("chargers/%1/settings").arg(chargerId));
                            QVariantMap data;
                            data.insert("dynamicChargerCurrent", oldMaxCurrent);
                            qCDebug(dcEasee()) << "Setting max current:" << request.url().toString() << QJsonDocument::fromVariant(data).toJson();
                            QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact));
                            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                        });


                    }
                });
            });

            return;
        }

    }
    info->finish(Thing::ThingErrorNoError);
}

QNetworkRequest IntegrationPluginEasee::createRequest(Thing *thing, const QString &endpoint)
{
    pluginStorage()->beginGroup(thing->id().toString());
    QByteArray accessToken = pluginStorage()->value("accessToken").toByteArray();
    pluginStorage()->endGroup();
    QNetworkRequest request(QUrl(QString("%1/%2").arg(apiEndpoint).arg(endpoint)));
    request.setRawHeader("Authorization", "Bearer " + accessToken);
    request.setRawHeader("accept", "application/json");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/*+json");
    return request;
}

QNetworkReply *IntegrationPluginEasee::refreshToken(Thing *thing)
{
    pluginStorage()->beginGroup(thing->id().toString());
    QByteArray refreshToken = pluginStorage()->value("refreshToken").toByteArray();
    QByteArray accessToken = pluginStorage()->value("accessToken").toByteArray();
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();


    // FIXME: Ideally we should use the refresh_token API and not store user/pass in the config, but it seems to not work
//    QNetworkRequest request(QUrl(QString("%1/%2").arg(apiEndpoint).arg("accounts/refresh_token")));
//    request.setRawHeader("accept", "application/json");
//    request.setRawHeader("content-type", "application/*+json");
//    QVariantMap body;
//    body.insert("refreshToken", refreshToken);
//    body.insert("accessToken", accessToken);

    QNetworkRequest request(QUrl(QString("%1/%2").arg(apiEndpoint).arg("accounts/login")));
    request.setRawHeader("accept", "application/json");
    request.setRawHeader("content-type", "application/*+json");
    QVariantMap body;
    body.insert("userName", username);
    body.insert("password", password);

    qCDebug(dcEasee()) << "Fetching:" << request.url().toString();

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [=](){
        qCDebug(dcEasee) << "Token refresh finished" << reply->error();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcEasee) << "Unable to contact easee server...";
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcEasee) << "Unable to parse json:" << error.errorString() << data;
            return;
        }

        QVariantMap map = jsonDoc.toVariant().toMap();
        qCDebug(dcEasee) << "Token refresh reply:" << data;

        QByteArray accessToken = map.value("accessToken").toByteArray();
        int expiresIn = map.value("expiresIn").toInt();
        QByteArray refreshToken = map.value("refreshToken").toByteArray();

        pluginStorage()->beginGroup(thing->id().toString());
        pluginStorage()->setValue("accessToken", accessToken);
        pluginStorage()->setValue("expiry", QDateTime::currentDateTime().addSecs(expiresIn));
        pluginStorage()->setValue("refreshToken", refreshToken);
        pluginStorage()->endGroup();

        if (m_signalRConnections.contains(thing)) {
            m_signalRConnections.value(thing)->updateToken(accessToken);
        }

    });

    return reply;
}

void IntegrationPluginEasee::refreshProducts(Thing *account)
{
    QNetworkRequest request = createRequest(account, "accounts/products");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, account, [this, account, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcEasee) << "Unable to refresh products:" << reply->error() << reply->errorString();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcEasee) << "Unable to parse json for products:" << error.errorString() << data;
            return;
        }

        QVariantList list = jsonDoc.toVariant().toList();
        qCDebug(dcEasee) << "Products reply:" << qUtf8Printable(data);

        foreach (const QVariant &siteVariant, list) {
            QVariantMap site = siteVariant.toMap();
            foreach (const QVariant &circuitVariant, site.value("circuits").toList()) {
                QVariantMap circuit = circuitVariant.toMap();
//                double maxChartingCurrentLimit = circuit.value("ratedCurrent").toDouble();
                uint circuitId = circuit.value("id").toUInt();
                uint siteId = circuit.value("siteId").toUInt();
                foreach (const QVariant &chargerVariant, circuit.value("chargers").toList()) {
                    QVariantMap charger = chargerVariant.toMap();
                    QString id = charger.value("id").toString();
                    QString name = charger.value("name").toString();

                    ParamList params{Param(chargerThingIdParamTypeId, id)};
                    Thing *existingThing = myThings().filterByParentId(account->id()).findByParams(params);
                    if (!existingThing) {
                        ThingDescriptor descriptor(chargerThingClassId, name, QString(), account->id());
                        descriptor.setParams(params);
                        emit autoThingsAppeared({descriptor});
                    }
                    m_siteIds[id] = siteId;
                    m_circuitIds[id] = circuitId;
                }
            }
        }
    });

}

void IntegrationPluginEasee::refreshCurrentState(Thing *charger)
{
    Thing *parentThing = myThings().findById(charger->parentId());
    QString chargerId = charger->paramValue(chargerThingIdParamTypeId).toString();
    QNetworkRequest request = createRequest(parentThing, QString("chargers/%1/state").arg(chargerId));

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, charger, [charger, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcEasee) << "Unable to fetch charger state:" << reply->error() << reply->errorString();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcEasee) << "Unable to parse json for charger state:" << error.errorString() << data;
            return;
        }

        QVariantMap map = jsonDoc.toVariant().toMap();
        qCDebug(dcEasee) << "Charger state reply:" << qUtf8Printable(jsonDoc.toJson());
        charger->setStateValue(chargerConnectedStateTypeId, map.value("isOnline").toBool());
    });


}

