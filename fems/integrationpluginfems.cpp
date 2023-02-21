/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2022 Consolinno Energy GmbH                              *
 * <felix.stoecker@consolinno.de>                                          *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation;                  *
 *  version 3 of the License.                                              *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationpluginfems.h"
#include "network/networkaccessmanager.h"
#include "network/networkdevicediscovery.h"
#include "plugininfo.h"
#include "plugintimer.h"

#include <QDebug>
#include <QJsonDocument>
#include <QPointer>
#include <QUrl>
#include <QUrlQuery>
#include <math.h>

static int id_increment = 0;
// use this to determine which connection to refresh -> connectionSwitch % 2 0
// == battery 1 == meter 2 == sum



IntegrationPluginFems::IntegrationPluginFems(QObject *parent)
    : IntegrationPlugin(parent) {
    qCDebug(dcFems()) << "IntegrationPluginsFems constructor called";
    this->ownId = id_increment++;
}

void IntegrationPluginFems::init() {
    // Initialisation can be done here.
    qCDebug(dcFems()) << "INIT called";
    meter = METER_0;
    batteryState = "idle";
    qCDebug(dcFems()) << "Plugin initialized.";
}

void IntegrationPluginFems::startPairing(ThingPairingInfo *info) {
    qCDebug(dcFems()) << "Start Pairing called";
    info->finish(Thing::ThingErrorNoError,
                 QString(QT_TR_NOOP("Please enter your login credentials for the FEMS connection")));
}

