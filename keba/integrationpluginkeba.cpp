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

#include "devicepluginkeba.h"
#include "plugininfo.h"

#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QUdpSocket>
#include <QTimeZone>

IntegrationPluginKeba::IntegrationPluginKeba()
{

}

void DevicePluginKeba::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (info->deviceClassId() == wallboxDeviceClassId) {
        Discovery *discovery = new Discovery(info);
        discovery->discoverHosts(25);

        connect(discovery, &Discovery::finished, info, [this, info](const QList<Host> &hosts) {
            qCDebug(dcKebaKeContact()) << "Discovery finished. Found" << hosts.count() << "devices";
            foreach (const Host &host, hosts) {
                if (!host.hostName().contains("keba", Qt::CaseSensitivity::CaseInsensitive))
                    continue;

                DeviceDescriptor descriptor(wallboxDeviceClassId, "Wallbox", host.address() + " (" + host.macAddress() + ")");

                foreach (Device *existingDevice, myDevices()) {
                    if (existingDevice->paramValue(wallboxDeviceMacAddressParamTypeId).toString() == host.macAddress()) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                ParamList params;
                params << Param(wallboxDeviceMacAddressParamTypeId, host.macAddress());
                params << Param(wallboxDeviceIpAddressParamTypeId, host.address());
                descriptor.setParams(params);
                info->addDeviceDescriptor(descriptor);
            }
            info->finish(Device::DeviceErrorNoError);
        });
    } else {
        qCWarning(dcKebaKeContact()) << "Discover device, unhandled device class" << info->deviceClassId();
        info->finish(Device::DeviceErrorDeviceClassNotFound);
    }
}

void IntegrationPluginKeba::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    qCDebug(dcKebaKeContact()) << "Setting up a new thing:" << thing->name() << thing->params();

    if (device->deviceClassId() == wallboxDeviceClassId) {

        QHostAddress address = QHostAddress(device->paramValue(wallboxDeviceIpAddressParamTypeId).toString());
        KeContact *keba = new KeContact(address, this);
        connect(keba, &KeContact::connectionChanged, this, &DevicePluginKeba::onConnectionChanged);
        connect(keba, &KeContact::commandExecuted, this, &DevicePluginKeba::onCommandExecuted);
        connect(keba, &KeContact::reportOneReceived, this, &DevicePluginKeba::onReportOneReceived);
        connect(keba, &KeContact::reportTwoReceived, this, &DevicePluginKeba::onReportTwoReceived);
        connect(keba, &KeContact::reportThreeReceived, this, &DevicePluginKeba::onReportThreeReceived);
        connect(keba, &KeContact::broadcastReceived, this, &DevicePluginKeba::onBroadcastReceived);
        if (!keba->init()){
            qCWarning(dcKebaKeContact()) << "Cannot bind to port" << 7090;
            keba->deleteLater();
            return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Error opening network port."));
        }

        DeviceId id = device->id();
        m_kebaDevices.insert(id, keba);
        m_asyncSetup.insert(keba, info);
        keba->getReport1();
        connect(info, &DeviceSetupInfo::aborted, this, [id, keba, this]{
            m_asyncSetup.remove(keba);
            m_kebaDevices.remove(id);
            keba->deleteLater();
        });
    } else {
        qCWarning(dcKebaKeContact()) << "setupDevice, unhandled device class" << device->deviceClass();
        info->finish(Device::DeviceErrorDeviceClassNotFound);
    }
}

void IntegrationPluginKeba::postSetupThing(Thing *thing)
{
    qCDebug(dcKebaKeContact()) << "Post setup" << device->name();
    KeContact *keba = m_kebaDevices.value(device->id());
    if (!keba) {
        return;
    }
    keba->getReport2();
    keba->getReport3();

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginKeba::updateData);
    }
}

