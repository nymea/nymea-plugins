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

#include "plugininfo.h"
#include "integrationpluginfronius.h"
#include "plugintimer.h"
#include "network/networkaccessmanager.h"
#include "network/networkdevicediscovery.h"

#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QPointer>
#include <QDebug>

// Notes: Test IPs: 93.82.221.82 | 88.117.152.99

IntegrationPluginFronius::IntegrationPluginFronius(QObject *parent): IntegrationPlugin(parent)
{

}

void IntegrationPluginFronius::discoverThings(ThingDiscoveryInfo *info)
{
    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
        qCWarning(dcFronius()) << "Failed to discover network devices. The network device discovery is not available.";
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to discovery devices in your network."));
        return;
    }

    qCDebug(dcFronius()) << "Starting network discovery...";
    NetworkDeviceDiscoveryReply *discoveryReply = hardwareManager()->networkDeviceDiscovery()->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        ThingDescriptors descriptors;
        qCDebug(dcFronius()) << "Discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "devices";
        foreach (const NetworkDeviceInfo &networkDeviceInfo, discoveryReply->networkDeviceInfos()) {
            qCDebug(dcFronius()) << networkDeviceInfo;
            if (networkDeviceInfo.macAddress().isNull())
                continue;

            // Hostname or MAC manufacturer must include Fronius
            if (!(networkDeviceInfo.macAddressManufacturer().toLower().contains("fronius") || networkDeviceInfo.hostName().toLower().contains("fronius")))
                continue;

            QString title;
            if (networkDeviceInfo.hostName().isEmpty()) {
                title += networkDeviceInfo.address().toString();
            } else {
                title += networkDeviceInfo.address().toString() + " (" + networkDeviceInfo.hostName() + ")";
            }

            QString description;
            if (networkDeviceInfo.macAddressManufacturer().isEmpty()) {
                description = networkDeviceInfo.macAddress();
            } else {
                description = networkDeviceInfo.macAddress() + " (" + networkDeviceInfo.macAddressManufacturer() + ")";
            }

            ThingDescriptor descriptor(dataloggerThingClassId, title, description);

            // Check if we already have set up this device
            Things existingThings = myThings().filterByParam(dataloggerThingLoggerMacParamTypeId, networkDeviceInfo.macAddress());
            if (existingThings.count() == 1) {
                qCDebug(dcFronius()) << "This thing already exists in the system." << existingThings.first() << networkDeviceInfo;
                descriptor.setThingId(existingThings.first()->id());
            }

            ParamList params;
            params << Param(dataloggerThingLoggerHostParamTypeId, networkDeviceInfo.address().toString());
            params << Param(dataloggerThingLoggerMacParamTypeId, networkDeviceInfo.macAddress());
            descriptor.setParams(params);
            info->addThingDescriptor(descriptor);
        }
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginFronius::setupThing(ThingSetupInfo *info)
{
    qCDebug(dcFronius()) << "Setting up a new thing:" << info->thing()->name();

    Thing *thing = info->thing();

    if (thing->thingClassId() == dataloggerThingClassId) {
        //check if a data logger is already added with this IPv4Address
        foreach(FroniusLogger *logger, m_froniusLoggers.keys()){
            if(logger->hostAddress() == thing->paramValue(dataloggerThingLoggerHostParamTypeId).toString()){
                //this logger at this IPv4 address is already added
                qCWarning(dcFronius()) << "thing at " << thing->paramValue(dataloggerThingLoggerHostParamTypeId).toString() << " already added!";
                info->finish(Thing::ThingErrorThingInUse);
                return;
            }
        }

        // Perform a HTTP request on the given IPv4Address to find things
        QUrl requestUrl;
        requestUrl.setScheme("http");
        requestUrl.setHost(thing->paramValue(dataloggerThingLoggerHostParamTypeId).toString());
        requestUrl.setPath("/solar_api/GetAPIVersion.cgi");

        qCDebug(dcFronius()) << "Search at address" << requestUrl.toString();

        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(requestUrl));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [this, info, thing, reply] {
            QByteArray data = reply->readAll();
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcFronius()) << "Network request error:" << reply->error() << reply->errorString() << reply->url();
                info->finish(Thing::ThingErrorHardwareNotAvailable, tr("Device not reachable"));
                return;
            }

            // Convert the rawdata to a JSON document
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcFronius()) << "Failed to parse JSON data" << data << ":" << error.errorString() << data;
                info->finish(Thing::ThingErrorHardwareFailure, tr("Please try again"));
                return;
            }

            QVariantMap versionResponseMap = jsonDoc.toVariant().toMap();

            // Knwon version with broken JSON API
            if (versionResponseMap.value("CompatibilityRange").toString() == "1.6-2") {
                qCWarning(dcFronius()) << "The Fronius data logger has a version which is known to have a broken JSON API firmware.";
                 info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The firmware version 1.6-2 of this Fronius data logger has a broken API. Please update your Fronius device."));
                 return;
            }

            FroniusLogger *newLogger = new FroniusLogger(thing, this);
            newLogger->setBaseUrl(versionResponseMap.value("BaseURL").toString());
            newLogger->setHostAddress(thing->paramValue(dataloggerThingLoggerHostParamTypeId).toString());
            m_froniusLoggers.insert(newLogger, thing);

            info->finish(Thing::ThingErrorNoError);
        });

    } else if ((thing->thingClassId() == inverterThingClassId) ||
               (thing->thingClassId() == meterThingClassId)    ||
               (thing->thingClassId() == storageThingClassId)) {

        Thing *loggerThing = myThings().findById(thing->parentId());
        if (!loggerThing) {
            qCWarning(dcFronius()) << "Could not find Logger Thing for thing " << thing->name();
            return info->finish(Thing::ThingErrorHardwareNotAvailable, "Please try again");
        }

        if (!loggerThing->setupComplete()) {
            //wait for the parent to finish the setup process
            connect(loggerThing, &Thing::setupStatusChanged, info, [this, info, loggerThing] {
                if (loggerThing->setupComplete()) {
                    setupChild(info, loggerThing);
                }
            });
            return;
        }
        setupChild(info, loggerThing);
    } else {
        Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginFronius::postSetupThing(Thing *thing)
{
    qCDebug(dcFronius()) << "Post setup" << thing->name();

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this]() {
            foreach (Thing *logger, m_froniusLoggers)
                updateThingStates(logger);

            foreach (Thing *inverter, m_froniusInverters)
                updateThingStates(inverter);

            foreach (Thing *meter, m_froniusMeters)
                updateThingStates(meter);

            foreach (Thing *storage, m_froniusStorages)
                updateThingStates(storage);
        });
    }

    if (thing->thingClassId() == dataloggerThingClassId) {
        searchNewThings(m_froniusLoggers.key(thing));
        thing->setStateValue(dataloggerConnectedStateTypeId, true);
        updateThingStates(thing);
    } else if ((thing->thingClassId() == inverterThingClassId) ||
               (thing->thingClassId() == meterThingClassId)    ||
               (thing->thingClassId() == storageThingClassId)) {
        updateThingStates(thing);
    } else {
        Q_ASSERT_X(false, "postSetupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginFronius::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == dataloggerThingClassId) {
        FroniusLogger *logger = m_froniusLoggers.key(thing);
        m_froniusLoggers.remove(logger);
        logger->deleteLater();
    } else if (thing->thingClassId() == inverterThingClassId) {
        FroniusInverter *inverter = m_froniusInverters.key(thing);
        m_froniusInverters.remove(inverter);
        inverter->deleteLater();
    } else if (thing->thingClassId() == meterThingClassId) {
        FroniusMeter *meter = m_froniusMeters.key(thing);
        m_froniusMeters.remove(meter);
        meter->deleteLater();
    } else if (thing->thingClassId() == storageThingClassId) {
        FroniusStorage *storage = m_froniusStorages.key(thing);
        m_froniusStorages.remove(storage);
        storage->deleteLater();
    } else {
        Q_ASSERT_X(false, "thingRemoved", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}


void IntegrationPluginFronius::executeAction(ThingActionInfo *info)
{
    Action action = info->action();
    Thing *thing = info->thing();
    qCDebug(dcFronius()) << "Execute action" << thing->name() << action.actionTypeId().toString();

    if (thing->thingClassId() == dataloggerThingClassId) {
        if (action.actionTypeId() == dataloggerSearchDevicesActionTypeId) {
            searchNewThings(m_froniusLoggers.key(thing));
            info->finish(Thing::ThingErrorNoError);
        } else {
            Q_ASSERT_X(false, "executeAction", QString("Unhandled action: %1").arg(action.actionTypeId().toString()).toUtf8());
        }
    } else {
        Q_ASSERT_X(false, "executeAction", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginFronius::updateThingStates(Thing *thing)
{
    qCDebug(dcFronius()) << "Update thing values for" << thing->name();

    if (thing->thingClassId() == inverterThingClassId) {
        qCDebug(dcFronius()) << "Update inverter" << m_froniusInverters.key(thing)->updateUrl();
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(m_froniusInverters.key(thing)->updateUrl()));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, thing, [this, thing, reply]() {

            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcFronius()) << "Network request error:" << reply->error() << reply->errorString() << reply->request().url();
                return;
            }
            QByteArray data = reply->readAll();
            if (m_froniusInverters.values().contains(thing)){ // check if thing was not removed before reply was received
                m_froniusInverters.key(thing)->updateThingInfo(data);
            }
        });
        QNetworkReply *next_reply = hardwareManager()->networkManager()->get(QNetworkRequest(m_froniusInverters.key(thing)->activityUrl()));
        connect(next_reply, &QNetworkReply::finished, next_reply, &QNetworkReply::deleteLater);
        connect(next_reply, &QNetworkReply::finished, thing, [this, thing, next_reply] {
            if (next_reply->error() != QNetworkReply::NoError) {
                qCWarning(dcFronius()) << "Network request error:" << next_reply->error() << next_reply->errorString() << next_reply->request().url();
                return;
            }
            QByteArray data = next_reply->readAll();
            if (m_froniusInverters.values().contains(thing)){ // check if thing was not removed before reply was received
                m_froniusInverters.key(thing)->updateActivityInfo(data);
            }
        });
    } else if (thing->thingClassId() == dataloggerThingClassId) {
        qCDebug(dcFronius()) << "Update logger" << m_froniusLoggers.key(thing)->updateUrl();
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(m_froniusLoggers.key(thing)->updateUrl()));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, thing, [this, thing, reply]() {

            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcFronius()) << "Network request error:" << reply->error() << reply->errorString() << reply->request().url();
                return;
            }
            QByteArray data = reply->readAll();
            if (m_froniusLoggers.values().contains(thing)){ // check if thing was not removed before reply was received
                m_froniusLoggers.key(thing)->updateThingInfo(data);
            }
        });

    } else if (thing->thingClassId() == meterThingClassId) {
        qCDebug(dcFronius()) << "Update meter" << m_froniusMeters.key(thing)->updateUrl();
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(m_froniusMeters.key(thing)->updateUrl()));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, thing, [this, thing, reply]() {
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcFronius()) << "Network request error:" << reply->error() << reply->errorString() << reply->request().url();
                return;
            }
            QByteArray data = reply->readAll();
            if (m_froniusMeters.values().contains(thing)){ // check if thing was not removed before reply was received
                m_froniusMeters.key(thing)->updateThingInfo(data);
            }
        });

    } else if (thing->thingClassId() == storageThingClassId) {
        qCDebug(dcFronius()) << "Update storage" << m_froniusStorages.key(thing)->updateUrl();
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(m_froniusStorages.key(thing)->updateUrl()));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, thing, [this, thing, reply]() {

            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcFronius()) << "Network request error:" << reply->error() << reply->errorString() << reply->request().url();
                return;
            }
            QByteArray data = reply->readAll();
            if (m_froniusStorages.values().contains(thing)){ // check if thing was not removed before reply was received
                m_froniusStorages.key(thing)->updateThingInfo(data);
            }
        });
        QNetworkReply *next_reply = hardwareManager()->networkManager()->get(QNetworkRequest(m_froniusStorages.key(thing)->activityUrl()));
        connect(next_reply, &QNetworkReply::finished, next_reply, &QNetworkReply::deleteLater);
        connect(next_reply, &QNetworkReply::finished, thing, [this, thing, next_reply]() {

            if (next_reply->error() != QNetworkReply::NoError) {
                qCWarning(dcFronius()) << "Network request error:" << next_reply->error() << next_reply->errorString() << next_reply->request().url();
                return;
            }
            QByteArray data = next_reply->readAll();
            if(m_froniusStorages.values().contains(thing)){ // check if thing was not removed before reply was received
                m_froniusStorages.key(thing)->updateActivityInfo(data);
            }
        });
    } else {
        Q_ASSERT_X(false, "updateThingState", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginFronius::searchNewThings(FroniusLogger *logger)
{
    QUrl url; QUrlQuery query;
    query.addQueryItem("DeviceClass", "System");
    url.setScheme("http");
    url.setHost(logger->hostAddress());
    url.setPath(logger->baseUrl() + "GetActiveDeviceInfo.cgi");
    url.setQuery(query);

    qCDebug(dcFronius()) << "Searching new things at address" << url.toString();
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, logger, [this, logger, reply]() {

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcFronius()) << "Network request error:" << reply->error() << reply->errorString() << reply->request().url();
            return;
        }

        Thing *loggerThing = m_froniusLoggers.value(logger);
        if (!loggerThing)
            return;

        // Convert the rawdata to a json document
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcFronius()) << "Failed to parse JSON data" << data << ":" << error.errorString();
            return;
        }

        // Parse the data for thing information
        QList<ThingDescriptor> thingDescriptors;

        // Check reply information
        QVariantMap bodyMap = jsonDoc.toVariant().toMap().value("Body").toMap();
        qCDebug(dcFronius()) << "System:" << qUtf8Printable(QJsonDocument::fromVariant(bodyMap).toJson(QJsonDocument::Indented));

        // Parse reply for inverters at the host address
        QVariantMap inverterMap = bodyMap.value("Data").toMap().value("Inverter").toMap();
        foreach (const QString &inverterId, inverterMap.keys()) {
            //check if thing already connected to logger
            if (!thingExists(inverterThingIdParamTypeId, inverterId)) {
                QString thingName = loggerThing->name() + " Inverter " + inverterId;
                ThingDescriptor descriptor(inverterThingClassId, thingName, "Fronius Solar Inverter", loggerThing->id());
                ParamList params;
                params.append(Param(inverterThingIdParamTypeId, inverterId));
                descriptor.setParams(params);
                thingDescriptors.append(descriptor);
            }
        }

        // parse reply for meter things at the host address
        QVariantMap meterMap = bodyMap.value("Data").toMap().value("Meter").toMap();
        foreach (const QString &meterId, meterMap.keys()) {
            //check if thing already connected to logger
            if (!thingExists(meterThingIdParamTypeId, meterId)) {
                // get meter infos
                ///solar_api/v1/GetMeterRealtimeData.cgi?Scope=Device&DeviceId=0
                QUrl requestUrl;
                requestUrl.setScheme("http");
                requestUrl.setHost(logger->hostAddress());
                requestUrl.setPath(logger->baseUrl() + "GetMeterRealtimeData.cgi");
                QUrlQuery query;
                query.addQueryItem("Scope", "Device");
                query.addQueryItem("DeviceId", meterId);
                requestUrl.setQuery(query);

                qCDebug(dcFronius()) << "Get meter information before setup";
                QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(requestUrl));
                connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                connect(reply, &QNetworkReply::finished, this, [=]() {
                    if (reply->error() != QNetworkReply::NoError) {
                        qCWarning(dcFronius()) << "Network request error:" << reply->error() << reply->errorString();
                        return;
                    }

                    QByteArray data = reply->readAll();
                    QJsonParseError error;
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                    if (error.error != QJsonParseError::NoError) {
                        qCWarning(dcFronius()) << "Failed to parse JSON data" << data << ":" << error.errorString();
                        return;
                    }

                    QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap();

                    QString thingName;
                    QString serialNumber;
                    if (dataMap.contains("Details")) {
                        QVariantMap details = dataMap.value("Details").toMap();
                        thingName = details.value("Manufacturer", "Fronius").toString() + " " + details.value("Model", "Smart Meter").toString();
                        serialNumber = details.value("Serial").toString();
                    } else {
                        thingName = loggerThing->name() + " Meter " + meterId;
                    }

                    ThingDescriptor descriptor(meterThingClassId, thingName, QString(), loggerThing->id());
                    ParamList params;
                    params.append(Param(meterThingIdParamTypeId, meterId));
                    params.append(Param(meterThingSerialNumberParamTypeId, serialNumber));
                    descriptor.setParams(params);
                    emit autoThingsAppeared(ThingDescriptors() << descriptor);
                });
            }
        }

        // parse reply for storage things at the host address
        QVariantMap storageMap = bodyMap.value("Data").toMap().value("Storage").toMap();
        foreach (const QString &storageId, storageMap.keys()) {
            //check if thing already connected to logger
            if (!thingExists(storageThingIdParamTypeId, storageId)) {
                QUrlQuery query;
                QUrl requestUrl;
                requestUrl.setScheme("http");
                requestUrl.setHost(logger->hostAddress());
                requestUrl.setPath(logger->baseUrl() + "GetStorageRealtimeData.cgi");
                query.addQueryItem("Scope","Device");
                query.addQueryItem("DeviceId", storageId);
                requestUrl.setQuery(query);

                qCDebug(dcFronius()) << "Get storage information before setup" << requestUrl.toString();
                QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(requestUrl));
                connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
                connect(reply, &QNetworkReply::finished, this, [=]() {
                    if (reply->error() != QNetworkReply::NoError) {
                        qCWarning(dcFronius()) << "Network request error:" << reply->error() << reply->errorString();
                        return;
                    }

                    // Convert the rawdata to a json document
                    QByteArray data = reply->readAll();
                    QJsonParseError error;
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                    if (error.error != QJsonParseError::NoError) {
                        qCWarning(dcFronius()) << "Failed to parse JSON data" << data << ":" << error.errorString();
                        return;
                    }

                    QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap().value("Controller").toMap();

                    QString thingName;
                    QString serialNumber;
                    if (dataMap.contains("Details")) {
                        QVariantMap details = dataMap.value("Details").toMap();
                        thingName = details.value("Manufacturer", "Fronius").toString() + " " + details.value("Model", "Energy Storage").toString();
                        serialNumber = details.value("Serial").toString();
                    } else {
                        thingName = loggerThing->name() + " Storage " + storageId;
                    }

                    ThingDescriptor descriptor(storageThingClassId, thingName, QString(), loggerThing->id());
                    ParamList params;
                    params.append(Param(storageThingIdParamTypeId, storageId));
                    params.append(Param(storageThingSerialNumberParamTypeId, serialNumber));
                    descriptor.setParams(params);
                    emit autoThingsAppeared(ThingDescriptors() << descriptor);

                });
            }
        }

        QVariantMap ohmpilotMap = bodyMap.value("Data").toMap().value("Ohmpilot").toMap();
        foreach (QString ohmpilotId, ohmpilotMap.keys()) {
            qCDebug(dcFronius()) << "Unhandled device Ohmpilot" << ohmpilotId;
        }

        QVariantMap sensorCardMap = bodyMap.value("Data").toMap().value("SensorCard").toMap();
        foreach (QString sensorCardId, sensorCardMap.keys()) {
            qCDebug(dcFronius()) << "Unhandled device SensorCard" << sensorCardId;
        }

        QVariantMap stringControlMap = bodyMap.value("Data").toMap().value("StringControl").toMap();
        foreach (QString stringControlId, stringControlMap.keys()) {
            qCDebug(dcFronius()) << "Unhandled device StringControl" << stringControlId;
        }

        if (!thingDescriptors.empty()) {
            emit autoThingsAppeared(thingDescriptors);
            thingDescriptors.clear();
        }
    });
}