void IntegrationPluginFems::confirmPairing(ThingPairingInfo *info,
                                           const QString &username,
                                           const QString &password) {
    qCDebug(dcFems()) << "Starting beginGroup and adding username and pwd";
    pluginStorage()->beginGroup(FEMS_PLUGIN_STORAGE_ID);
    pluginStorage()->setValue("username", username);
    pluginStorage()->setValue("password", password);
    pluginStorage()->endGroup();
    qCDebug(dcFems()) << QString("Username: %1 and pwd %2 added").arg(username).arg(password);
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginFems::setupThing(ThingSetupInfo *info) {

    qCDebug(dcFems()) << "Setting up Thing";
    Thing *thing = info->thing();
    qCDebug(dcFems()) << "Setting up" << thing;
    qCDebug(dcFems()) << "Thing is " << thing;

    if (thing->thingClassId() == connectionThingClassId) {
        qCDebug(dcFems()) << "ConnectionThingClass found";

        QHostAddress address(
                    thing->paramValue(connectionThingAddressParamTypeId).toString());

        // Handle reconfigure
        if (m_femsConnections.values().contains(thing)) {
            FemsConnection *connection = m_femsConnections.key(thing);
            m_femsConnections.remove(connection);
            connection->deleteLater();
        }

        // Create the connection
        pluginStorage()->beginGroup(FEMS_PLUGIN_STORAGE_ID);
        QString user = pluginStorage()->value("username").toString();
        QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();
        qCDebug(dcFems()) << "Username: " << user;
        qCDebug(dcFems()) << "Password: " << password;
        FemsConnection *connection = new FemsConnection(
                    hardwareManager()->networkManager(), address, thing,
                    user,
                    password,
                    //thing->paramValue(connectionThingUsernameParamTypeId).toString(),
                    //thing->paramValue(connectionThingPasswordParamTypeId).toString(),
                    thing->paramValue(connectionThingEdgeParamTypeId).toBool(),
                    thing->paramValue(connectionThingPortParamTypeId).toString());
        qCDebug(dcFems()) << "Creating isAvailableDevice By Checking _sum/State";
        FemsNetworkReply *reply = connection->isAvailable();
        qCDebug(dcFems()) << "Connecting Signal and Slot";
        connect(reply, &FemsNetworkReply::finished, info, [=] {
            qCDebug(dcFems()) << "Callback called";
            QByteArray data = reply->networkReply()->readAll();
            qCDebug(dcFems()) << "reply data";
            qCDebug(dcFems()) << data;
            if (reply->networkReply()->error() != QNetworkReply::NoError) {
                // no URL bc URL contains uname and pwd
                // qcWarning(dcFems() << "Network request error:") <<
                // reply->networkReply()->error() <<
                // reply->networkReply()->errorString();
                qCDebug(dcFems()) << "Error: " << reply->networkReply()->error();
                qCDebug(dcFems()) << "WE ARE HERE";
                if (reply->networkReply()->error() ==
                        QNetworkReply::ContentNotFoundError) {
                    info->finish(
                                Thing::ThingErrorHardwareNotAvailable,
                                QT_TR_NOOP("The device does not reply to our requests. Please "
                                           "verify that the FEMS API is enabled on the device."));
                } else {
                    qCDebug(dcFems()) << "NETWORK REPLY FAILED";
                    info->finish(Thing::ThingErrorHardwareNotAvailable,
                                 QT_TR_NOOP("The device is not reachable."));
                }
                return;
            }

            // Convert the rawdata to a JSON document
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcFems()) << "Failed to parse JSON data" << data << ":"
                                    << error.errorString() << data;
                info->finish(Thing::ThingErrorHardwareFailure,
                             QT_TR_NOOP("The data received from the device could not "
                                        "be processed because the format is unknown."));
                return;
            }

            // NOT POSSIBLE -> PARENT HERE NOT CHILDREN!
            QVariantMap responseMap = jsonDoc.toVariant().toMap();
            qint8 status = responseMap.value("value", -1).toInt();
            /*
if (status >= 0) {
  // 0 == ok, 1 == info, 2 == Warning, 3 == fault (todo change to strings)
  thing->setStateValue(femsstatusStatusStateTypeId, status);
}
qCDebug(dcFems()) << "Fems Status" <<
responseMap.value("value").toString();*/
            // STATE Connection
            qCDebug(dcFems()) << "Adding new Connection";
            qCDebug(dcFems()) << "status: " << status;
            if (status >= 0) {
                m_femsConnections.insert(connection, thing);
                qCDebug(dcFems()) << "Connection added "
                        << this->m_femsConnections.contains(connection);
                info->finish(Thing::ThingErrorNoError);
                thing->setStateValue("connected", true);
            } else {
                info->finish(Thing::ThingErrorHardwareFailure);
                thing->setStateValue("connected", false);
            }
        });
        connect(connection, &FemsConnection::availableChanged, this,
                [=](bool available) {
            qCDebug(dcFems()) << thing << "Available changed" << available;
            thing->setStateValue("connected", available);

            if (!available) {
                // Update all child things, they will be set to available once
                // the connection starts working again
                foreach (Thing *childThing,
                         myThings().filterByParentId(thing->id())) {
                    childThing->setStateValue("connected", false);
                }
            }
        });
        qCDebug(dcFems()) << "Here is line after callback declaration";
    } else if ((thing->thingClassId() == meterThingClassId) ||
               (thing->thingClassId() == batteryThingClassId) || (thing -> thingClassId() == femsstatusThingClassId)) {
        qCDebug(dcFems()) << "This line appears because Parent was setup and now children "
                   "are created";
        Thing *parentThing = myThings().findById(thing->parentId());
        if (!parentThing) {
            qCWarning(dcFems()) << "Could not find the parent for" << thing;
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        FemsConnection *connection = m_femsConnections.key(parentThing);
        if (!connection) {
            qCWarning(dcFems()) << "Could not find the parent connection for"
                                << thing;
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        info->finish(Thing::ThingErrorNoError);
    } else {
        Q_ASSERT_X(false, "setupThing",
                   QString("Unhandled thingClassId: %1")
                   .arg(thing->thingClassId().toString())
                   .toUtf8());
    }
}

void IntegrationPluginFems::postSetupThing(Thing *thing) {

    qCDebug(dcFems()) << "Post setup" << thing->name();
    qCDebug(dcFems()) << "Post Setup";

    if (thing->thingClassId() == connectionThingClassId) {
        qCDebug(dcFems()) << "ConnectionClass sets up Timer";
        if (!m_connectionRefreshTimer) {
            qCDebug(dcFems()) << "Creating Timer";
            m_connectionRefreshTimer =
                    hardwareManager()->pluginTimerManager()->registerTimer(5);
            qCDebug(dcFems()) << "connecting RefreshTimer and TimeOut";
            qCDebug(dcFems()) << "Size Of Connections: " << m_femsConnections.keys().length();

            connect(m_connectionRefreshTimer, &PluginTimer::timeout, this, [this]() {
                qCDebug(dcFems()) << "Refreshing each connection";
                foreach (FemsConnection *connection, m_femsConnections.keys()) {
                    refreshConnection(connection);
                }
            });
            qCDebug(dcFems()) << "Connection Refresh Timer Start";
            m_connectionRefreshTimer->start();
        }

        FemsConnection *connection = m_femsConnections.key(thing);
        if (connection) {
            refreshConnection(connection);
        }
    }
}

void IntegrationPluginFems::thingRemoved(Thing *thing) {
    qCDebug(dcFems()) << "Thing will be removed";
    if (thing->thingClassId() == connectionThingClassId) {
        qCDebug(dcFems()) << "ConnectionThing is being removed";
        FemsConnection *connection = m_femsConnections.key(thing);
        m_femsConnections.remove(connection);
        connection->deleteLater();
    } else{
        qCDebug(dcFems()) << "Child is going to be removed";
           thing -> deleteLater();
    }

    if (myThings().filterByThingClassId(connectionThingClassId).isEmpty()) {
        qCDebug(dcFems()) << "Unregistering Timer";
        this->meterCreated = false;
        this->batteryCreated = false;
        this->statusCreated = false;
        hardwareManager()->pluginTimerManager()->unregisterTimer(
                    m_connectionRefreshTimer);
        m_connectionRefreshTimer = nullptr;
    }

}

void IntegrationPluginFems::executeAction(ThingActionInfo *info) {
    Q_UNUSED(info)
}

// Poll data again
void IntegrationPluginFems::refreshConnection(FemsConnection *connection) {
    if (connection->busy()) {
        qCWarning(dcFems()) << "Connection busy. Skipping refresh cycle for host"
                            << connection->address().toString();
        return;
    }
    // 3 things, parent, and battery as well as meter but only parent connection
    // nec. GET Meter and Battery ID Later and update param info
    Thing *connectionThing = m_femsConnections.value(connection);
    if (!connectionThing)
        return;
    qCDebug(dcFems()) << "Updating States";
    switch (connectionSwitch) {
    case 0:
        qCDebug(dcFems()) << "#############################################################";
        qCDebug(dcFems()) << "#############################################################";
        qCDebug(dcFems()) << "#############################################################";
        qCDebug(dcFems()) << "Updating Storages";
        if (myThings()
                .filterByParentId(m_femsConnections.value(connection)->id())
                .filterByThingClassId(batteryThingClassId)
                .length() < 1 &&
                !this->batteryCreated) {
            qCDebug(dcFems()) << "Creating ESS";
            ThingDescriptor descriptor(batteryThingClassId, "FEMS Battery", QString(),
                                       connectionThing->id());
            ParamList params;
            params.append(Param(batteryThingIdParamTypeId, "battery"));
            descriptor.setParams(params);
            emit autoThingsAppeared(ThingDescriptors() << descriptor);
            this->batteryCreated = true;
        }
        this->updateStorages(connection);
        break;
    case 1:
        qCDebug(dcFems()) << "#############################################################";
        qCDebug(dcFems()) << "#############################################################";
        qCDebug(dcFems()) << "#############################################################";
        qCDebug(dcFems()) << "Updating Meters";

        this->updateMeters(connection);
        if (myThings()
                .filterByParentId(m_femsConnections.value(connection)->id())
                .filterByThingClassId(meterThingClassId)
                .length() < 1 &&
                !this->meterCreated) {

            ThingDescriptor descriptor(meterThingClassId, "FEMS Meter", QString(),
                                       connectionThing->id());
            ParamList params;
            params.append(Param(meterThingIdParamTypeId, "meter"));
            descriptor.setParams(params);
            emit autoThingsAppeared(ThingDescriptors() << descriptor);
            this->meterCreated = true;
        }
        break;
    case 2:
    default:
        qCDebug(dcFems()) << "#############################################################";
        qCDebug(dcFems()) << "#############################################################";
        qCDebug(dcFems()) << "#############################################################";
        qCDebug(dcFems()) << "#############################################################";
        qCDebug(dcFems()) << "Updating Sum";
        if (myThings()
                .filterByParentId(m_femsConnections.value(connection)->id())
                .filterByThingClassId(femsstatusThingClassId)
                .length() < 1 &&
                !this->statusCreated) {
            ThingDescriptor descriptor(femsstatusThingClassId, "FEMS Status", QString(),
                                       connectionThing->id());
            ParamList params;
            params.append(Param(femsstatusThingIdParamTypeId, "Status"));
            descriptor.setParams(params);
            emit autoThingsAppeared(ThingDescriptors() << descriptor);
            this->statusCreated = true;
        }
        this->updateSumState(connection);
        break;
    }
    this->connectionSwitch = (this->connectionSwitch + 1) % 3;
}

void IntegrationPluginFems::updateStorages(FemsConnection *connection) {

    Thing *parentThing = m_femsConnections.value(connection);
    // Get everything from the sum that is ESS related.
    // most states are available at the "sum"
    // Since FEMS/OpenEMS can have mutltiple ess it can be risky to get multiple
    // ess but normally only 1 ess (ess0) ist just start and try ess0 -> if not
    // available -> SKIP However sum should supply enough data

    // ChargingState -> this should be done seperately when comparing charing and
    // discharging energy idle, charging, discharging

    // ChargingEnergy(Nymea)
    // EssActiveChargeEnergy(OpenEMS)
    FemsNetworkReply *essActiveChargeEnergy =
            connection->getFemsDataPoint(ESS_ACTIVE_CHARGE_ENERGY);

    connect(essActiveChargeEnergy, &FemsNetworkReply::finished, this,
            [this, essActiveChargeEnergy, parentThing]() {
        qCDebug(dcFems()) << "Checking ESS ACTIVE ENERGY";
        if (connectionError(essActiveChargeEnergy)) {
            qCDebug(dcFems()) << "Connection error at ESS Active Energy";
            return;
        }
        //TODO networkReply->error(){return;}
        QByteArray data = essActiveChargeEnergy->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCDebug(dcFems()) << "JSON ERROR at ESS ACTIVE ENERGY";
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();
            return;
        }
        qCDebug(dcFems()) << "Getting Variant for ESS ACTIVE ENERGY";
        QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
        // GET "value" of data
        qCDebug(dcFems()) << "Value of ESS ACTIVE ENERGY received: "
                << var->toString();
        qCDebug(dcFems()) << "Value of things: " << batteryThingClassId << " and"
                << batteryChargingEnergyStateTypeId;

        // qCDebug(dcFems()) << "addValueToThing is called with parentThing etc";
        Thing *child = this->GetThingByParentAndClassId(
                    parentThing, batteryThingClassId);
        qCDebug(dcFems()) << "Child: " << child;
        if (child != nullptr) {
            qCDebug(dcFems()) << "calling child ValueToAddThing";
            this->addValueToThing(child, batteryChargingEnergyStateTypeId,
                                  var, DOUBLE, 0);
        }
        // addValueToThing(parentThing, batteryThingClassId,
        //              batteryChargingEnergyStateTypeId, var, DOUBLE, 0);
        qCDebug(dcFems()) << "Add Value to thing done";
        var = NULL;
        delete var;
        //TODO read FEMS ESS
        checkBatteryState(parentThing);
    });

    // DischargingEnergy(Nymea)
    // EssActiveDischargeEnergy(OpenEMS)
    FemsNetworkReply *essActiveDischargeEnergy =
            connection->getFemsDataPoint(ESS_ACTIVE_DISCHARGE_ENERGY);

    connect(essActiveDischargeEnergy, &FemsNetworkReply::finished, this,
            [this, essActiveDischargeEnergy, parentThing]() {
        qCDebug(dcFems()) << "Checking essACtiveDischargeEnergy";
        if (connectionError(essActiveDischargeEnergy)) {

            return;
        }
        QByteArray data =
                essActiveDischargeEnergy->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }
        // GET "value" of data
        QVariant *var =
                new QVariant((this->getValueOfRequestedData(&jsonDoc)));
        addValueToThing(parentThing, batteryThingClassId,
                        batteryDischarginEnergyStateTypeId, var, DOUBLE, 0);
        var = NULL;
        delete var;
        checkBatteryState(parentThing);
    });

    // CurrentPower(Nymea)
    // EssActivePower(OpenEMS)

    FemsNetworkReply *essActivePower =
            connection->getFemsDataPoint(ESS_ACTIVE_POWER);

    connect(essActivePower, &FemsNetworkReply::finished, this,
            [this, essActivePower, parentThing]() {
        qCDebug(dcFems()) << "Ess Active Power";
        if (connectionError(essActivePower)) {

            return;
        }
        QByteArray data = essActivePower->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }
        QVariant *var =
                new QVariant((this->getValueOfRequestedData(&jsonDoc)));
        // GET "value" of data
        addValueToThing(parentThing, batteryThingClassId,
                        batteryCurrentPowerStateTypeId, var, DOUBLE, 0);
        var = NULL;
        delete var;
    });

    // CurrentPowerA
    // EssActivePowerL1

    FemsNetworkReply *essActivePowerL1 =
            connection->getFemsDataPoint(ESS_ACTIVE_POWER_L1);

    connect(essActivePowerL1, &FemsNetworkReply::finished, this,
            [this, essActivePowerL1, parentThing]() {
        qCDebug(dcFems()) << "essActivePowerL1";
        if (connectionError(essActivePowerL1)) {

            return;
        }
        QByteArray data = essActivePowerL1->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = this->jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }
        QVariant *var =
                new QVariant((this->getValueOfRequestedData(&jsonDoc)));
        // GET "value" of data
        addValueToThing(parentThing, batteryThingClassId,
                        batteryCurrentPowerAStateTypeId, var, DOUBLE, 0);
        var = NULL;
        delete var;
    });

    // CurrentPowerB
    // EssActivePowerL2

    FemsNetworkReply *essActivePowerL2 =
            connection->getFemsDataPoint(ESS_ACTIVE_POWER_L2);

    connect(essActivePowerL2, &FemsNetworkReply::finished, this,
            [this, essActivePowerL2, parentThing]() {
        qCDebug(dcFems()) << "Ess Active Power L2";
        if (connectionError(essActivePowerL2)) {

            return;
        }
        QByteArray data = essActivePowerL2->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }
        QVariant *var =
                new QVariant((this->getValueOfRequestedData(&jsonDoc)));
        // GET "value" of data
        addValueToThing(parentThing, batteryThingClassId,
                        batteryCurrentPowerBStateTypeId, var, DOUBLE, 0);
        var = NULL;
        delete var;
    });

    // CurrentPowerC
    // EssActivePowerL3

    FemsNetworkReply *essActivePowerL3 =
            connection->getFemsDataPoint(ESS_ACTIVE_POWER_L3);

    connect(
                essActivePowerL3, &FemsNetworkReply::finished, this,
                [this, essActivePowerL3, parentThing]() {
        qCDebug(dcFems()) << "Ess Active Power L3";

        if (this->connectionError(essActivePowerL3)) {

            return;
        }
        QByteArray data = essActivePowerL3->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = this->jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }
        QVariant *var = new QVariant((this->getValueOfRequestedData(&jsonDoc)));
        // GET "value" of data
        this->addValueToThing(parentThing, batteryThingClassId,
                              batteryCurrentPowerCStateTypeId, var, DOUBLE, 0);
        var = NULL;
        delete var;
    });

    // Capacity
    // Try ess0/Capacity
    FemsNetworkReply *essCapacity = connection->getFemsDataPoint(ESS_CAPACITY);

    connect(essCapacity, &FemsNetworkReply::finished, this,
            [this, essCapacity, parentThing]() {
        qCDebug(dcFems()) << "Ess Capacity";
        if (connectionError(essCapacity)) {

            return;
        }
        QByteArray data = essCapacity->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }
        QVariant *var =
                new QVariant((this->getValueOfRequestedData(&jsonDoc)));
        // GET "value" of data
        addValueToThing(parentThing, batteryThingClassId,
                        batteryCapacityStateTypeId, var, DOUBLE, 0);
        var = NULL;
        delete var;
    });

    // BatteryLevel (SoC)
    // EssSoc
    FemsNetworkReply *essBatteryLevel = connection->getFemsDataPoint(ESS_SOC);

    connect(essBatteryLevel, &FemsNetworkReply::finished, this,
            [this, essBatteryLevel, parentThing]() {
        qCDebug(dcFems()) << "ess Battery Level";
        if (connectionError(essBatteryLevel)) {

            return;
        }
        QByteArray data = essBatteryLevel->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }
        QVariant *var =
                new QVariant((this->getValueOfRequestedData(&jsonDoc)));
        // GET "value" of data
        addValueToThing(parentThing, batteryThingClassId,
                        batteryBatteryLevelStateTypeId, var, DOUBLE, 0);
        var = NULL;
        delete var;

        // Calc state of Health
        Thing *thing =
                GetThingByParentAndClassId(parentThing, batteryThingClassId);
        if (thing != nullptr) {
            QVariant socStateValue =
                    thing->stateValue(batteryBatteryLevelStateTypeId);
            int soc = socStateValue.toInt();
            bool critical = false;
            if (soc <= 10) {
                critical = true;
            }
            QVariant var = critical ? "true" : "false";
            ;
            addValueToThing(thing, batteryBatteryCriticalStateTypeId, &var,
                            MY_BOOLEAN, 0);
        }
    });

    // CellTemperature <- mostly not available
    // try but ....ye ess0/Temperature
    FemsNetworkReply *essTemperature =
            connection->getFemsDataPoint("ess0/Temperature");

    connect(essTemperature, &FemsNetworkReply::finished, this,
            [this, essTemperature, parentThing]() {
        qCDebug(dcFems()) << "ESS Temperature";
        if (this->connectionError(essTemperature)) {

            return;
        }
        QByteArray data = essTemperature->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = this->jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }
        QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
        // GET "value" of data
        addValueToThing(parentThing, batteryThingClassId,
                        batteryCellTemperatureStateTypeId, var, DOUBLE, 0);
        var = NULL;
        delete var;
    });
    qCDebug(dcFems()) << "Battery Done";
}