void DevicePluginKeba::deviceRemoved(Device *device)
{   
    if (device->deviceClassId() == wallboxDeviceClassId) {
        KeContact *keba = m_kebaDevices.take(device->id());
        keba->deleteLater();
    }

    if (myDevices().empty()) {
        // last device has been removed the plug in timer can be stopped again
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginKeba::updateData()
{
    foreach (KeContact *keba, m_kebaDevices) {
        keba->getReport2();
        keba->getReport3();
    }

    foreach (Device *device, myDevices().filterByDeviceClassId(wallboxDeviceClassId)) {
        if (m_chargingSessionStartTime.contains(device->id())) {
            QDateTime startTime = m_chargingSessionStartTime.value(device->id());

            QTimeZone tz = QTimeZone(QTimeZone::systemTimeZoneId());
            QDateTime currentTime = QDateTime::currentDateTime().toTimeZone(tz);

            int minutes = (currentTime.toSecsSinceEpoch() - startTime.toSecsSinceEpoch())/60;
            device->setStateValue(wallboxSessionTimeStateTypeId, minutes);
        } else {
            device->setStateValue(wallboxSessionTimeStateTypeId, 0);
        }
    }
}

void DevicePluginKeba::setDeviceState(Device *device, KeContact::State state)
{
    switch (state) {
    case KeContact::StateStarting:
        device->setStateValue(wallboxActivityStateTypeId, "Starting");
        break;
    case KeContact::StateNotReady:
        device->setStateValue(wallboxActivityStateTypeId, "Not ready for charging");
        break;
    case KeContact::StateReady:
        device->setStateValue(wallboxActivityStateTypeId, "Ready for charging");
        break;
    case KeContact::StateCharging:
        device->setStateValue(wallboxActivityStateTypeId, "Charging");
        break;
    case KeContact::StateError:
        device->setStateValue(wallboxActivityStateTypeId, "Error");
        break;
    case KeContact::StateAuthorizationRejected:
        device->setStateValue(wallboxActivityStateTypeId, "Authorization rejected");
        break;
    }

    if (state == KeContact::StateCharging) {
        //Set charging session
        QTimeZone tz = QTimeZone(QTimeZone::systemTimeZoneId());
        QDateTime startedChargingSession = QDateTime::currentDateTime().toTimeZone(tz);
        m_chargingSessionStartTime.insert(device->id(), startedChargingSession);
    } else {
        m_chargingSessionStartTime.remove(device->id());
        device->setStateValue(wallboxSessionTimeStateTypeId, 0);
    }
}

void DevicePluginKeba::setDevicePlugState(Device *device, KeContact::PlugState plugState)
{
    switch (plugState) {
    case KeContact::PlugStateUnplugged:
        device->setStateValue(wallboxPlugStateStateTypeId, "Unplugged");
        break;
    case KeContact::PlugStatePluggedOnChargingStation:
        device->setStateValue(wallboxPlugStateStateTypeId, "Plugged in charging station");
        break;
    case KeContact::PlugStatePluggedOnChargingStationAndPluggedOnEV:
        device->setStateValue(wallboxPlugStateStateTypeId, "Plugged in on EV");
        break;
    case KeContact::PlugStatePluggedOnChargingStationAndPlugLocked:
        device->setStateValue(wallboxPlugStateStateTypeId, "Plugged in and locked");
        break;
    case KeContact::PlugStatePluggedOnChargingStationAndPlugLockedAndPluggedOnEV:
        device->setStateValue(wallboxPlugStateStateTypeId, "Plugged in on EV and locked");
        break;
    }
}

void DevicePluginKeba::onConnectionChanged(bool status)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Device *device = myDevices().findById(m_kebaDevices.key(keba));
    if (!device) {
        qCWarning(dcKebaKeContact()) << "On connection changed: missing device object";
        return;
    }
    device->setStateValue(wallboxConnectedStateTypeId, status);
    if (!status) {
        //TODO start rediscovery
    }
}

void DevicePluginKeba::onCommandExecuted(QUuid requestId, bool success)
{
    updateData();
    if (m_asyncActions.contains(requestId)) {
        KeContact *keba = static_cast<KeContact *>(sender());
        Device *device = myDevices().findById(m_kebaDevices.key(keba));
        if (!device) {
            qCWarning(dcKebaKeContact()) << "On command executed: missing device object";
            return;
        }
        DeviceActionInfo *info = m_asyncActions.take(requestId);
        if (success) {
            info->finish(Device::DeviceErrorNoError);
        } else {
            info->finish(Device::DeviceErrorHardwareFailure);
        }
    }
}

void DevicePluginKeba::onReportOneReceived(const KeContact::ReportOne &reportOne)
{
    Q_UNUSED(reportOne);
    KeContact *keba = static_cast<KeContact *>(sender());
    if (m_asyncSetup.contains(keba)) {
        DeviceSetupInfo *info = m_asyncSetup.value(keba);
        info->finish(Device::DeviceErrorNoError);
    } else {
        qCDebug(dcKebaKeContact()) << "Report one received without an associated async setup";
    }
}

void DevicePluginKeba::onReportTwoReceived(const KeContact::ReportTwo &reportTwo)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Device *device = myDevices().findById(m_kebaDevices.key(keba));
    if (!device)
        return;

    device->setStateValue(wallboxPowerStateTypeId, reportTwo.enableUser);
    device->setStateValue(wallboxMaxChargingCurrentPercentStateTypeId, reportTwo.MaxCurrentPercentage);

    setDeviceState(device, reportTwo.state);
    setDevicePlugState(device, reportTwo.plugState);
}

