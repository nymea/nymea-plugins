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

#include "devicepluginnetatmo.h"
#include "devices/device.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

#include <QDebug>
#include <QUrlQuery>
#include <QJsonDocument>

DevicePluginNetatmo::DevicePluginNetatmo()
{

}

DevicePluginNetatmo::~DevicePluginNetatmo()
{

}

void DevicePluginNetatmo::init()
{

}

void DevicePluginNetatmo::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == netatmoConnectionDeviceClassId) {
        qCDebug(dcNetatmo) << "Setup netatmo connection" << device->name() << device->params();

        if (!m_pluginTimer) {
            m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(600);
            connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginNetatmo::onPluginTimer);
        }

        OAuth2 *authentication = new OAuth2("561c015d49c75f0d1cce6e13", "GuvKkdtu7JQlPD47qTTepRR9hQ0CUPAj4Tae3Ohcq", this);
        authentication->setUrl(QUrl("https://api.netatmo.net/oauth2/token"));
        authentication->setUsername(device->paramValue(netatmoConnectionDeviceUsernameParamTypeId).toString());
        authentication->setPassword(device->paramValue(netatmoConnectionDevicePasswordParamTypeId).toString());
        authentication->setScope("read_station read_thermostat write_thermostat");
        m_authentications.insert(authentication, device);

        // Update device connected state based on OAuth connected state
        connect(authentication, &OAuth2::authenticationChanged, device, [this, device, authentication](){
            device->setStateValue(netatmoConnectionConnectedStateTypeId, authentication->authenticated());
            if (authentication->authenticated()) {
                refreshData(device, authentication->token());
            }
        });

        authentication->startAuthentication();

        // Report device setup finished when authentication reports success
        connect(authentication, &OAuth2::authenticationChanged, info, [info, authentication](){
            if (!authentication->authenticated()) {
                authentication->deleteLater();
                info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("Error logging in to Netatmo server."));
                return;
            }

            info->finish(Device::DeviceErrorNoError);
        });

        return;

    } else if (device->deviceClassId() == indoorDeviceClassId) {
        qCDebug(dcNetatmo) << "Setup netatmo indoor base station" << device->params();
        NetatmoBaseStation *indoor = new NetatmoBaseStation(device->paramValue(indoorDeviceNameParamTypeId).toString(),
                                                            device->paramValue(indoorDeviceMacParamTypeId).toString(),
                                                            this);

        m_indoorDevices.insert(indoor, device);
        connect(indoor, SIGNAL(statesChanged()), this, SLOT(onIndoorStatesChanged()));

        return info->finish(Device::DeviceErrorNoError);
    } else if (device->deviceClassId() == outdoorDeviceClassId) {
        qCDebug(dcNetatmo) << "Setup netatmo outdoor module" << device->params();


        // Migrate device parameters after changing param type UUIDs in 0.14.
        QMap<QString, ParamTypeId> migrationMap;
        migrationMap.insert("a97d256c-e159-4aa0-bc71-6bd7cd0688b3", outdoorDeviceNameParamTypeId);
        migrationMap.insert("157d470a-e579-4d0e-b879-6b5bfa8e34ae", outdoorDeviceMacParamTypeId);

        ParamList migratedParams;
        foreach (const Param &oldParam, device->params()) {
            QString oldId = oldParam.paramTypeId().toString();
            oldId.remove(QRegExp("[{}]"));
            if (migrationMap.contains(oldId)) {
                ParamTypeId newId = migrationMap.value(oldId);
                QVariant oldValue = oldParam.value();
                qCDebug(dcNetatmo()) << "Migrating netatmo outdoor station param:" << oldId << "->" << newId << ":" << oldValue;
                Param newParam(newId, oldValue);
                migratedParams << newParam;
            } else {
                migratedParams << oldParam;
            }
        }
        device->setParams(migratedParams);
        // Migration done


        NetatmoOutdoorModule *outdoor = new NetatmoOutdoorModule(device->paramValue(outdoorDeviceNameParamTypeId).toString(),
                                                                 device->paramValue(outdoorDeviceMacParamTypeId).toString(),
                                                                 device->paramValue(outdoorDeviceBaseStationParamTypeId).toString(),
                                                                 this);

        m_outdoorDevices.insert(outdoor, device);
        connect(outdoor, SIGNAL(statesChanged()), this, SLOT(onOutdoorStatesChanged()));

        return info->finish(Device::DeviceErrorNoError);
    }

    qCWarning(dcNetatmo()) << "Unhandled device class in setupDevice";
}

