/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
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

#include "deviceplugintado.h"
#include "devices/device.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

#include <QDebug>
#include <QUrlQuery>
#include <QJsonDocument>

DevicePluginTado::DevicePluginTado()
{

}

DevicePluginTado::~DevicePluginTado()
{

}

void DevicePluginTado::startPairing(DevicePairingInfo *info)
{
    info->finish(Device::DeviceErrorNoError, QT_TR_NOOP("Please enter the login credentials."));
}

void DevicePluginTado::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &password)
{
    Tado *tado = new Tado(hardwareManager()->networkManager(), username, this);
    connect(tado, &Tado::tokenReceived, this, &DevicePluginTado::onTokenReceived);
    connect(tado, &Tado::authenticationStatusChanged, this, &DevicePluginTado::onAuthenticationStatusChanged);
    connect(tado, &Tado::connectionChanged, this, &DevicePluginTado::onConnectionChanged);
    connect(tado, &Tado::homesReceived, this, &DevicePluginTado::onHomesReceived);
    connect(tado, &Tado::zonesReceived, this, &DevicePluginTado::onZonesReceived);
    connect(tado, &Tado::zoneStateReceived, this, &DevicePluginTado::onZoneStateReceived);
    m_unfinishedTadoAccounts.insert(info->deviceId(), tado);
    m_unfinishedDevicePairings.insert(info->deviceId(), info);
    tado->getToken(password);

    pluginStorage()->beginGroup(info->deviceId().toString());
    pluginStorage()->setValue("username", username);
    pluginStorage()->setValue("password", password);
    pluginStorage()->endGroup();
}

void DevicePluginTado::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == tadoConnectionDeviceClassId) {

        qCDebug(dcTado) << "Setup tado connection" << device->name() << device->params();
        pluginStorage()->beginGroup(device->id().toString());
        QString username = pluginStorage()->value("username").toString();
        QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();

        Tado *tado;
        if (m_unfinishedTadoAccounts.contains(device->id())) {
            tado = m_unfinishedTadoAccounts.take(device->id());
            m_tadoAccounts.insert(device->id(), tado);
            return info->finish(Device::DeviceErrorNoError);
        } else {
            tado = new Tado(hardwareManager()->networkManager(), username, this);
            connect(tado, &Tado::tokenReceived, this, &DevicePluginTado::onTokenReceived);
            connect(tado, &Tado::authenticationStatusChanged, this, &DevicePluginTado::onAuthenticationStatusChanged);
            connect(tado, &Tado::connectionChanged, this, &DevicePluginTado::onConnectionChanged);
            connect(tado, &Tado::homesReceived, this, &DevicePluginTado::onHomesReceived);
            connect(tado, &Tado::zonesReceived, this, &DevicePluginTado::onZonesReceived);
            connect(tado, &Tado::zoneStateReceived, this, &DevicePluginTado::onZoneStateReceived);
            tado->getToken(password);
            m_tadoAccounts.insert(device->id(), tado);
            m_asyncDeviceSetup.insert(tado, info);
            return;
        }

    } else if (device->deviceClassId() == zoneDeviceClassId) {
        qCDebug(dcTado) << "Setup tado thermostat" << device->params();
        return info->finish(Device::DeviceErrorNoError);
    }
    qCWarning(dcTado()) << "Unhandled device class in setupDevice";
}

void DevicePluginTado::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == tadoConnectionDeviceClassId) {
        Tado *tado = m_tadoAccounts.take(device->id());
        tado->deleteLater();

    } else if (device->deviceClassId() == zoneDeviceClassId) {

    }

    if (myDevices().isEmpty() && m_pluginTimer) {
        m_pluginTimer->deleteLater();
        m_pluginTimer = nullptr;
    }
}

void DevicePluginTado::postSetupDevice(Device *device)
{
    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginTado::onPluginTimer);
    }

    if (device->deviceClassId() == tadoConnectionDeviceClassId) {
        Tado *tado = m_tadoAccounts.value(device->id());
        device->setStateValue(tadoConnectionUserDisplayNameStateTypeId, tado->username());
        device->setStateValue(tadoConnectionLoggedInStateTypeId, true);
        device->setStateValue(tadoConnectionConnectedStateTypeId, true);

        tado->getHomes();

    } else if (device->deviceClassId() == zoneDeviceClassId) {
        if (m_tadoAccounts.contains(device->parentId())) {
            Tado *tado = m_tadoAccounts.value(device->parentId());
            tado->getZoneState(device->paramValue(zoneDeviceHomeIdParamTypeId).toString(), device->paramValue(zoneDeviceZoneIdParamTypeId).toString());
        }
    }
}

void DevicePluginTado::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == zoneDeviceClassId) {
        Tado *tado = m_tadoAccounts.value(device->parentId());
        if (!tado)
            return;
        QString homeId = device->paramValue(zoneDeviceHomeIdParamTypeId).toString();
        QString zoneId = device->paramValue(zoneDeviceZoneIdParamTypeId).toString();
        if (action.actionTypeId() == zoneModeActionTypeId) {

            if (action.param(zoneModeActionModeParamTypeId).value().toString() == "Tado") {
                tado->deleteOverlay(homeId, zoneId);
            } else if (action.param(zoneModeActionModeParamTypeId).value().toString() == "Off") {
                tado->setOverlay(homeId, zoneId, false, device->stateValue(zoneTargetTemperatureStateTypeId).toDouble());
            } else {
                tado->setOverlay(homeId, zoneId, true, device->stateValue(zoneTargetTemperatureStateTypeId).toDouble());
            }
            info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == zoneTargetTemperatureActionTypeId) {

            double temperature = action.param(zoneTargetTemperatureActionTargetTemperatureParamTypeId).value().toDouble();
            if (temperature <= 0) {
                tado->setOverlay(homeId, zoneId, false, 0);
            } else {
                tado->setOverlay(homeId, zoneId, true, temperature);
            }
            info->finish(Device::DeviceErrorNoError);
        }
    }
}