bool IntegrationPluginFronius::thingExists(const ParamTypeId &thingParamId, QString thingId)
{
    foreach (Thing *thing, myThings()) {
        if (thing->paramValue(thingParamId).toString() == thingId) {
            return true;
        }
    }
    return false;
}

void IntegrationPluginFronius::setupChild(ThingSetupInfo *info, Thing *loggerThing)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == inverterThingClassId) {
        FroniusInverter *newInverter = new FroniusInverter(thing,this);
        newInverter->setDeviceId(thing->paramValue(inverterThingIdParamTypeId).toString());
        newInverter->setBaseUrl(m_froniusLoggers.key(loggerThing)->baseUrl());
        newInverter->setHostAddress(m_froniusLoggers.key(loggerThing)->hostAddress());

        // Get inverter unique ID
        QUrl requestUrl;
        requestUrl.setScheme("http");
        requestUrl.setHost(newInverter->hostAddress());
        requestUrl.setPath(newInverter->baseUrl() + "GetInverterInfo.cgi");

        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(requestUrl));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [this, info, newInverter, reply]() {

            QByteArray data = reply->readAll();

            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcFronius()) << "Network request error:" << reply->error() << reply->errorString();
                info->finish(Thing::ThingErrorHardwareNotAvailable, "Device not reachable");
                return;
            }

            // Convert the rawdata to a json document
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcFronius()) << "Failed to parse JSON data" << data << ":" << error.errorString();
                info->finish(Thing::ThingErrorHardwareNotAvailable, "Please try again");
                return;
            }

            // Check reply information
            QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap();
            // check for thing id in reply
            if (dataMap.contains(newInverter->deviceId())) {
                qCDebug(dcFronius()) << "Found Thing with unique:" << dataMap.value(newInverter->deviceId()).toMap().value("UniqueID").toString();
                newInverter->setUniqueId(dataMap.value(newInverter->deviceId()).toMap().value("UniqueID").toString());
                qCDebug(dcFronius()) << "Stored unique ID:" << newInverter->uniqueId();
            }

            m_froniusInverters.insert(newInverter, info->thing());
            info->finish(Thing::ThingErrorNoError);
        });

    } else if (thing->thingClassId() == meterThingClassId) {
        FroniusMeter *newMeter = new FroniusMeter(thing, this);;
        newMeter->setDeviceId(thing->paramValue(meterThingIdParamTypeId).toString());
        newMeter->setBaseUrl(m_froniusLoggers.key(loggerThing)->baseUrl());
        newMeter->setHostAddress(m_froniusLoggers.key(loggerThing)->hostAddress());

        m_froniusMeters.insert(newMeter, thing);
        info->finish(Thing::ThingErrorNoError);

    } else if (thing->thingClassId() == storageThingClassId) {
        FroniusStorage *newStorage = new FroniusStorage(thing, this);
        newStorage->setDeviceId(thing->paramValue(storageThingIdParamTypeId).toString());
        newStorage->setBaseUrl(m_froniusLoggers.key(loggerThing)->baseUrl());
        newStorage->setHostAddress(m_froniusLoggers.key(loggerThing)->hostAddress());

        m_froniusStorages.insert(newStorage, info->thing());
        info->finish(Thing::ThingErrorNoError);
    }
}