void IntegrationPluginFems::updateSumState(FemsConnection *connection) {
    Thing *parentThing = m_femsConnections.value(connection);
    // Get everything from the sum that is coherend to states.
    // e.g. Fems State of SUM, then if not at fault -> set Battery and Meter
    // states.

    // ChargingEnergy(Nymea)
    // EssActiveChargeEnergy(OpenEMS)
    FemsNetworkReply *sumState = connection->getFemsDataPoint(SUM_STATE);

    connect(sumState, &FemsNetworkReply::finished, this,
                [this, sumState, parentThing]() {
        qCDebug(dcFems()) << "sumState";
        if (connectionError(sumState)) {

            return;
        }
        QByteArray data = sumState->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }
        QVariant *var = new QVariant((this->getValueOfRequestedData(&jsonDoc)));
        // GET "value" of data
        qCDebug(dcFems()) << "Adding SUM STATE";
        addValueToThing(parentThing, femsstatusThingClassId,
                        femsstatusStatusStateTypeId, var, MY_INT, 0);

        if (var != nullptr) {
            qCDebug(dcFems()) << "Checking fo Updating States";
            QVariant *varBool = new QVariant(true);
            // FEMS STATE == FAULT on 3
            if (var->toInt() == 3) {
                varBool->setValue(false);
            }
            qCDebug(dcFems()) << "ADDING CONNECTION STATE: " << varBool;
            //TODO addValueToThingOverload( instead of QVariant -> boolean)
            addValueToThing(parentThing, meterThingClassId,
                            meterConnectedStateTypeId, varBool, MY_BOOLEAN, 0);

            addValueToThing(parentThing, batteryThingClassId,
                            batteryConnectedStateTypeId, varBool, MY_BOOLEAN, 0);

            addValueToThing(parentThing, femsstatusThingClassId,
                            femsstatusConnectedStateTypeId, varBool, MY_BOOLEAN,
                            0);
            varBool = NULL;
            delete varBool;
        }
        var = NULL;
        delete var;
    });
}