void DevicePluginTado::onPluginTimer()
{
    foreach (Device *device, myDevices().filterByDeviceClassId(zoneDeviceClassId)) {
        Tado *tado = m_tadoAccounts.value(device->parentId());
        if (!tado)
            continue;

        QString homeId = device->paramValue(zoneDeviceHomeIdParamTypeId).toString();
        QString zoneId = device->paramValue(zoneDeviceZoneIdParamTypeId).toString();
        tado->getZoneState(homeId, zoneId);
    }
}

void DevicePluginTado::onConnectionChanged(bool connected)
{
    Tado *tado = static_cast<Tado*>(sender());

    if (m_tadoAccounts.values().contains(tado)){
        Device *device = myDevices().findById(m_tadoAccounts.key(tado));
        device->setStateValue(tadoConnectionConnectedStateTypeId, connected);

        foreach(Device *zoneDevice, myDevices().filterByParentDeviceId(device->id())) {
            zoneDevice->setStateValue(zoneConnectedStateTypeId, connected);
        }
    }
}

void DevicePluginTado::onAuthenticationStatusChanged(bool authenticated)
{
    Tado *tado = static_cast<Tado*>(sender());

    if (m_unfinishedTadoAccounts.values().contains(tado) && !authenticated){
        DeviceId id = m_tadoAccounts.key(tado);
        DevicePairingInfo *info = m_unfinishedDevicePairings.take(id);
        info->finish(Device::DeviceErrorSetupFailed);
    }

    if (m_tadoAccounts.values().contains(tado)){
        Device *device = myDevices().findById(m_tadoAccounts.key(tado));
        device->setStateValue(tadoConnectionLoggedInStateTypeId, authenticated);
    }
}

void DevicePluginTado::onTokenReceived(Tado::Token token)
{
    Q_UNUSED(token);

    qCDebug(dcTado()) << "Token received";
    Tado *tado = static_cast<Tado*>(sender());

    if (m_asyncDeviceSetup.contains(tado)) {
        DeviceSetupInfo *info = m_asyncDeviceSetup.take(tado);
        info->finish(Device::DeviceErrorNoError);
    }

    if (m_unfinishedTadoAccounts.values().contains(tado)) {
        DeviceId id = m_unfinishedTadoAccounts.key(tado);
        DevicePairingInfo *info = m_unfinishedDevicePairings.take(id);
        info->finish(Device::DeviceErrorNoError);
    }

    if (m_tadoAccounts.values().contains(tado)) {

    }
}

void DevicePluginTado::onHomesReceived(QList<Tado::Home> homes)
{
    qCDebug(dcTado()) << "Homes received";
    Tado *tado = static_cast<Tado*>(sender());
    foreach (Tado::Home home, homes) {
        tado->getZones(home.id);
    }
}

void DevicePluginTado::onZonesReceived(const QString &homeId, QList<Tado::Zone> zones)
{
    Tado *tado = static_cast<Tado*>(sender());

    if (m_tadoAccounts.values().contains(tado)) {

        Device *parentDevice = myDevices().findById(m_tadoAccounts.key(tado));
        qCDebug(dcTado()) << "Zones received:" << zones.count() << parentDevice->name();

        DeviceDescriptors descriptors;
        foreach (Tado::Zone zone, zones) {

            DeviceDescriptor descriptor(zoneDeviceClassId, zone.name, "Type:" + zone.type, parentDevice->id());
            ParamList params;
            params.append(Param(zoneDeviceHomeIdParamTypeId, homeId));
            params.append(Param(zoneDeviceZoneIdParamTypeId, zone.id));
            if (myDevices().findByParams(params))
                continue;

            params.append(Param(zoneDeviceTypeParamTypeId, zone.type));
            descriptor.setParams(params);
            descriptors.append(descriptor);
        }
        emit autoDevicesAppeared(descriptors);
    } else {
        qCWarning(dcTado()) << "Tado connection not linked to a device Id" << m_tadoAccounts.size() << m_tadoAccounts.key(tado).toString();
    }
}

void DevicePluginTado::onZoneStateReceived(const QString &homeId, const QString &zoneId, Tado::ZoneState state)
{
    Tado *tado = static_cast<Tado*>(sender());
    DeviceId parentId = m_tadoAccounts.key(tado);
    ParamList params;
    params.append(Param(zoneDeviceHomeIdParamTypeId, homeId));
    params.append(Param(zoneDeviceZoneIdParamTypeId, zoneId));
    Device *device = myDevices().filterByParentDeviceId(parentId).findByParams(params);
    if (!device)
        return;

    if (state.overlayIsSet)  {
        if (state.overlaySettingPower) {
            device->setStateValue(zoneModeStateTypeId, "Manual");
        } else {
            device->setStateValue(zoneModeStateTypeId, "Off");
        }
    } else {
        device->setStateValue(zoneModeStateTypeId, "Tado");
    }

    device->setStateValue(zonePowerStateTypeId, state.power);
    device->setStateValue(zoneConnectedStateTypeId, state.connected);
    device->setStateValue(zoneTargetTemperatureStateTypeId, state.settingTemperature);
    device->setStateValue(zoneTemperatureStateTypeId, state.temperature);
    device->setStateValue(zoneHumidityStateTypeId, state.humidity);
    device->setStateValue(zoneWindowOpenStateTypeId, state.windowOpen);
}
