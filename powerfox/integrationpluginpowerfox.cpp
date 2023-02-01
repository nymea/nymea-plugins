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

#include "integrationpluginpowerfox.h"
#include "plugininfo.h"
#include "plugintimer.h"

#include <network/networkaccessmanager.h>
#include <QNetworkReply>
#include <QJsonDocument>

IntegrationPluginPowerfox::IntegrationPluginPowerfox()
{

}

IntegrationPluginPowerfox::~IntegrationPluginPowerfox()
{
}

void IntegrationPluginPowerfox::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter the login credentials for your powerfox account."));
}

void IntegrationPluginPowerfox::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{
    // Using /main/current as that one has the highest rate limit and we don't want to lock up the api for the following
    // setupThing call as /all/devices can only be called once per minute.
    QNetworkRequest request(QUrl("https://backend.powerfox.energy/api/2.0/my/main/current"));
    QString concatenated = username + ":" + password;
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;
    request.setRawHeader("Authorization", headerData.toLocal8Bit());
    qCDebug(dcPowerfox()) << "requesting:" << request.url().toString() << headerData;
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [info, reply, this, username, password](){
        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            qCWarning(dcPowerfox()) << "Error fetching devices from account";
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Error logging to powerfox. Please try again."));
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPowerfox()) << "Error getting paired devices" << reply->error() << reply->errorString();
            info->finish(Thing::ThingErrorHardwareFailure, reply->errorString());
            return;
        }

        qCDebug(dcPowerfox) << "Auth request succeeded";

        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", password);
        pluginStorage()->endGroup();
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginPowerfox::setupThing(ThingSetupInfo *info)
{
    if (info->thing()->thingClassId() == accountThingClassId) {
        QNetworkReply *reply = request(info->thing(), "/all/devices"); // Max 1 per minute
        connect(reply, &QNetworkReply::finished, info, [info, reply, this](){
            if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
                qCWarning(dcPowerfox()) << "Error fetching devices from account";
                info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Login error at powerfox.energy."));
                return;
            }
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcPowerfox()) << "Error fetching devices from account" << reply->error();
                info->finish(Thing::ThingErrorAuthenticationFailure, reply->errorString());
                return;
            }

            QByteArray data = reply->readAll();

            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcPowerfox()) << "JSON parse error in response from powerfox:" << error.error << error.errorString() << data;
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unable to process response from from server."));
                return;
            }

            info->finish(Thing::ThingErrorNoError);

            info->thing()->setStateValue(accountConnectedStateTypeId, true);
            info->thing()->setStateValue(accountLoggedInStateTypeId, true);

            foreach (const QVariant &entry, jsonDoc.toVariant().toList()) {
                QVariantMap entryMap = entry.toMap();
                QString id = entryMap.value("DeviceId").toString();
                bool mainDevice = entryMap.value("MainDevice").toBool();
                Division division = static_cast<Division>(entryMap.value("Devision").toInt());

                if (!mainDevice) {
                    qCDebug(dcPowerfox()) << "Skipping non-main device" << qUtf8Printable(QJsonDocument::fromVariant(entryMap).toJson());
                    continue;
                }

                if (division != DivisionPowerMeter) {
                    qCInfo(dcPowerfox()) << "Device type" << division << "is not yet supported.";
                    continue;
                }

                Thing *child = myThings().filterByParentId(info->thing()->id()).findByParams({Param(powerMeterThingIdParamTypeId, id)});
                if (!child) {
                    qCDebug(dcPowerfox()) << "Found new power meter device:" << id;
                    ThingDescriptor descriptor(powerMeterThingClassId, "powerfox power meter", QString(), info->thing()->id());
                    descriptor.setParams({Param(powerMeterThingIdParamTypeId, id)});
                    emit autoThingsAppeared({descriptor});
                }
            }
        });

        return;
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginPowerfox::postSetupThing(Thing */*thing*/)
{
    if (!m_pollTimer) {
        m_pollTimer = hardwareManager()->pluginTimerManager()->registerTimer(4);
        connect(m_pollTimer, &PluginTimer::timeout, this, [this](){
            foreach (Thing *account, myThings().filterByInterface("account")) {
                foreach (Thing *powerMeter, myThings().filterByParentId(account->id()).filterByInterface("energymeter")) {
                    QUrlQuery query;
                    query.addQueryItem("unit", "kWh");
                    // Can be called at max once per 3 secs. Not sure if that's per account or per meter ID yet. Assuming per meter ID for now.
                    QNetworkReply *reply = request(account, "/" + powerMeter->paramValue(powerMeterThingIdParamTypeId).toString() + "/current", query);
                    connect(reply, &QNetworkReply::finished, powerMeter, [this, account, powerMeter, reply](){
                        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
                            account->setStateValue(accountLoggedInStateTypeId, false);
                            markAsDisconnected(powerMeter);
                        }
                        if (reply->error() != QNetworkReply::NoError) {
                            qCWarning(dcPowerfox()) << "Failed to poll power meter:" << reply->error() << reply->errorString();
                            markAsDisconnected(powerMeter);
                            return;
                        }

                        QByteArray data = reply->readAll();
                        QJsonParseError error;
                        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                        if (error.error != QJsonParseError::NoError) {
                            qCWarning(dcPowerfox()) << "Unable to parse reply from powerfox:" << error.error << error.errorString() << data;
                            return;
                        }

                        account->setStateValue(accountConnectedStateTypeId, true);

                        QVariantMap map = jsonDoc.toVariant().toMap();
                        powerMeter->setStateValue(powerMeterConnectedStateTypeId, !map.value("Outdated").toBool());

                        powerMeter->setStateValue(powerMeterCurrentPowerStateTypeId, map.value("Watt").toDouble());
                        powerMeter->setStateValue(powerMeterTotalEnergyConsumedStateTypeId, map.value("A_Plus").toDouble());
                        powerMeter->setStateValue(powerMeterTotalEnergyProducedStateTypeId, map.value("A_Minus").toDouble());

                        // We don't get voltage/current from the API, let's assume 230V as powerfox is only available in Europe for now
                        powerMeter->setStateValue(powerMeterVoltagePhaseAStateTypeId, 230);
                        powerMeter->setStateValue(powerMeterCurrentPhaseAStateTypeId, powerMeter->stateValue(powerMeterCurrentPowerStateTypeId).toDouble() / powerMeter->stateValue(powerMeterVoltagePhaseAStateTypeId).toDouble());
                    });
                }
            }
        });
    }
}