void IntegrationPluginFems::updateMeters(FemsConnection *connection) {
    qCDebug(dcFems()) << "STARTING METERS";
    Thing *parentThing = m_femsConnections.value(connection);
    // Get everything from the sum that is meter related.
    // most states are available at the "sum"
    // Since FEMS/OpenEMS can have mutltiple Meters it can be risky to get
    // multiple meters start with meter0 -> if of type AsymmetricMeter it will
    // have values at L1-L3 phase calls If Symmetric it won't You can try with
    // meter0 and meter1 (should be sufficent) However sum should supply enough
    // data

    // GridActivePower
    FemsNetworkReply *currentGridPowerReply =
            connection->getFemsDataPoint(GRID_ACTIVE_POWER);

    connect(currentGridPowerReply, &FemsNetworkReply::finished, this,
            [this, currentGridPowerReply, parentThing]() {
        qCDebug(dcFems()) << "Current Grid Power";
        if (connectionError(currentGridPowerReply)) {

            return;
        }
        QByteArray data = currentGridPowerReply->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }
        QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
        addValueToThing(parentThing, meterThingClassId,
                        meterCurrentGridPowerStateTypeId, var, DOUBLE, 0);
        var = NULL;
        delete var;
    });

    // ProductionActivePower
    FemsNetworkReply *currentPowerProduction =
            connection->getFemsDataPoint(PRODCUTION_ACTIVE_POWER);
    connect(currentPowerProduction, &FemsNetworkReply::finished, this,
            [this, currentPowerProduction, parentThing]() {
        qCDebug(dcFems()) << "Current Power Production";
        if (connectionError(currentPowerProduction)) {

            return;
        }
        QByteArray data = currentPowerProduction->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }

        QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
        addValueToThing(parentThing, meterThingClassId,
                        meterCurrentPowerProductionStateTypeId, var, DOUBLE,
                        0);
        var = NULL;
        delete var;
    });

    // ProductionAcActivePower
    FemsNetworkReply *currentPowerProductionAc =
            connection->getFemsDataPoint(PRODUCTION_ACTIVE_AC_POWER);
    connect(currentPowerProductionAc, &FemsNetworkReply::finished, this,
            [this, currentPowerProductionAc, parentThing]() {
        qCDebug(dcFems()) << "current power production ac";
        if (connectionError(currentPowerProductionAc)) {

            return;
        }
        QByteArray data =
                currentPowerProductionAc->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }

        QVariant *var =
                new QVariant((this->getValueOfRequestedData(&jsonDoc)));
        addValueToThing(parentThing, meterThingClassId,
                        meterCurrentPowerProductionAcStateTypeId, var,
                        DOUBLE, 0);
        var = NULL;
        delete var;
    });

    // ProductionDcActivePower
    FemsNetworkReply *currentPowerProductionDc =
            connection->getFemsDataPoint(PRODUCTION_ACTIVE_DC_POWER);
    connect(currentPowerProductionDc, &FemsNetworkReply::finished, this,
            [this, currentPowerProductionDc, parentThing]() {
        qCDebug(dcFems()) << "Current Power Production DC";
        if (connectionError(currentPowerProductionDc)) {

            return;
        }
        QByteArray data =
                currentPowerProductionDc->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }
        QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
        addValueToThing(parentThing, meterThingClassId,
                        meterCurrentPowerProductionDcStateTypeId, var,
                        DOUBLE, 0);
        var = NULL;
        delete var;
    });
    // CurrentPower
    FemsNetworkReply *currentPower =
            connection->getFemsDataPoint(CONSUMPTION_ACTIVE_POWER);
    connect(currentPower, &FemsNetworkReply::finished, this,
            [this, currentPower, parentThing]() {
        qCDebug(dcFems()) << "Current Power";
        if (connectionError(currentPower)) {

            return;
        }
        QByteArray data = currentPower->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }
        QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
        addValueToThing(parentThing, meterThingClassId,
                        meterCurrentPowerStateTypeId, var, DOUBLE, 0);
        var = NULL;
        delete var;
    });
    // ProductionActiveEnergy
    FemsNetworkReply *totalEnergyProduced =
            connection->getFemsDataPoint(GRID_PRODUCTION_ACTIVE_ENERGY);
    connect(totalEnergyProduced, &FemsNetworkReply::finished, this,
            [this, totalEnergyProduced, parentThing]() {
        qCDebug(dcFems()) << "Total Energy Produced";
        if (connectionError(totalEnergyProduced)) {

            return;
        }
        QByteArray data = totalEnergyProduced->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }

        QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
        addValueToThing(parentThing, meterThingClassId,
                        meterTotalEnergyProducedStateTypeId, var, DOUBLE,
                        -3);
        var = NULL;
        delete var;
    });
    // ConsumptionActiveEnergy
    FemsNetworkReply *totalEnergyConsumed =
            connection->getFemsDataPoint(GRID_CONSUMPTION_ACTIVE_ENERGY);
    connect(totalEnergyConsumed, &FemsNetworkReply::finished, this,
            [this, totalEnergyConsumed, parentThing]() {
        qCDebug(dcFems()) << "Total Energy Consumed";
        if (connectionError(totalEnergyConsumed)) {

            return;
        }
        QByteArray data = totalEnergyConsumed->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }

        QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
        addValueToThing(parentThing, meterThingClassId,
                        meterTotalEnergyConsumedStateTypeId, var, DOUBLE,
                        -3);
        var = NULL;
        delete var;
    });
    // Grid BUY
    FemsNetworkReply *currentGridBuyEnergy =
            connection->getFemsDataPoint(GRID_BUY_ACTIVE_ENERGY);
    connect(currentGridBuyEnergy, &FemsNetworkReply::finished, this,
            [this, currentGridBuyEnergy, parentThing]() {
        qCDebug(dcFems()) << "Current Grid Buy Energy";
        if (connectionError(currentGridBuyEnergy)) {

            return;
        }
        QByteArray data = currentGridBuyEnergy->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }

        QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
        addValueToThing(parentThing, meterThingClassId,
                        meterCurrentGridBuyEnergyStateTypeId, var, DOUBLE,
                        -3);
        var = NULL;
        delete var;
    });
    // Grid SELL
    FemsNetworkReply *currentGridSellEnergy =
            connection->getFemsDataPoint(GRID_SELL_ACTIVE_ENERGY);
    connect(currentGridSellEnergy, &FemsNetworkReply::finished, this,
            [this, currentGridSellEnergy, parentThing]() {
        qCDebug(dcFems()) << "Current Grid Sell Energy";
        if (connectionError(currentGridSellEnergy)) {

            return;
        }
        QByteArray data = currentGridSellEnergy->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }

        QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
        addValueToThing(parentThing, meterThingClassId,
                        meterCurrentGridSellEnergyStateTypeId, var, DOUBLE,
                        -3);
        var = NULL;
        delete var;
    });

    // GridActivePowerL1
    FemsNetworkReply *currentPowerPhaseA =
            connection->getFemsDataPoint(GRID_ACTIVE_POWER_L1);
    connect(currentPowerPhaseA, &FemsNetworkReply::finished, this,
            [this, currentPowerPhaseA, parentThing]() {
        qCDebug(dcFems()) << "Current Power Phase A";
        if (connectionError(currentPowerPhaseA)) {

            return;
        }
        QByteArray data = currentPowerPhaseA->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }

        QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
        addValueToThing(parentThing, meterThingClassId,
                        meterCurrentPowerPhaseAStateTypeId, var, DOUBLE,
                        -3);
        var = NULL;
        delete var;
    });

    // GridActivePowerL2
    FemsNetworkReply *currentPowerPhaseB =
            connection->getFemsDataPoint(GRID_ACTIVE_POWER_L2);
    connect(currentPowerPhaseB, &FemsNetworkReply::finished, this,
            [this, currentPowerPhaseB, parentThing]() {
        qCDebug(dcFems()) << "Current Power Phase B";
        if (connectionError(currentPowerPhaseB)) {

            return;
        }
        QByteArray data = currentPowerPhaseB->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }

        QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
        addValueToThing(parentThing, meterThingClassId,
                        meterCurrentPowerPhaseBStateTypeId, var, DOUBLE,
                        -3);
        var = NULL;
        delete var;
    });

    // GridActivePowerL3
    FemsNetworkReply *currentPowerPhaseC =
            connection->getFemsDataPoint(GRID_ACTIVE_POWER_L3);
    connect(currentPowerPhaseC, &FemsNetworkReply::finished, this,
            [this, currentPowerPhaseC, parentThing]() {
        qCDebug(dcFems()) << "Current Power Phase C";
        if (connectionError(currentPowerPhaseC)) {

            return;
        }
        QByteArray data = currentPowerPhaseC->networkReply()->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // Check JSON Reply
        bool jsonE = jsonError(data);
        if (jsonE) {
            qCWarning(dcFems()) << "Meter: Failed to parse JSON data" << data
                                << ":" << error.errorString();

            return;
        }

        QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
        addValueToThing(parentThing, meterThingClassId,
                        meterCurrentPowerPhaseCStateTypeId, var, DOUBLE,
                        -3);
        var = NULL;
        delete var;
    });

    // HERE TEST CONNECTION! if Meter asymmetric -> check meter0 first -> if
    // normally connection available -> test meter0 if no conn. test meter1 up to
    // meter2 else -> just skip
    bool shouldNotSkip = !(this->meter == SKIP);
    if (shouldNotSkip) {
        QString PhaseA = this->meter + "/" + CURRENT_PHASE_1;
        QString PhaseB = this->meter + "/" + CURRENT_PHASE_2;
        QString PhaseC = this->meter + "/" + CURRENT_PHASE_3;
        QString Frequency = this->meter + "/" + FREQUENCY;
        // CurrentL1
        FemsNetworkReply *currentPhaseA = connection->getFemsDataPoint(PhaseA);
        connect(currentPhaseA, &FemsNetworkReply::finished, this,
                [this, currentPhaseA, parentThing]() {
            qCDebug(dcFems()) << "current Phase A";
            //TODO Possible error -> when not connected at all -> meterString changes either way! -> _sum/State == 3
            if (connectionError(currentPhaseA)) {
                changeMeterString();

                return;
            }
            QByteArray data = currentPhaseA->networkReply()->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

            // Check JSON Reply
            bool jsonE = this->jsonError(data);
            if (jsonE) {
                qCWarning(dcFems()) << "Meter: Failed to parse JSON data"
                                    << data << ":" << error.errorString();
                this->changeMeterString();

                return;
            }
            QVariant *var =
                    new QVariant((this->getValueOfRequestedData(&jsonDoc)));
            this->addValueToThing(parentThing, meterThingClassId,
                                  meterCurrentPhaseAStateTypeId, var, DOUBLE,
                                  -3);
            var = NULL;
            delete var;
        });
        // Current L2
        FemsNetworkReply *currentPhaseB = connection->getFemsDataPoint(PhaseB);
        connect(currentPhaseB, &FemsNetworkReply::finished, this,
                [this, currentPhaseB, parentThing]() {
            qCDebug(dcFems()) << "Current Phase B";
            if (connectionError(currentPhaseB)) {
                changeMeterString();

                return;
            }
            QByteArray data = currentPhaseB->networkReply()->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

            // Check JSON Reply
            bool jsonE = jsonError(data);
            if (jsonE) {
                qCWarning(dcFems()) << "Meter: Failed to parse JSON data"
                                    << data << ":" << error.errorString();
                changeMeterString();

                return;
            }

            QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
            addValueToThing(parentThing, meterThingClassId,
                            meterCurrentPhaseBStateTypeId, var, DOUBLE, -3);
            var = NULL;
            delete var;
        });
        // Current L3
        FemsNetworkReply *currentPhaseC = connection->getFemsDataPoint(PhaseC);
        connect(currentPhaseC, &FemsNetworkReply::finished, this,
                [this, currentPhaseC, parentThing]() {
            qCDebug(dcFems()) << "current Phase C";
            if (connectionError(currentPhaseC)) {
                this->changeMeterString();

                return;
            }
            QByteArray data = currentPhaseC->networkReply()->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

            // Check JSON Reply
            bool jsonE = jsonError(data);
            if (jsonE) {
                qCWarning(dcFems()) << "Meter: Failed to parse JSON data"
                                    << data << ":" << error.errorString();
                changeMeterString();

                return;
            }
            QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
            addValueToThing(parentThing, meterThingClassId,
                            meterCurrentPhaseCStateTypeId, var, DOUBLE, -3);
            var = NULL;
            delete var;
        });

        // Frequency
        FemsNetworkReply *frequency = connection->getFemsDataPoint(Frequency);
        connect(frequency, &FemsNetworkReply::finished, this,
                [this, frequency, parentThing]() {
            qCDebug(dcFems()) << "Frequency";
            if (connectionError(frequency)) {
                changeMeterString();

                return;
            }
            QByteArray data = frequency->networkReply()->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

            // Check JSON Reply
            bool jsonE = jsonError(data);
            if (jsonE) {
                qCWarning(dcFems()) << "Meter: Failed to parse JSON data"
                                    << data << ":" << error.errorString();
                changeMeterString();

                return;
            }
            QVariant *var = new QVariant((getValueOfRequestedData(&jsonDoc)));
            addValueToThing(parentThing, meterThingClassId,
                            meterFrequencyStateTypeId, var, DOUBLE, -3);
            var = NULL;
            delete var;
        });
    }
    qCDebug(dcFems()) << "METERS DONE";
}

