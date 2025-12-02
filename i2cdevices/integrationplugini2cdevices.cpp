// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationplugini2cdevices.h"
#include "plugininfo.h"

#include "pi16adcchannel.h"
#include "ads1115channel.h"
#include "ina219.h"

#include <hardware/i2c/i2cmanager.h>

#include <QDebug>
#include <QJsonDocument>

IntegrationPluginI2CDevices::IntegrationPluginI2CDevices(): IntegrationPlugin()
{
    m_ads1115ChannelMap.insert(0, ads1115Channel1StateTypeId);
    m_ads1115ChannelMap.insert(1, ads1115Channel2StateTypeId);
    m_ads1115ChannelMap.insert(2, ads1115Channel3StateTypeId);
    m_ads1115ChannelMap.insert(3, ads1115Channel4StateTypeId);

    m_ads1115OvervoltageMap.insert(0, ads1115Channel1overvoltageStateTypeId);
    m_ads1115OvervoltageMap.insert(1, ads1115Channel2overvoltageStateTypeId);
    m_ads1115OvervoltageMap.insert(2, ads1115Channel3overvoltageStateTypeId);
    m_ads1115OvervoltageMap.insert(3, ads1115Channel4overvoltageStateTypeId);

    m_pi16adcChannelMap.insert(0, pi16ADCChannel1StateTypeId);
    m_pi16adcChannelMap.insert(1, pi16ADCChannel2StateTypeId);
    m_pi16adcChannelMap.insert(2, pi16ADCChannel3StateTypeId);
    m_pi16adcChannelMap.insert(3, pi16ADCChannel4StateTypeId);
    m_pi16adcChannelMap.insert(4, pi16ADCChannel5StateTypeId);
    m_pi16adcChannelMap.insert(5, pi16ADCChannel6StateTypeId);
    m_pi16adcChannelMap.insert(6, pi16ADCChannel7StateTypeId);
    m_pi16adcChannelMap.insert(7, pi16ADCChannel8StateTypeId);
    m_pi16adcChannelMap.insert(8, pi16ADCChannel9StateTypeId);
    m_pi16adcChannelMap.insert(9, pi16ADCChannel10StateTypeId);
    m_pi16adcChannelMap.insert(10, pi16ADCChannel11StateTypeId);
    m_pi16adcChannelMap.insert(11, pi16ADCChannel12StateTypeId);
    m_pi16adcChannelMap.insert(12, pi16ADCChannel13StateTypeId);
    m_pi16adcChannelMap.insert(13, pi16ADCChannel14StateTypeId);
    m_pi16adcChannelMap.insert(14, pi16ADCChannel15StateTypeId);
    m_pi16adcChannelMap.insert(15, pi16ADCChannel16StateTypeId);

    m_pi16adcOvervoltageMap.insert(0, pi16ADCChannel1overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(1, pi16ADCChannel2overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(2, pi16ADCChannel3overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(3, pi16ADCChannel4overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(4, pi16ADCChannel5overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(5, pi16ADCChannel6overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(6, pi16ADCChannel7overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(7, pi16ADCChannel8overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(8, pi16ADCChannel9overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(9, pi16ADCChannel10overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(10, pi16ADCChannel11overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(11, pi16ADCChannel12overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(12, pi16ADCChannel13overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(13, pi16ADCChannel14overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(14, pi16ADCChannel15overvoltageStateTypeId);
    m_pi16adcOvervoltageMap.insert(15, pi16ADCChannel16overvoltageStateTypeId);
}

void IntegrationPluginI2CDevices::init() {
}

void IntegrationPluginI2CDevices::discoverThings(ThingDiscoveryInfo *info)
{
    foreach (const I2CScanResult &scanResult, hardwareManager()->i2cManager()->scanRegisters()) {
        qCDebug(dcI2cDevices()) << "Found I2C deevice on port:" << scanResult.portName << "0x" + QString::number(scanResult.address, 16);

        if (info->thingClassId() == pi16ADCThingClassId) {
            // Depending on jumper settings, the Pi-16ADC can be any of those:
            QList<int> addresses = {0x14, 0x15, 0x16, 0x17, 0x24, 0x25, 0x26, 0x27, 0x34, 0x35, 0x36, 0x37, 0x44, 0x45, 0x46, 0x47, 0x54, 0x55, 0x56, 0x57, 0x64, 0x65, 0x66, 0x67, 0x74, 0x75, 0x76};
            if (addresses.contains(scanResult.address)) {
                ThingDescriptor descriptor(pi16ADCThingClassId, "Pi-16ADC", QString("%1: 0x%2").arg(scanResult.portName).arg(scanResult.address, 0, 16));
                ParamList params;
                params << Param(pi16ADCThingI2cPortParamTypeId, scanResult.portName);
                params << Param(pi16ADCThingI2cAddressParamTypeId, scanResult.address);
                descriptor.setParams(params);
                info->addThingDescriptor(descriptor);
            }
        }

        if (info->thingClassId() == ads1115ThingClassId) {
            // The ADS1115 has selectable addresses from 0x48 to 0x4B
            if (scanResult.address >= 0x48 && scanResult.address <= 0x4B) {
                ThingDescriptor descriptor(ads1115ThingClassId, "ADS1113/ADS1114/ADS1115", QString("%1: 0x%2").arg(scanResult.portName).arg(scanResult.address, 0, 16));
                ParamList params;
                params << Param(ads1115ThingI2cPortParamTypeId, scanResult.portName);
                params << Param(ads1115ThingI2cAddressParamTypeId, scanResult.address);
                descriptor.setParams(params);
                info->addThingDescriptor(descriptor);
            }
        }

        if (info->thingClassId() == ina219ThingClassId) {
            // The INA219 has selectable addresses from 0x040 tox 0x4A
            if (scanResult.address >= 0x40 && scanResult.address <= 0x4A) {
                ThingDescriptor descriptor(ina219ThingClassId, "INA219", QString("%1: 0x%2").arg(scanResult.portName).arg(scanResult.address, 0, 16));
                ParamList params;
                params << Param(ina219ThingI2cPortParamTypeId, scanResult.portName);
                params << Param(ina219ThingI2cAddressParamTypeId, scanResult.address);
                descriptor.setParams(params);
                info->addThingDescriptor(descriptor);
            }
        }
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginI2CDevices::setupThing(ThingSetupInfo *info)
{
    if (info->thing()->thingClassId() == pi16ADCThingClassId) {

        QString i2cPortName = info->thing()->paramValue(pi16ADCThingI2cPortParamTypeId).toString();
        int i2cAddress = info->thing()->paramValue(pi16ADCThingI2cAddressParamTypeId).toInt();
        Q_UNUSED(i2cAddress)

        for (int i = 0; i < 16; i++) {
            Pi16ADCChannel *pi16ADC = new Pi16ADCChannel(i2cPortName, i2cAddress, i, this);
            if (!hardwareManager()->i2cManager()->open(pi16ADC)) {
                delete pi16ADC;
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Failed to open I2C port."));
                return;
            }
            Thing *thing = info->thing();
            connect(pi16ADC, &Pi16ADCChannel::readingAvailable, thing, [this, thing, i](const QByteArray &data){
                if (data.length() != 3) {
                    qCWarning(dcI2cDevices()) << "Error reading from" << thing->name();
                    return;
                }
                int value = ((((data[0]&0x3F))<<16))+((data[1]<<8))+(((data[2]&0xE0)));
                const int max = 8388608;
                double transformedValue = 2.5 * value / max;
                thing->setStateValue(m_pi16adcChannelMap.value(i), transformedValue);
                thing->setStateValue(m_pi16adcOvervoltageMap.value(i), (data[0] & 0xC0));
            });
            hardwareManager()->i2cManager()->startReading(pi16ADC, 5000);
            m_i2cDevices.insert(pi16ADC, thing);
        }

        info->finish(Thing::ThingErrorNoError);
    }

    if (info->thing()->thingClassId() == ads1115ThingClassId) {
        QString i2cPortName = info->thing()->paramValue(ads1115ThingI2cPortParamTypeId).toString();
        int i2cAddress = info->thing()->paramValue(ads1115ThingI2cAddressParamTypeId).toInt();
        double gainParam = info->thing()->paramValue(ads1115ThingInputGainParamTypeId).toDouble();
        ADS1115Channel::Gain inputGain = ADS1115Channel::Gain_4_096;
        if (qFuzzyCompare(gainParam, 6.144)) {
            inputGain = ADS1115Channel::Gain_6_144;
        } else if (qFuzzyCompare(gainParam, 4.096)) {
            inputGain = ADS1115Channel::Gain_4_096;
        } else if (qFuzzyCompare(gainParam, 2.048)) {
            inputGain = ADS1115Channel::Gain_2_048;
        } else if (qFuzzyCompare(gainParam, 1.024)) {
            inputGain = ADS1115Channel::Gain_1_024;
        } else if (qFuzzyCompare(gainParam, 0.512)) {
            inputGain = ADS1115Channel::Gain_0_512;
        } else if (qFuzzyCompare(gainParam, 0.256)) {
            inputGain = ADS1115Channel::Gain_0_256;
        }

        for (int i = 0; i < 4; i++) {
            ADS1115Channel *ads1115 = new ADS1115Channel(i2cPortName, i2cAddress, i, inputGain, this);
            if (!hardwareManager()->i2cManager()->open(ads1115)) {
                delete ads1115;
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Failed to open I2C port."));
                return;
            }

            Thing *thing = info->thing();
            connect(ads1115, &ADS1115Channel::readingAvailable, thing, [this, thing, i](const QByteArray &data){
                if (data.length() != 2) {
                    qCWarning(dcI2cDevices()) << "Error reading from" << thing;
                    return;
                }
                const int max = 32768;
                int value = static_cast<qint16>(data[0]) * 256 + static_cast<qint16>(data[1]);
                double transformedValue = qMin(1.0 * value / max, 1.0);
                thing->setStateValue(m_ads1115ChannelMap.value(i), transformedValue);
                thing->setStateValue(m_ads1115OvervoltageMap.value(i), value > 32768);
            });
            hardwareManager()->i2cManager()->startReading(ads1115, 5000);
            m_i2cDevices.insert(ads1115, thing);
        }
        info->finish(Thing::ThingErrorNoError);
    }

    if (info->thing()->thingClassId() == ina219ThingClassId) {
        QString i2cPortName = info->thing()->paramValue(ina219ThingI2cPortParamTypeId).toString();
        int i2cAddress = info->thing()->paramValue(ina219ThingI2cAddressParamTypeId).toInt();
        double shuntOhms = info->thing()->paramValue(ina219ThingShuntOhmsParamTypeId).toDouble();
        Ina219::VoltageRange voltageRange = info->thing()->paramValue(ina219ThingVoltageRangeParamTypeId).toUInt() == 16 ? Ina219::VoltageRange16 : Ina219::VoltageRange32;

        Ina219 *ina219 = new Ina219(i2cPortName, i2cAddress, shuntOhms, voltageRange, this);
        if (!hardwareManager()->i2cManager()->open(ina219)) {
            delete ina219;
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Failed to open I2C port."));
            return;
        }

        Thing *thing = info->thing();
        connect(ina219, &Ina219::readingAvailable, thing, [thing](const QByteArray &data){
            QJsonParseError error;
            QVariantMap values = QJsonDocument::fromJson(data, &error).toVariant().toMap();
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcI2cDevices()) << thing->name() << "Failed to read data from INA219";
                return;
            }
            double currentPower = values.value("power").toDouble();
            thing->setStateValue(ina219CurrentPowerStateTypeId, currentPower);
            thing->setStateValue(ina219VoltagePhaseAStateTypeId, values.value("busVoltage").toDouble());
            thing->setStateValue(ina219CurrentPhaseAStateTypeId, values.value("current").toDouble());
            thing->setStateValue(ina219OverflowStateTypeId, values.value("overflow").toBool());

            // Calculate an estimate of totalEnergyConsumed
            QDateTime lastUpdate = thing->property("lastUpdate").toDateTime();
            if (lastUpdate.isNull()) {
                lastUpdate = QDateTime::currentDateTime();
            }
            double hoursPassed = lastUpdate.msecsTo(QDateTime::currentDateTime()) / 1000 / 60 / 60;
            if (currentPower >= 0) {
                double totalEnergyConsumed = thing->stateValue(ina219TotalEnergyConsumedStateTypeId).toDouble();
                totalEnergyConsumed += currentPower / 1000 * hoursPassed;
                thing->setStateValue(ina219TotalEnergyConsumedStateTypeId, totalEnergyConsumed);
            } else {
                double totalEnergyReturned = thing->stateValue(ina219TotalEnergyProducedStateTypeId).toDouble();
                totalEnergyReturned += -currentPower / 1000 * hoursPassed;
                thing->setStateValue(ina219TotalEnergyProducedStateTypeId, totalEnergyReturned);
            }
        });

        hardwareManager()->i2cManager()->writeData(ina219, "init");
        hardwareManager()->i2cManager()->startReading(ina219, 5000);

        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginI2CDevices::thingRemoved(Thing *thing)
{
    foreach (I2CDevice* i2cDevice, m_i2cDevices.keys(thing)) {
        hardwareManager()->i2cManager()->close(i2cDevice);
        i2cDevice->deleteLater();
        m_i2cDevices.take(i2cDevice);
    }
}
