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

IntegrationPluginEasee::IntegrationPluginEasee()
{

}

IntegrationPluginEasee::~IntegrationPluginEasee()
{
}

void IntegrationPluginEasee::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    QNetworkRequest request(QUrl(QString("https://api.easee.cloud/api/accounts/login")));
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
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Authentication failed. Please try again."));
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
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
        refreshCurrentState(thing);
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

                    if (!m_signalRConnections.value(t)->connected()) {
                        // If the SignalR connection fails for whatever reason, let's poll
                        foreach (Thing *child, myThings().filterByParentId(t->id())) {
                            refreshCurrentState(child);
                        }
                    }
                } else if (t->thingClassId() == chargerThingClassId) {
                    // We'll be using the SignalR connection instead for updates.
                    //refreshCurrentState(t);
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

        SignalRConnection *signalR = new SignalRConnection(QUrl("http://streams.easee.com/hubs/chargers"), accessToken, hardwareManager()->networkManager(), thing);
        m_signalRConnections.insert(thing, signalR);

        connect(signalR, &SignalRConnection::connectionStateChanged, thing, [=](bool connected){
            foreach (Thing *charger, myThings().filterByParentId(thing->id())) {
                charger->setStateValue(chargerConnectedStateTypeId, true);
                if (connected) {
                    signalR->subscribe(charger->paramValue(chargerThingIdParamTypeId).toString());
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
                    } else {
                    }
                    break;
                }
                case ObservationPointOutputPhase:
                    charger->setStateValue(chargerPhaseCountStateTypeId, value.toUInt() > 10 ? 3 : 1);
                    break;
                case ObservationPointChargerOpMode:
                    // 2: charging disabled, 3: enabled and charging, 4: enabled but not charging
                    charger->setStateValue(chargerChargingStateTypeId, value.toUInt() == 3);
                    charger->setStateValue(chargerPowerStateTypeId, value.toUInt() >= 3);
                    break;
                case ObservationPointDynamicChargerCurrent:
                    charger->setStateValue(chargerMaxChargingCurrentStateTypeId, value.toUInt());
                    break;
                case ObservationPointMaxChargerCurrent:
                    charger->setStateMaxValue(chargerMaxChargingCurrentStateTypeId, value.toUInt());
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
}

void IntegrationPluginEasee::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == chargerThingClassId) {
        Thing *parentThing = myThings().findById(thing->parentId());
        QString chargerId = thing->paramValue(chargerThingIdParamTypeId).toString();
        if (info->action().actionTypeId() == chargerPowerActionTypeId) {
            bool power = info->action().paramValue(chargerPowerActionPowerParamTypeId).toBool();
            QString actionPath = power ? "start_charging" : "stop_charging";
            QNetworkRequest request = createRequest(parentThing, QString("chargers/%1/commands/%2").arg(chargerId).arg(actionPath));
            qCDebug(dcEasee()) << "Setting power:" << request.url().toString();
            QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QByteArray());
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [reply, info, power](){
                qCDebug(dcEasee()) << "Reply" << reply->error();
                if (reply->error() == QNetworkReply::NoError) {
                    info->thing()->setStateValue(chargerPowerStateTypeId, power);
                }
                info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
            return;
        }
        if (info->action().actionTypeId() == chargerMaxChargingCurrentActionTypeId) {
            uint maxChargingCurrent = info->action().paramValue(chargerMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt();
            QNetworkRequest request = createRequest(parentThing, QString("chargers/%1/settings").arg(chargerId));
            QVariantMap data;
            data.insert("dynamicChargerCurrent", maxChargingCurrent);
            qCDebug(dcEasee()) << "Setting max current:" << request.url().toString() << QJsonDocument::fromVariant(data).toJson();
            QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [reply, info, maxChargingCurrent](){
                qCDebug(dcEasee()) << "Reply" << reply->error();
                if (reply->error() == QNetworkReply::NoError) {
                    info->thing()->setStateValue(chargerMaxChargingCurrentStateTypeId, maxChargingCurrent);
                }
                info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            });
            return;
        }
        if (info->action().actionTypeId() == chargerDesiredPhaseCountActionTypeId) {
            uint desiredPhaseCount = info->action().paramValue(chargerMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt();
            QNetworkRequest request = createRequest(parentThing, QString("chargers/%1/settings").arg(chargerId));
            QVariantMap data;
            data.insert("lockToSinglePhaseCharging", desiredPhaseCount == 1);
            qCDebug(dcEasee()) << "Setting single phase charging:" << request.url().toString() << QJsonDocument::fromVariant(data).toJson();
            QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact));
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [reply, info, desiredPhaseCount](){
                qCDebug(dcEasee()) << "Reply" << reply->error();
                if (reply->error() == QNetworkReply::NoError) {
                    info->thing()->setStateValue(chargerDesiredPhaseCountStateTypeId, desiredPhaseCount);
                }
                info->finish(reply->error() == QNetworkReply::NoError ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
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
    QNetworkRequest request(QUrl(QString("https://api.easee.cloud/api/%1").arg(endpoint)));
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
//    QNetworkRequest request(QUrl(QString("https://api.easee.cloud/api/accounts/refresh_token")));
//    request.setRawHeader("accept", "application/json");
//    request.setRawHeader("content-type", "application/*+json");
//    QVariantMap body;
//    body.insert("refreshToken", refreshToken);
//    body.insert("accessToken", accessToken);

    QNetworkRequest request(QUrl(QString("https://api.easee.cloud/api/accounts/login")));
    request.setRawHeader("accept", "application/json");
    request.setRawHeader("content-type", "application/*+json");
    QVariantMap body;
    body.insert("userName", username);
    body.insert("password", password);


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

        m_signalRConnections.value(thing)->updateToken(accessToken);

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
        charger->setStateValue(chargerSignalStrengthStateTypeId, qMax(0, qMin(100, (map.value("wiFiRSSI").toInt() + 100) * 2)));
        charger->setStateValue(chargerCurrentPowerStateTypeId, map.value("totalPower").toDouble() * 1000);
        charger->setStateValue(chargerPhaseCountStateTypeId, map.value("outputPhase").toUInt() > 10 ? 3 : 1);
        charger->setStateValue(chargerChargingStateTypeId, map.value("chargerOpMode").toUInt() == 3);

        // 1: unplugged, 2: charging disabled, 3: enabled and charging, 4: enabled but not charging
        uint chargerOpMode = map.value("chargerOpMode").toUInt();
        charger->setStateValue(chargerPluggedInStateTypeId, chargerOpMode >= 2);
        charger->setStateValue(chargerChargingStateTypeId, chargerOpMode == 3);
        charger->setStateValue(chargerPowerStateTypeId, chargerOpMode >= 3);

        charger->setStateValue(chargerMaxChargingCurrentStateTypeId, map.value("dynamicChargerCurrent").toUInt());
        charger->setStateMaxValue(chargerMaxChargingCurrentStateTypeId, 6); // Fixme: where to get this from?
        charger->setStateMaxValue(chargerMaxChargingCurrentStateTypeId, 32); // Fixme: where to get this from?

        charger->setStateValue(chargerTotalEnergyConsumedStateTypeId, map.value("lifetimeEnergy").toDouble());
        charger->setStateValue(chargerSessionEnergyStateTypeId, map.value("sessionEnergy").toDouble());

    });


}