bool IntegrationPluginFems::connectionError(FemsNetworkReply *reply) {
    return reply->networkReply()->error() != QNetworkReply::NoError;
}

bool IntegrationPluginFems::jsonError(QByteArray data) {
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    return error.error != QJsonParseError::NoError;
}
// JSON of FEMS contains value behind "value"
QString IntegrationPluginFems::getValueOfRequestedData(QJsonDocument *json) {
    return json->toVariant().toMap().value(DATA_ACCESS_STRING_FEMS).toString();
}

void IntegrationPluginFems::addValueToThing(Thing *parentThing,
                                            ThingClassId identifier,
                                            StateTypeId stateName,
                                            const QVariant *value,
                                            ValueType valueType, int scale) {
    qCDebug(dcFems()) << "addValueToThing is called with parentThing etc";
    Thing *child = this->GetThingByParentAndClassId(parentThing, identifier);
    if (child != nullptr) {
        this->addValueToThing(child, stateName, value, valueType, scale);
    }
}

void IntegrationPluginFems::addValueToThing(Thing *childThing,
                                            StateTypeId stateName,
                                            const QVariant *value,
                                            ValueType valueType, int scale) {
    qCDebug(dcFems()) << "Add Value to thing called";

    qCDebug(dcFems()) << "Adding Value : " << value->toString()
            << " to child: " << childThing->id() << " with state: " << stateName;
    // void setStateValue(const QString &stateName, const QVariant &value);
    if (value != nullptr && value->toString() != "null" &&
            value->toString() != "") {
        //TODO Low Prio -> Switch case cast enum to int ValueType::Double etc
        if (valueType == DOUBLE) {
            double doubleValue = (value->toDouble()) * pow(10, scale);
            qCDebug(dcFems()) << "Value will be added: " << doubleValue;
            childThing->setStateValue(stateName, doubleValue);
        } else if (valueType == QSTRING) {
            QString myString = value->toString();
            qCDebug(dcFems()) << "Value will be added: " << myString;
            childThing->setStateValue(stateName, myString);
        } else if (valueType == MY_BOOLEAN) {
            bool booleanValue = value->toBool();
            qCDebug(dcFems()) << "Value will be added: " << booleanValue;
            childThing->setStateValue(stateName, booleanValue);
        } else if (valueType == MY_INT) {
            int intValue = (value->toInt()) * pow(10, scale);
            qCDebug(dcFems()) << "Value will be added: " << intValue;
            childThing->setStateValue(stateName, intValue);
        }
    }
}

