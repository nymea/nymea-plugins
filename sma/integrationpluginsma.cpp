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

#include "integrationpluginsma.h"
#include "plugininfo.h"

#include <network/networkdevicediscovery.h>
#include "speedwirediscovery.h"

IntegrationPluginSma::IntegrationPluginSma()
{

}

void IntegrationPluginSma::init()
{

}

void IntegrationPluginSma::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == sunnyWebBoxThingClassId) {
        if (!hardwareManager()->networkDeviceDiscovery()->available()) {
            qCWarning(dcSma()) << "Failed to discover network devices. The network device discovery is not available.";
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to discover devices in your network."));
            return;
        }

        qCDebug(dcSma()) << "Starting network discovery...";
        NetworkDeviceDiscoveryReply *discoveryReply = hardwareManager()->networkDeviceDiscovery()->discover();
        connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
            ThingDescriptors descriptors;
            qCDebug(dcSma()) << "Discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "devices";
            foreach (const NetworkDeviceInfo &networkDeviceInfo, discoveryReply->networkDeviceInfos()) {
                // Filter for sma hosts
                if (!networkDeviceInfo.hostName().toLower().contains("sma"))
                    continue;

                QString title = networkDeviceInfo.hostName() + " (" + networkDeviceInfo.address().toString() + ")";
                QString description;
                if (networkDeviceInfo.macAddressManufacturer().isEmpty()) {
                    description = networkDeviceInfo.macAddress();
                } else {
                    description = networkDeviceInfo.macAddress() + " (" + networkDeviceInfo.macAddressManufacturer() + ")";
                }

                ThingDescriptor descriptor(sunnyWebBoxThingClassId, title, description);

                // Check for reconfiguration
                foreach (Thing *existingThing, myThings()) {
                    if (existingThing->paramValue(sunnyWebBoxThingMacAddressParamTypeId).toString() == networkDeviceInfo.macAddress()) {
                        descriptor.setThingId(existingThing->id());
                        break;
                    }
                }

                ParamList params;
                params << Param(sunnyWebBoxThingHostParamTypeId, networkDeviceInfo.address().toString());
                params << Param(sunnyWebBoxThingMacAddressParamTypeId, networkDeviceInfo.macAddress());
                descriptor.setParams(params);
                descriptors.append(descriptor);
            }
            info->addThingDescriptors(descriptors);
            info->finish(Thing::ThingErrorNoError);
        });
    } else if (info->thingClassId() == speedwireMeterThingClassId) {
        SpeedwireDiscovery *speedwireDiscovery = new SpeedwireDiscovery(hardwareManager()->networkDeviceDiscovery(), info);
        if (!speedwireDiscovery->initialize()) {
            qCWarning(dcSma()) << "Could not discovery inverter. The speedwire interface initialization failed.";
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unable to discover the network."));
            return;
        }

        connect(speedwireDiscovery, &SpeedwireDiscovery::discoveryFinished, this, [=](){
            qCDebug(dcSma()) << "Speed wire discovery finished.";
            speedwireDiscovery->deleteLater();

            ThingDescriptors descriptors;
            foreach (const SpeedwireDiscovery::SpeedwireDiscoveryResult &result, speedwireDiscovery->discoveryResult()) {
                if (result.deviceType == SpeedwireInterface::DeviceTypeMeter) {
                    if (result.serialNumber == 0)
                        continue;

                    ThingDescriptor descriptor(speedwireMeterThingClassId, "SMA Energy Meter", "Serial: " + QString::number(result.serialNumber) + " - " + result.address.toString());
                    // We found an energy meter, let's check if we already added this one
                    foreach (Thing *existingThing, myThings()) {
                        if (existingThing->paramValue(speedwireMeterThingSerialNumberParamTypeId).toUInt() == result.serialNumber) {
                            descriptor.setThingId(existingThing->id());
                            break;
                        }
                    }

                    ParamList params;
                    params << Param(speedwireMeterThingHostParamTypeId, result.address.toString());
                    params << Param(speedwireMeterThingMacAddressParamTypeId, result.networkDeviceInfo.macAddress());
                    params << Param(speedwireMeterThingSerialNumberParamTypeId, result.serialNumber);
                    params << Param(speedwireMeterThingModelIdParamTypeId, result.modelId);
                    descriptor.setParams(params);
                    descriptors.append(descriptor);
                }
            }

            info->addThingDescriptors(descriptors);
            info->finish(Thing::ThingErrorNoError);
        });

        speedwireDiscovery->startDiscovery();
    }
}

