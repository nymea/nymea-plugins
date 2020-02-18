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

#include "integrationpluginkeba.h"

#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QPointer>
#include "plugininfo.h"
#include <QUdpSocket>

IntegrationPluginKeba::IntegrationPluginKeba()
{

}

void DevicePluginKeba::init()
{

}

void DevicePluginKeba::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (info->deviceClassId() == wallboxDeviceClassId) {
        Discovery *discovery = new Discovery(this);
        discovery->discoverHosts(25);

        // clean up discovery object when this discovery info is deleted
        connect(info, &DeviceDiscoveryInfo::destroyed, discovery, &Discovery::deleteLater);

        connect(discovery, &Discovery::finished, info, [this, info](const QList<Host> &hosts) {
            qCDebug(dcKebaKeContact()) << "Discovery finished. Found" << hosts.count() << "devices";
            foreach (const Host &host, hosts) {
                if (!host.hostName().contains("keba", Qt::CaseSensitivity::CaseInsensitive))
                    continue;

                DeviceDescriptor descriptor(wallboxDeviceClassId, host.hostName().isEmpty() ? host.address() : host.hostName(), host.address() + " (" + host.macAddress() + ")");

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
        m_kebaDevices.remove(device->id());
    }

    if(m_kebaDevices.isEmpty()){
        m_kebaSocket->close();
        m_kebaSocket->deleteLater();
        qCDebug(dcKebaKeContact()) << "clear socket";
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
    KeContact *keba = static_cast<KeContact *>(sender());
    if (m_asyncSetup.contains(keba)) {
        DeviceSetupInfo *info = m_asyncSetup.value(keba);
        info->finish(Device::DeviceErrorNoError);

    } else {
        qCDebug(dcKebaKeContact()) << "Report one received without an associated async setup";

        Device *device = myDevices().findById(m_kebaDevices.key(keba));
        if (!device) {
            qCWarning(dcKebaKeContact()) << "Could not set serialnumber because of missing device object";
            return;
        }
        device->setParamValue(wallboxDeviceSerialnumberParamTypeId, reportOne.serialNumber);
    }
}

void DevicePluginKeba::onReportTwoReceived(const KeContact::ReportTwo &reportTwo)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Device *device = myDevices().findById(m_kebaDevices.key(keba));
    if (!device)
        return;

    switch (reportTwo.state) {
    case KeContact::State::Starting:
        device->setStateValue(wallboxActivityStateTypeId, QT_TR_NOOP("Starting"));
        break;
    case KeContact::State::NotReady:
        device->setStateValue(wallboxActivityStateTypeId, QT_TR_NOOP("Not ready for charging"));
        break;
    case KeContact::State::Ready:
        device->setStateValue(wallboxActivityStateTypeId, QT_TR_NOOP("Ready for charging"));
        break;
    case KeContact::State::Charging:
        device->setStateValue(wallboxActivityStateTypeId, QT_TR_NOOP("Charging"));
        break;
    case KeContact::State::Error:
        device->setStateValue(wallboxActivityStateTypeId, QT_TR_NOOP("Erro"));
        break;
    case KeContact::State::AuthorizationRejected:
        device->setStateValue(wallboxActivityStateTypeId, QT_TR_NOOP("Authorization rejected"));
        break;
    }

    switch (reportTwo.plugState) {
    case KeContact::PlugState::Unplugged:
        device->setStateValue(wallboxPlugStateStateTypeId, QT_TR_NOOP("Unplugged"));
        break;
    case KeContact::PlugState::PluggedOnChargingStation:
        device->setStateValue(wallboxPlugStateStateTypeId, QT_TR_NOOP("Plugged in charging station"));
        break;
    case KeContact::PlugState::PluggedOnChargingStationAndPluggedOnEV:
        device->setStateValue(wallboxPlugStateStateTypeId, QT_TR_NOOP("Plugged in on EV"));
        break;
    case KeContact::PlugState::PluggedOnChargingStationAndPlugLocked:
        device->setStateValue(wallboxPlugStateStateTypeId, QT_TR_NOOP("Plugged in and locked"));
        break;
    case KeContact::PlugState::PluggedOnChargingStationAndPlugLockedAndPluggedOnEV:
        device->setStateValue(wallboxPlugStateStateTypeId, QT_TR_NOOP("Plugged in on EV and locked"));
        break;
    }
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
    device->setStateValue(wallboxPStateTypeId, reportThree.Power);
    device->setStateValue(wallboxEPStateTypeId, reportThree.EnergySession);
    device->setStateValue(wallboxTotalEnergyConsumedStateTypeId, reportThree.EnergyTotal);
}

void DevicePluginKeba::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    qCDebug(dcKebaKeContact()) << "Execute action" << device->name() << action.actionTypeId().toString();

    if (device->deviceClassId() == wallboxDeviceClassId) {

        // Print information that we are executing now the update action
        qCDebug(dcKebaKeContact()) << "Execute update action" << action.id();
        KeContact *keba = m_kebaDevices.value(device->id());
        if (!keba) {
            qCWarning(dcKebaKeContact()) << "Device not properly initialized, Keba object missing";
            return info->finish(Device::DeviceErrorHardwareNotAvailable);
        }

        if(action.actionTypeId() == wallboxMaxChargingCurrentActionTypeId){
            int ampere = action.param(wallboxMaxChargingCurrentActionMaxChargingCurrentParamTypeId).value().toInt()*1000;
            QUuid requestId = keba->setMaxAmpere(ampere);
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