void IntegrationPluginFems::changeMeterString() {

 //ATM Try METER_0 -> Meter_1 and then Meter_2 again and again and again
  if (this->meter == METER_0) {
    this->meter = METER_1;
  } else if (this->meter == METER_1) {
    this->meter = METER_2;
  } else if (this->meter == METER_2) {
    this->meter = METER_0;
  } else {
    this->meter = SKIP;
  }

    qCDebug(dcFems()) << "ChangeMeter in Future";
}

Thing *
IntegrationPluginFems::GetThingByParentAndClassId(Thing *parentThing,
                                                  ThingClassId identifier) {
    qCDebug(dcFems()) << "Calling Thing By Parent And Class Id";
    qCDebug(dcFems()) << "parentThing Id: " << parentThing->id();
    qCDebug(dcFems()) << "thingClass identifier" << identifier;
    qCDebug(dcFems()) << "general myThings size: " << myThings().length();
    qCDebug(dcFems()) << "ThingsSize without classId"
            << myThings().filterByParentId(parentThing->id()).size();
    if (myThings().length() > 0) {
        qCDebug(dcFems()) << "Id of first My Things" << myThings().first()->id();
        qCDebug(dcFems()) << "Parent ID: " << parentThing->id();
    }
    Things valueToAddThings = myThings()
            .filterByParentId(parentThing->id())
            .filterByThingClassId(identifier);

    qCDebug(dcFems()) << "Things in my Things found: " << valueToAddThings.length();
    if (valueToAddThings.count() == 1) {
        qCDebug(dcFems()) << "Returning Thing: " << valueToAddThings.first()->id();
        return valueToAddThings.first();
    }
    return nullptr;
}