void DevicePluginKeba::onReportThreeReceived(const KeContact::ReportThree &reportThree)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Device *device = myDevices().findById(m_kebaDevices.key(keba));
    if (!device)
        return;

    device->setStateValue(wallboxI1EventTypeId, reportThree.CurrentPhase1);
    device->setStateValue(wallboxI2EventTypeId, reportThree.CurrentPhase2);
    device->setStateValue(wallboxI3EventTypeId, reportThree.CurrentPhase3);
    device->setStateValue(wallboxU1EventTypeId, reportThree.VoltagePhase1);
    device->setStateValue(wallboxU2EventTypeId, reportThree.VoltagePhase2);
    device->setStateValue(wallboxU3EventTypeId, reportThree.VoltagePhase3);
    device->setStateValue(wallboxPStateTypeId,  reportThree.Power);
    device->setStateValue(wallboxEPStateTypeId, reportThree.EnergySession);
    device->setStateValue(wallboxTotalEnergyConsumedStateTypeId, reportThree.EnergyTotal);
}

void DevicePluginKeba::onBroadcastReceived(KeContact::BroadcastType type, const QVariant &content)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Device *device = myDevices().findById(m_kebaDevices.key(keba));
    if (!device)
        return;

    switch (type) {
    case KeContact::BroadcastTypePlug:
        setDevicePlugState(device, KeContact::PlugState(content.toInt()));
        break;
    case KeContact::BroadcastTypeInput:
        break;
    case KeContact::BroadcastTypeEPres:
        device->setStateValue(wallboxEPStateTypeId, content.toInt());
        break;
    case KeContact::BroadcastTypeState:
        setDeviceState(device, KeContact::State(content.toInt()));
        break;
    case KeContact::BroadcastTypeMaxCurr:
        device->setStateValue(wallboxMaxChargingCurrentStateTypeId, content.toInt());
        break;
    case KeContact::BroadcastTypeEnableSys:
        break;
    }
}

void DevicePluginKeba::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == wallboxDeviceClassId) {
        KeContact *keba = m_kebaDevices.value(device->id());
        if (!keba) {
            qCWarning(dcKebaKeContact()) << "Device not properly initialized, Keba object missing";
            return info->finish(Device::DeviceErrorHardwareNotAvailable);
        }

        if(action.actionTypeId() == wallboxMaxChargingCurrentActionTypeId){
            int milliAmpere = action.param(wallboxMaxChargingCurrentActionMaxChargingCurrentParamTypeId).value().toInt();
            QUuid requestId = keba->setMaxAmpere(milliAmpere);
            m_asyncActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this]{m_asyncActions.remove(requestId);});

        } else if(action.actionTypeId() == wallboxPowerActionTypeId){
            QUuid requestId = keba->enableOutput(action.param(wallboxPowerActionTypeId).value().toBool());
            m_asyncActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this]{m_asyncActions.remove(requestId);});

        } else if(action.actionTypeId() == wallboxDisplayActionTypeId){
            QUuid requestId = keba->displayMessage(action.param(wallboxDisplayActionMessageParamTypeId).value().toByteArray());
            m_asyncActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this]{m_asyncActions.remove(requestId);});

        } else {
            qCWarning(dcKebaKeContact()) << "Unhandled ActionTypeId:" << action.actionTypeId();
            info->finish(Device::DeviceErrorActionTypeNotFound);
        }
    } else {
        qCWarning(dcKebaKeContact()) << "Execute action, unhandled device class" << device->deviceClass();
        info->finish(Device::DeviceErrorDeviceClassNotFound);
    }
}