void IntegrationPluginSma::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcSma()) << "Setup thing" << thing << thing->params();

    if (thing->thingClassId() == sunnyWebBoxThingClassId) {
        // Check if a Sunny WebBox is already added with this mac address
        foreach (SunnyWebBox *sunnyWebBox, m_sunnyWebBoxes.values()) {
            if (sunnyWebBox->macAddress() == thing->paramValue(sunnyWebBoxThingMacAddressParamTypeId).toString()){
                qCWarning(dcSma()) << "Thing with mac address" << thing->paramValue(sunnyWebBoxThingMacAddressParamTypeId).toString() << " already added!";
                info->finish(Thing::ThingErrorThingInUse);
                return;
            }
        }

        if (m_sunnyWebBoxes.contains(thing)) {
            qCDebug(dcSma()) << "Setup after reconfiguration, cleaning up...";
            m_sunnyWebBoxes.take(thing)->deleteLater();
        }

        SunnyWebBox *sunnyWebBox = new SunnyWebBox(hardwareManager()->networkManager(), QHostAddress(thing->paramValue(sunnyWebBoxThingHostParamTypeId).toString()), this);
        sunnyWebBox->setMacAddress(thing->paramValue(sunnyWebBoxThingMacAddressParamTypeId).toString());

        connect(info, &ThingSetupInfo::aborted, sunnyWebBox, &SunnyWebBox::deleteLater);
        connect(sunnyWebBox, &SunnyWebBox::destroyed, this, [thing, this] { m_sunnyWebBoxes.remove(thing);});

        QString requestId = sunnyWebBox->getPlantOverview();
        connect(sunnyWebBox, &SunnyWebBox::plantOverviewReceived, info, [=] (const QString &messageId, SunnyWebBox::Overview overview) {
            qCDebug(dcSma()) << "Received plant overview" << messageId << "Finish setup";
            Q_UNUSED(overview)

            info->finish(Thing::ThingErrorNoError);
            connect(sunnyWebBox, &SunnyWebBox::connectedChanged, this, &IntegrationPluginSma::onConnectedChanged);
            connect(sunnyWebBox, &SunnyWebBox::plantOverviewReceived, this, &IntegrationPluginSma::onPlantOverviewReceived);
            m_sunnyWebBoxes.insert(info->thing(), sunnyWebBox);

            if (!m_refreshTimer) {
                qCDebug(dcSma()) << "Starting refresh timer";
                m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
                connect(m_refreshTimer, &PluginTimer::timeout, this, &IntegrationPluginSma::onRefreshTimer);
            }
        });
    } else if (thing->thingClassId() == speedwireMeterThingClassId) {
        QHostAddress address = QHostAddress(thing->paramValue(speedwireMeterThingHostParamTypeId).toString());
        quint32 serialNumber = static_cast<quint32>(thing->paramValue(speedwireMeterThingSerialNumberParamTypeId).toUInt());
        quint16 modelId = static_cast<quint16>(thing->paramValue(speedwireMeterThingModelIdParamTypeId).toUInt());

        if (m_speedwireMeters.contains(thing)) {
            m_speedwireMeters.take(thing)->deleteLater();
        }

        SpeedwireMeter *meter = new SpeedwireMeter(address, modelId, serialNumber, this);
        if (!meter->initialize()) {
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // TODO: reachable state

        connect(meter, &SpeedwireMeter::valuesUpdated, this, [=](){
            thing->setStateValue(speedwireMeterConnectedStateTypeId, true);
            thing->setStateValue(speedwireMeterCurrentPowerStateTypeId, meter->currentPower());
            thing->setStateValue(speedwireMeterCurrentPowerPhaseAStateTypeId, meter->currentPowerPhaseA());
            thing->setStateValue(speedwireMeterCurrentPowerPhaseBStateTypeId, meter->currentPowerPhaseB());
            thing->setStateValue(speedwireMeterCurrentPowerPhaseCStateTypeId, meter->currentPowerPhaseC());
            thing->setStateValue(speedwireMeterVoltagePhaseAStateTypeId, meter->voltagePhaseA());
            thing->setStateValue(speedwireMeterVoltagePhaseBStateTypeId, meter->voltagePhaseB());
            thing->setStateValue(speedwireMeterVoltagePhaseCStateTypeId, meter->voltagePhaseC());
            thing->setStateValue(speedwireMeterTotalEnergyConsumedStateTypeId, meter->totalEnergyConsumed());
            thing->setStateValue(speedwireMeterTotalEnergyProducedStateTypeId, meter->totalEnergyProduced());
            thing->setStateValue(speedwireMeterEnergyConsumedPhaseAStateTypeId, meter->energyConsumedPhaseA());
            thing->setStateValue(speedwireMeterEnergyConsumedPhaseBStateTypeId, meter->energyConsumedPhaseB());
            thing->setStateValue(speedwireMeterEnergyConsumedPhaseCStateTypeId, meter->energyConsumedPhaseC());
            thing->setStateValue(speedwireMeterEnergyProducedPhaseAStateTypeId, meter->energyProducedPhaseA());
            thing->setStateValue(speedwireMeterEnergyProducedPhaseBStateTypeId, meter->energyProducedPhaseB());
            thing->setStateValue(speedwireMeterEnergyProducedPhaseCStateTypeId, meter->energyProducedPhaseC());
            thing->setStateValue(speedwireMeterCurrentPhaseAStateTypeId, meter->amperePhaseA());
            thing->setStateValue(speedwireMeterCurrentPhaseBStateTypeId, meter->amperePhaseB());
            thing->setStateValue(speedwireMeterCurrentPhaseCStateTypeId, meter->amperePhaseC());
            thing->setStateValue(speedwireMeterFirmwareVersionStateTypeId, meter->softwareVersion());
        });

        m_speedwireMeters.insert(thing, meter);
        info->finish(Thing::ThingErrorNoError);
    }else {
        Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginSma::postSetupThing(Thing *thing)
{
    qCDebug(dcSma()) << "Post setup thing" << thing->name();
    if (thing->thingClassId() == sunnyWebBoxThingClassId) {
        SunnyWebBox *sunnyWebBox = m_sunnyWebBoxes.value(thing);
        if (!sunnyWebBox)
            return;
        sunnyWebBox->getPlantOverview();
        thing->setStateValue(sunnyWebBoxConnectedStateTypeId, true);
    }
}

void IntegrationPluginSma::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == sunnyWebBoxThingClassId) {
        m_sunnyWebBoxes.take(thing)->deleteLater();
    }

    if (myThings().isEmpty()) {
        qCDebug(dcSma()) << "Stopping timer";
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
        m_refreshTimer = nullptr;
    }
}