void IntegrationPluginFems::checkBatteryState(Thing *parentThing) {
    // ChargingState -> this should be done seperately when comparing charing and
    // discharging energy idle, charging, discharging
    qCDebug(dcFems()) << "Checking Battery State";
    Thing *thing = GetThingByParentAndClassId(parentThing, batteryThingClassId);
    if (thing != nullptr) {
        QVariant chargingEnergy =
                thing->stateValue(batteryChargingEnergyStateTypeId);
        QVariant dischargingEnergy =
                thing->stateValue(batteryDischarginEnergyStateTypeId);
        qCDebug(dcFems()) << "Getting stateValues";
        int charging = chargingEnergy.toInt();
        int discharging = dischargingEnergy.toInt();
        qCDebug(dcFems()) << "charging: " << charging << " discharging: " << discharging;
        //TODO: ASSUMPTION! + READ Doc. In Code
        if (charging != 0 && charging > discharging) {
            this->batteryState = "charging";
        } else if (discharging != 0 && discharging > charging) {
            this->batteryState = "discharging";
        } else if (discharging == charging) {
            this->batteryState = "idle";
        }

        QVariant var = this->batteryState;
        qCDebug(dcFems()) << "Setting Battery State: " << this->batteryState;
        this->addValueToThing(thing, batteryChargingStateStateTypeId, &var, QSTRING,
                              0);
    }
}