void IntegrationPluginPowerfox::thingRemoved(Thing */*thing*/)
{
    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pollTimer);
        m_pollTimer = nullptr;
    }
}

QNetworkReply *IntegrationPluginPowerfox::request(Thing *thing, const QString &path, const QUrlQuery &query)
{
    pluginStorage()->beginGroup(thing->id().toString());
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();

    QUrl url;
    url.setScheme("https");
    url.setHost("backend.powerfox.energy");
    url.setPath("/api/2.0/my" + path);
    url.setQuery(query);
    QNetworkRequest request(url);
    QString concatenated = username + ":" + password;
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;
    request.setRawHeader("Authorization", headerData.toLocal8Bit());
//    qCDebug(dcPowerfox()) << "requesting:" << url.toString() << headerData;
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    return reply;
}

void IntegrationPluginPowerfox::markAsDisconnected(Thing *thing)
{
    qCDebug(dcPowerfox()) << "Mark thing as disconnected" << thing;
    thing->setStateValue(powerMeterConnectedStateTypeId, false);
    thing->setStateValue(powerMeterCurrentPowerStateTypeId, 0);
    thing->setStateValue(powerMeterCurrentPhaseAStateTypeId, 0);
    thing->setStateValue(powerMeterVoltagePhaseAStateTypeId, 0);
}