void DevicePluginNetatmo::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == netatmoConnectionDeviceClassId) {
        OAuth2 * authentication = m_authentications.key(device);
        m_authentications.remove(authentication);
        authentication->deleteLater();
    } else if (device->deviceClassId() == indoorDeviceClassId) {
        NetatmoBaseStation *indoor = m_indoorDevices.key(device);
        m_indoorDevices.remove(indoor);
        indoor->deleteLater();
    } else if (device->deviceClassId() == outdoorDeviceClassId) {
        NetatmoOutdoorModule *outdoor = m_outdoorDevices.key(device);
        m_outdoorDevices.remove(outdoor);
        outdoor->deleteLater();
    }

    if (myDevices().isEmpty() && m_pluginTimer) {
        m_pluginTimer->deleteLater();
        m_pluginTimer = nullptr;
    }
}

void DevicePluginNetatmo::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == indoorDeviceClassId) {
        QString stationId = device->paramValue(indoorDeviceMacParamTypeId).toString();
        if (m_indoorStationInitData.contains(stationId) && m_indoorDevices.values().contains(device)) {
            m_indoorDevices.key(device)->updateStates(m_indoorStationInitData.take(stationId));
        }
    } else if (device->deviceClassId() == outdoorDeviceClassId) {
        QString stationId = device->paramValue(outdoorDeviceMacParamTypeId).toString();
        if (m_outdoorStationInitData.contains(stationId) && m_outdoorDevices.values().contains(device)) {
            m_outdoorDevices.key(device)->updateStates(m_outdoorStationInitData.take(stationId));
        }
    }
}

void DevicePluginNetatmo::refreshData(Device *device, const QString &token)
{
    QUrlQuery query;
    query.addQueryItem("access_token", token);

    QUrl url("https://api.netatmo.com/api/getstationsdata");
    url.setQuery(query);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &DevicePluginNetatmo::onNetworkReplyFinished);

    m_refreshRequest.insert(reply, device);
}