void IntegrationPluginSma::onRefreshTimer()
{
    foreach (Thing *thing, myThings().filterByThingClassId(sunnyWebBoxThingClassId)) {
        SunnyWebBox *sunnyWebBox = m_sunnyWebBoxes.value(thing);
        sunnyWebBox->getPlantOverview();
    }
}

void IntegrationPluginSma::onConnectedChanged(bool connected)
{
    Thing *thing = m_sunnyWebBoxes.key(static_cast<SunnyWebBox *>(sender()));
    if (!thing)
        return;
    thing->setStateValue(sunnyWebBoxConnectedStateTypeId, connected);
}

void IntegrationPluginSma::onPlantOverviewReceived(const QString &messageId, SunnyWebBox::Overview overview)
{
    Q_UNUSED(messageId)

    qCDebug(dcSma()) << "Plant overview received" << overview.status;
    Thing *thing = m_sunnyWebBoxes.key(static_cast<SunnyWebBox *>(sender()));
    if (!thing)
        return;

    thing->setStateValue(sunnyWebBoxCurrentPowerStateTypeId, overview.power);
    thing->setStateValue(sunnyWebBoxDayEnergyProducedStateTypeId, overview.dailyYield);
    thing->setStateValue(sunnyWebBoxTotalEnergyProducedStateTypeId, overview.totalYield);
    thing->setStateValue(sunnyWebBoxModeStateTypeId, overview.status);
    if (!overview.error.isEmpty()){
        qCDebug(dcSma()) << "Received error" << overview.error;
        thing->setStateValue(sunnyWebBoxErrorStateTypeId, overview.error);
    }
}