void DevicePluginNetatmo::processRefreshData(const QVariantMap &data, Device *connectionDevice)
{
    // Process the data
    if (data.contains("body")) {
        // check devices
        if (data.value("body").toMap().contains("devices")) {
            QVariantList deviceList = data.value("body").toMap().value("devices").toList();
            // check devices
            foreach (QVariant deviceVariant, deviceList) {
                QVariantMap deviceMap = deviceVariant.toMap();
                // we support currently only NAMain devices
                if (deviceMap.value("type").toString() == "NAMain") {
                    Device *indoorDevice = findIndoorDevice(deviceMap.value("_id").toString());
                    // check if we have to create the device (auto)
                    if (!indoorDevice) {
                        DeviceDescriptor descriptor(indoorDeviceClassId, deviceMap.value("module_name").toString(), deviceMap.value("station_name").toString(), connectionDevice->id());
                        ParamList params;
                        params.append(Param(indoorDeviceNameParamTypeId, deviceMap.value("station_name").toString()));
                        params.append(Param(indoorDeviceMacParamTypeId, deviceMap.value("_id").toString()));
                        descriptor.setParams(params);
                        m_indoorStationInitData.insert(deviceMap.value("_id").toString(), deviceMap);
                        emit autoDevicesAppeared({descriptor});
                    } else {
                        if (m_indoorDevices.values().contains(indoorDevice)) {
                            m_indoorDevices.key(indoorDevice)->updateStates(deviceMap);
                        }
                    }
                }

                // check modules
                if (deviceMap.contains("modules")) {
                    QVariantList modulesList = deviceMap.value("modules").toList();
                    foreach (QVariant moduleVariant, modulesList) {
                        QVariantMap moduleMap = moduleVariant.toMap();
                        // we support currently only NAModule1
                        if (moduleMap.value("type").toString() == "NAModule1") {
                            Device *outdoorDevice = findOutdoorDevice(moduleMap.value("_id").toString());

                            // check if we have to create the device (auto)
                            if (!outdoorDevice) {
                                DeviceDescriptor descriptor(outdoorDeviceClassId, "Outdoor Module", moduleMap.value("module_name").toString(), connectionDevice->id());
                                ParamList params;
                                params.append(Param(outdoorDeviceNameParamTypeId, moduleMap.value("module_name").toString()));
                                params.append(Param(outdoorDeviceMacParamTypeId, moduleMap.value("_id").toString()));
                                params.append(Param(outdoorDeviceBaseStationParamTypeId, deviceMap.value("_id").toString()));
                                descriptor.setParams(params);
                                m_outdoorStationInitData.insert(moduleMap.value("_id").toString(), moduleMap);
                                emit autoDevicesAppeared({descriptor});
                            } else {
                                if (m_outdoorDevices.values().contains(outdoorDevice)) {
                                    m_outdoorDevices.key(outdoorDevice)->updateStates(moduleMap);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

Device *DevicePluginNetatmo::findIndoorDevice(const QString &macAddress)
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == indoorDeviceClassId) {
            if (device->paramValue(indoorDeviceMacParamTypeId).toString() == macAddress) {
                return device;
            }
        }
    }
    return nullptr;
}

Device *DevicePluginNetatmo::findOutdoorDevice(const QString &macAddress)
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == outdoorDeviceClassId) {
            if (device->paramValue(outdoorDeviceMacParamTypeId).toString() == macAddress) {
                return device;
            }
        }
    }
    return nullptr;
}

void DevicePluginNetatmo::onPluginTimer()
{
    foreach (OAuth2 *authentication, m_authentications.keys()) {
        if (authentication->authenticated()) {
            refreshData(m_authentications.value(authentication), authentication->token());
        } else {
            authentication->startAuthentication();
        }
    }
}

void DevicePluginNetatmo::onNetworkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    // update values request
    if (m_refreshRequest.keys().contains(reply)) {
        Device *device = m_refreshRequest.take(reply);

        // check HTTP status code
        if (status != 200) {
            qCWarning(dcNetatmo) << "Device list reply HTTP error:" << status << reply->errorString();
            device->setStateValue(netatmoConnectionConnectedStateTypeId, false);
            return;
        }

        // check JSON file
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcNetatmo) << "Device list reply JSON error:" << error.errorString();
            return;
        }

        qCDebug(dcNetatmo) << qUtf8Printable(jsonDoc.toJson());
        processRefreshData(jsonDoc.toVariant().toMap(), device);
    }
}

void DevicePluginNetatmo::onAuthenticationChanged()
{

}

void DevicePluginNetatmo::onIndoorStatesChanged()
{
    NetatmoBaseStation *indoor = static_cast<NetatmoBaseStation *>(sender());
    Device *device = m_indoorDevices.value(indoor);

    device->setStateValue(indoorUpdateTimeStateTypeId, indoor->lastUpdate());
    device->setStateValue(indoorTemperatureStateTypeId, indoor->temperature());
    device->setStateValue(indoorTemperatureMinStateTypeId, indoor->minTemperature());
    device->setStateValue(indoorTemperatureMaxStateTypeId, indoor->maxTemperature());
    device->setStateValue(indoorPressureStateTypeId, indoor->pressure());
    device->setStateValue(indoorHumidityStateTypeId, indoor->humidity());
    device->setStateValue(indoorCo2StateTypeId, indoor->co2());
    device->setStateValue(indoorNoiseStateTypeId, indoor->noise());
    device->setStateValue(indoorWifiStrengthStateTypeId, indoor->wifiStrength());
}

void DevicePluginNetatmo::onOutdoorStatesChanged()
{
    NetatmoOutdoorModule *outdoor = static_cast<NetatmoOutdoorModule *>(sender());
    Device *device = m_outdoorDevices.value(outdoor);

    device->setStateValue(outdoorUpdateTimeStateTypeId, outdoor->lastUpdate());
    device->setStateValue(outdoorTemperatureStateTypeId, outdoor->temperature());
    device->setStateValue(outdoorTemperatureMinStateTypeId, outdoor->minTemperature());
    device->setStateValue(outdoorTemperatureMaxStateTypeId, outdoor->maxTemperature());
    device->setStateValue(outdoorHumidityStateTypeId, outdoor->humidity());
    device->setStateValue(outdoorSignalStrengthStateTypeId, outdoor->signalStrength());
    device->setStateValue(outdoorBatteryLevelStateTypeId, outdoor->battery());
    device->setStateValue(outdoorBatteryCriticalStateTypeId, outdoor->battery() < 10);
}


