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

#include "integrationplugingpio.h"
#include "types/param.h"
#include "integrations/thing.h"
#include "plugininfo.h"

IntegrationPluginGpio::IntegrationPluginGpio()
{
}

void IntegrationPluginGpio::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcGpioController()) << "Setup" << thing->name() << thing->params();

    // Check if GPIOs are available on this platform
    if (!Gpio::isAvailable()) {
        qCWarning(dcGpioController()) << "There are ou GPIOs on this plattform";
        //: Error setting up GPIO thing
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No GPIOs found on this system."));
    }

    // GPIO Switch
    if (thing->thingClassId() == gpioOutputRpiThingClassId || thing->thingClassId() == gpioOutputBbbThingClassId) {
        // Create and configure gpio
        int gpioId = -1;
        if (thing->thingClassId() == gpioOutputRpiThingClassId)
            gpioId = thing->paramValue(gpioOutputRpiThingGpioParamTypeId).toInt();

        if (thing->thingClassId() == gpioOutputBbbThingClassId)
            gpioId = thing->paramValue(gpioOutputBbbThingGpioParamTypeId).toInt();

        Gpio *gpio = new Gpio(gpioId, this);

        if (!gpio->exportGpio()) {
            qCWarning(dcGpioController()) << "Could not export gpio for thing" << thing->name();
            //: Error setting up GPIO thing
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Exporting GPIO failed."));
        }

        if (!gpio->setDirection(Gpio::DirectionOutput)) {
            qCWarning(dcGpioController()) << "Could not configure output gpio for thing" << thing->name();
            //: Error setting up GPIO thing
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Configuring output GPIO failed."));
        }

        if (!gpio->setValue(Gpio::ValueLow)) {
            qCWarning(dcGpioController()) << "Could not set gpio  value for thing" << thing->name();
            //: Error setting up GPIO thing
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Setting GPIO value failed."));
        }

        m_gpioDevices.insert(gpio, thing);

        if (thing->thingClassId() == gpioOutputRpiThingClassId)
            m_raspberryPiGpios.insert(gpio->gpioNumber(), gpio);

        if (thing->thingClassId() == gpioOutputBbbThingClassId)
            m_beagleboneBlackGpios.insert(gpio->gpioNumber(), gpio);

        return info->finish(Thing::ThingErrorNoError);
    }

    if (thing->thingClassId() == gpioInputRpiThingClassId || thing->thingClassId() == gpioInputBbbThingClassId) {

        int gpioId = -1;
        if (thing->thingClassId() == gpioInputRpiThingClassId)
            gpioId = thing->paramValue(gpioInputRpiThingGpioParamTypeId).toInt();

        if (thing->thingClassId() == gpioInputBbbThingClassId)
            gpioId = thing->paramValue(gpioInputBbbThingGpioParamTypeId).toInt();

        GpioMonitor *monitor = new GpioMonitor(gpioId, this);

        if (!monitor->enable()) {
            qCWarning(dcGpioController()) << "Could not enable gpio monitor for thing" << thing->name();
            //: Error setting up GPIO thing
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Enabling GPIO monitor failed."));
        }

        connect(monitor, &GpioMonitor::valueChanged, this, &IntegrationPluginGpio::onGpioValueChanged);

        m_monitorDevices.insert(monitor, thing);

        if (thing->thingClassId() == gpioInputRpiThingClassId)
            m_raspberryPiGpioMoniors.insert(monitor->gpio()->gpioNumber(), monitor);

        if (thing->thingClassId() == gpioInputBbbThingClassId)
            m_beagleboneBlackGpioMoniors.insert(monitor->gpio()->gpioNumber(), monitor);

        return info->finish(Thing::ThingErrorNoError);
    }

    if (thing->thingClassId() == counterRpiThingClassId || thing->thingClassId() == counterBbbThingClassId) {

        int gpioId = -1;
        if (thing->thingClassId() == counterRpiThingClassId)
            gpioId = thing->paramValue(counterRpiThingGpioParamTypeId).toInt();

        if (thing->thingClassId() == counterBbbThingClassId)
            gpioId = thing->paramValue(counterBbbThingGpioParamTypeId).toInt();

        GpioMonitor *monitor = new GpioMonitor(gpioId, this);

        if (!monitor->enable()) {
            qCWarning(dcGpioController()) << "Could not enable gpio monitor for thing" << thing->name();
            //: Error setting up GPIO thing
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Enabling GPIO monitor failed."));
        }

        connect(monitor, &GpioMonitor::valueChanged, this, &IntegrationPluginGpio::onGpioValueChanged);

        m_monitorDevices.insert(monitor, thing);

        if (thing->thingClassId() == counterRpiThingClassId)
            m_raspberryPiGpioMoniors.insert(monitor->gpio()->gpioNumber(), monitor);

        if (thing->thingClassId() == counterBbbThingClassId)
            m_beagleboneBlackGpioMoniors.insert(monitor->gpio()->gpioNumber(), monitor);

        m_counterValues.insert(thing->id(), 0);
        return info->finish(Thing::ThingErrorNoError);
    }
    return info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginGpio::discoverThings(ThingDiscoveryInfo *info)
{
    ThingClassId deviceClassId = info->thingClassId();

    // Check if GPIOs are available on this platform
    if (!Gpio::isAvailable()) {
        qCWarning(dcGpioController()) << "There are no GPIOs on this plattform";
        //: Error discovering GPIO devices
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No GPIOs available on this system."));
    }

    // Check which board / gpio configuration
    const ThingClass deviceClass = supportedThings().findById(deviceClassId);
    if (deviceClass.vendorId() == raspberryPiVendorId) {

        // Create the list of available gpios
        QList<GpioDescriptor> gpioDescriptors = raspberryPiGpioDescriptors();
        for (int i = 0; i < gpioDescriptors.count(); i++) {
            const GpioDescriptor gpioDescriptor = gpioDescriptors.at(i);

            // Offer only gpios which arn't in use already
            if (m_raspberryPiGpios.keys().contains(gpioDescriptor.gpio()))
                continue;

            if (m_raspberryPiGpioMoniors.keys().contains(gpioDescriptor.gpio()))
                continue;

            QString description;
            if (gpioDescriptor.description().isEmpty()) {
                description = QString("Pin %1").arg(gpioDescriptor.pin());
            } else {
                description = QString("Pin %1 | %2").arg(gpioDescriptor.pin()).arg(gpioDescriptor.description());
            }

            ThingDescriptor descriptor(deviceClassId, QString("GPIO %1").arg(gpioDescriptor.gpio()), description);
            ParamList parameters;
            if (deviceClass.id() == gpioOutputRpiThingClassId) {
                parameters.append(Param(gpioOutputRpiThingGpioParamTypeId, gpioDescriptor.gpio()));
                parameters.append(Param(gpioOutputRpiThingPinParamTypeId, gpioDescriptor.pin()));
                parameters.append(Param(gpioOutputRpiThingDescriptionParamTypeId, gpioDescriptor.description()));
            } else if (deviceClass.id() == gpioInputRpiThingClassId) {
                parameters.append(Param(gpioInputRpiThingGpioParamTypeId, gpioDescriptor.gpio()));
                parameters.append(Param(gpioInputRpiThingPinParamTypeId, gpioDescriptor.pin()));
                parameters.append(Param(gpioInputRpiThingDescriptionParamTypeId, gpioDescriptor.description()));
            }
            descriptor.setParams(parameters);

            foreach (Thing *existingThing, myThings()) {
                if (existingThing->paramValue(gpioOutputRpiThingGpioParamTypeId).toInt() == gpioDescriptor.gpio()) {
                    descriptor.setThingId(existingThing->id());
                    break;
                }
            }
            info->addThingDescriptor(descriptor);
        }

        return info->finish(Thing::ThingErrorNoError);
    }

    if (deviceClass.vendorId() == beagleboneBlackVendorId) {

        // Create the list of available gpios
        QList<GpioDescriptor> gpioDescriptors = beagleboneBlackGpioDescriptors();
        for (int i = 0; i < gpioDescriptors.count(); i++) {
            const GpioDescriptor gpioDescriptor = gpioDescriptors.at(i);

            // Offer only gpios which arn't in use already
            if (m_beagleboneBlackGpios.keys().contains(gpioDescriptor.gpio()))
                continue;

            if (m_beagleboneBlackGpioMoniors.keys().contains(gpioDescriptor.gpio()))
                continue;

            QString description;
            if (gpioDescriptor.description().isEmpty()) {
                description = QString("Pin %1").arg(gpioDescriptor.pin());
            } else {
                description = QString("Pin %1 | %2").arg(gpioDescriptor.pin()).arg(gpioDescriptor.description());
            }

            ThingDescriptor descriptor(deviceClassId, QString("GPIO %1").arg(gpioDescriptor.gpio()), description);
            ParamList parameters;
            if (deviceClass.id() == gpioOutputBbbThingClassId) {
                parameters.append(Param(gpioOutputBbbThingGpioParamTypeId, gpioDescriptor.gpio()));
                parameters.append(Param(gpioOutputBbbThingPinParamTypeId, gpioDescriptor.pin()));
                parameters.append(Param(gpioOutputBbbThingDescriptionParamTypeId, gpioDescriptor.description()));
            } else if (deviceClass.id() == gpioInputBbbThingClassId) {
                parameters.append(Param(gpioInputBbbThingGpioParamTypeId, gpioDescriptor.gpio()));
                parameters.append(Param(gpioInputBbbThingPinParamTypeId, gpioDescriptor.pin()));
                parameters.append(Param(gpioInputBbbThingDescriptionParamTypeId, gpioDescriptor.description()));
            }
            descriptor.setParams(parameters);

            info->addThingDescriptor(descriptor);
        }

        return info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginGpio::thingRemoved(Thing *thing)
{
    if (m_gpioDevices.values().contains(thing)) {
        Gpio *gpio = m_gpioDevices.key(thing);
        if (!gpio)
            return;

        m_gpioDevices.remove(gpio);

        if (m_raspberryPiGpios.values().contains(gpio))
            m_raspberryPiGpios.remove(gpio->gpioNumber());

        if (m_beagleboneBlackGpios.values().contains(gpio))
            m_beagleboneBlackGpios.remove(gpio->gpioNumber());

        delete gpio;
    }

    if (m_monitorDevices.values().contains(thing)) {
        GpioMonitor *monitor = m_monitorDevices.key(thing);
        if (!monitor)
            return;

        m_monitorDevices.remove(monitor);

        if (m_raspberryPiGpioMoniors.values().contains(monitor))
            m_raspberryPiGpios.remove(monitor->gpio()->gpioNumber());

        if (m_beagleboneBlackGpioMoniors.values().contains(monitor))
            m_beagleboneBlackGpioMoniors.remove(monitor->gpio()->gpioNumber());

        delete monitor;
    }

    if (m_counterValues.contains(thing->id())) {
        m_counterValues.remove(thing->id());
    }

    if (myThings().filterByThingClassId(counterRpiThingClassId).isEmpty() && myThings().filterByThingClassId(counterBbbThingClassId).isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_counterTimer);
        m_counterTimer = nullptr;
    }
}

void IntegrationPluginGpio::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    // Get the gpio
    const ThingClass deviceClass = supportedThings().findById(thing->thingClassId());
    Gpio *gpio = Q_NULLPTR;

    // Find the gpio in the corresponding hash
    if (deviceClass.vendorId() == raspberryPiVendorId)
        gpio = m_raspberryPiGpios.value(thing->paramValue(gpioOutputRpiThingGpioParamTypeId).toInt());

    if (deviceClass.vendorId() == beagleboneBlackVendorId)
        gpio = m_beagleboneBlackGpios.value(thing->paramValue(gpioOutputBbbThingGpioParamTypeId).toInt());

    // Check if gpio was found
    if (!gpio) {
        qCWarning(dcGpioController()) << "Could not find gpio for executing action on" << thing->name();
        //: Error executing GPIO action
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("GPIO not found"));
    }

    // GPIO Switch power action
    if (deviceClass.vendorId() == raspberryPiVendorId) {
        if (action.actionTypeId() == gpioOutputRpiPowerActionTypeId) {
            bool success = false;
            if (action.param(gpioOutputRpiPowerActionPowerParamTypeId).value().toBool()) {
                success = gpio->setValue(Gpio::ValueHigh);
            } else {
                success = gpio->setValue(Gpio::ValueLow);
            }

            if (!success) {
                qCWarning(dcGpioController()) << "Could not set gpio value while execute action on" << thing->name();
                //: Error executing GPIO action
                return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Setting GPIO value failed."));
            }

            // Set the current state
            thing->setStateValue(gpioOutputRpiPowerStateTypeId, action.param(gpioOutputRpiPowerActionPowerParamTypeId).value());

            return info->finish(Thing::ThingErrorNoError);
        }
    } else if (deviceClass.vendorId() == beagleboneBlackVendorId) {
        if (action.actionTypeId() == gpioOutputBbbPowerActionTypeId) {
            bool success = false;
            if (action.param(gpioOutputBbbPowerActionPowerParamTypeId).value().toBool()) {
                success = gpio->setValue(Gpio::ValueHigh);
            } else {
                success = gpio->setValue(Gpio::ValueLow);
            }

            if (!success) {
                qCWarning(dcGpioController()) << "Could not set gpio value while execute action on" << thing->name();
                //: Error executing GPIO action
                return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Setting GPIO value failed."));
            }

            // Set the current state
            thing->setStateValue(gpioOutputBbbPowerStateTypeId, action.param(gpioOutputBbbPowerActionPowerParamTypeId).value());

            info->finish(Thing::ThingErrorNoError);
        }
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginGpio::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == gpioOutputRpiThingClassId || thing->thingClassId() == gpioOutputBbbThingClassId) {
        Gpio *gpio = m_gpioDevices.key(thing);
        if (!gpio)
            return;

        gpio->setValue(Gpio::ValueLow);
        if (thing->thingClassId() == gpioOutputRpiThingClassId) {
            thing->setStateValue(gpioOutputRpiPowerStateTypeId, false);
        }
        if (thing->thingClassId() == gpioOutputBbbThingClassId) {
            thing->setStateValue(gpioOutputBbbPowerStateTypeId, false);
        }
    }

    if (thing->thingClassId() == gpioInputRpiThingClassId || thing->thingClassId() == gpioInputBbbThingClassId) {
        GpioMonitor *monitor = m_monitorDevices.key(thing);
        if (!monitor)
            return;

        if (thing->thingClassId() == gpioInputRpiThingClassId) {
            thing->setStateValue(gpioInputRpiPowerStateTypeId, monitor->value());
        } else if (thing->thingClassId() == gpioInputBbbThingClassId) {
            thing->setStateValue(gpioInputBbbPowerStateTypeId, monitor->value());
        }
    }

    if (thing->thingClassId() == counterRpiThingClassId || thing->thingClassId() == counterBbbThingClassId) {
        if (!m_counterTimer) {
            m_counterTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
            connect(m_counterTimer, &PluginTimer::timeout, this, [this] (){

                foreach (Thing *thing, myThings()) {
                    if (thing->thingClassId() == counterRpiThingClassId) {
                        int counterValue = m_counterValues.value(thing->id());
                        thing->setStateValue(counterRpiCounterStateTypeId, counterValue);
                        m_counterValues[thing->id()] = 0;
                    }
                    if (thing->thingClassId() == counterBbbThingClassId) {
                        int counterValue = m_counterValues.value(thing->id());
                        thing->setStateValue(counterBbbCounterStateTypeId, counterValue);
                    }
                }
            });
        }
    }
}

QList<GpioDescriptor> IntegrationPluginGpio::raspberryPiGpioDescriptors()
{
    // Note: http://www.raspberrypi-spy.co.uk/wp-content/uploads/2012/06/Raspberry-Pi-GPIO-Layout-Model-B-Plus-rotated-2700x900.png
    QList<GpioDescriptor> gpioDescriptors;
    gpioDescriptors << GpioDescriptor(2, 3, "SDA1_I2C");
    gpioDescriptors << GpioDescriptor(3, 5, "SCL1_I2C");
    gpioDescriptors << GpioDescriptor(4, 7);
    gpioDescriptors << GpioDescriptor(5, 29);
    gpioDescriptors << GpioDescriptor(6, 31);
    gpioDescriptors << GpioDescriptor(7, 26, "SPI0_CE1_N");
    gpioDescriptors << GpioDescriptor(8, 24, "SPI0_CE0_N");
    gpioDescriptors << GpioDescriptor(9, 21, "SPI0_MISO");
    gpioDescriptors << GpioDescriptor(10, 19, "SPI0_MOSI");
    gpioDescriptors << GpioDescriptor(11, 23, "SPI0_SCLK");
    gpioDescriptors << GpioDescriptor(12, 32);
    gpioDescriptors << GpioDescriptor(13, 33);
    gpioDescriptors << GpioDescriptor(14, 8, "UART0_TXD");
    gpioDescriptors << GpioDescriptor(15, 10, "UART0_RXD");
    gpioDescriptors << GpioDescriptor(16, 36);
    gpioDescriptors << GpioDescriptor(17, 11);
    gpioDescriptors << GpioDescriptor(18, 12, "PCM_CLK");
    gpioDescriptors << GpioDescriptor(19, 35);
    gpioDescriptors << GpioDescriptor(20, 38);
    gpioDescriptors << GpioDescriptor(21, 40);
    gpioDescriptors << GpioDescriptor(22, 15);
    gpioDescriptors << GpioDescriptor(23, 16);
    gpioDescriptors << GpioDescriptor(24, 18);
    gpioDescriptors << GpioDescriptor(25, 22);
    gpioDescriptors << GpioDescriptor(26, 37);
    gpioDescriptors << GpioDescriptor(27, 13);
    return gpioDescriptors;
}

QList<GpioDescriptor> IntegrationPluginGpio::beagleboneBlackGpioDescriptors()
{
    // Note: https://www.mathworks.com/help/examples/beaglebone_product/beaglebone_black_gpio_pinmap.png
    QList<GpioDescriptor> gpioDescriptors;
    gpioDescriptors << GpioDescriptor(2, 22, "P9");
    gpioDescriptors << GpioDescriptor(3, 21, "P9");
    gpioDescriptors << GpioDescriptor(4, 18, "P9 - I2C1_SDA");
    gpioDescriptors << GpioDescriptor(5, 17, "P9 - I2C1_SCL");
    gpioDescriptors << GpioDescriptor(7, 42, "P9");
    gpioDescriptors << GpioDescriptor(12, 20, "P9 - I2C2_SDA");
    gpioDescriptors << GpioDescriptor(13, 19, "P9 - I2C2_SCL");
    gpioDescriptors << GpioDescriptor(14, 26, "P9");
    gpioDescriptors << GpioDescriptor(15, 24, "P9");
    gpioDescriptors << GpioDescriptor(20, 41, "P9");
    gpioDescriptors << GpioDescriptor(30, 11, "P9");
    gpioDescriptors << GpioDescriptor(31, 13, "P9");
    gpioDescriptors << GpioDescriptor(48, 15, "P9");
    gpioDescriptors << GpioDescriptor(49, 23, "P9");
    gpioDescriptors << GpioDescriptor(50, 14, "P9");
    gpioDescriptors << GpioDescriptor(51, 16, "P9");
    gpioDescriptors << GpioDescriptor(60, 12, "P9");
    gpioDescriptors << GpioDescriptor(117, 25, "P9");
    gpioDescriptors << GpioDescriptor(120, 31, "P9");
    gpioDescriptors << GpioDescriptor(121, 29, "P9");
    gpioDescriptors << GpioDescriptor(122, 30, "P9");
    gpioDescriptors << GpioDescriptor(123, 28, "P9");

    gpioDescriptors << GpioDescriptor(8, 35, "P8 - LCD_DATA12");
    gpioDescriptors << GpioDescriptor(9, 33, "P8 - LCD_DATA13");
    gpioDescriptors << GpioDescriptor(10, 31, "P8 - LCD_DATA14");
    gpioDescriptors << GpioDescriptor(11, 32, "P8 - LCD_DATA15");
    gpioDescriptors << GpioDescriptor(22, 19, "P8");
    gpioDescriptors << GpioDescriptor(23, 13, "P8");
    gpioDescriptors << GpioDescriptor(26, 14, "P8");
    gpioDescriptors << GpioDescriptor(27, 17, "P8");
    gpioDescriptors << GpioDescriptor(32, 25, "P8 - MMC1-DAT0");
    gpioDescriptors << GpioDescriptor(33, 24, "P8 - MMC1_DAT1");
    gpioDescriptors << GpioDescriptor(34, 5, "P8 - MMC1_DAT2");
    gpioDescriptors << GpioDescriptor(35, 6, "P8 - MMC1_DAT3");
    gpioDescriptors << GpioDescriptor(36, 23, "P8 - MMC1-DAT4");
    gpioDescriptors << GpioDescriptor(37, 22, "P8 - MMC1_DAT5");
    gpioDescriptors << GpioDescriptor(38, 3, "P8 - MMC1_DAT6");
    gpioDescriptors << GpioDescriptor(39, 4, "P8 - MMC1_DAT7");
    gpioDescriptors << GpioDescriptor(44, 12, "P8");
    gpioDescriptors << GpioDescriptor(45, 11, "P8");
    gpioDescriptors << GpioDescriptor(46, 16, "P8");
    gpioDescriptors << GpioDescriptor(47, 15, "P8");
    gpioDescriptors << GpioDescriptor(61, 26, "P8");
    gpioDescriptors << GpioDescriptor(62, 21, "P8 - MMC1-CLK");
    gpioDescriptors << GpioDescriptor(63, 20, "P8 - MMC1_CMD");
    gpioDescriptors << GpioDescriptor(65, 18, "P8");
    gpioDescriptors << GpioDescriptor(66, 7, "P8");
    gpioDescriptors << GpioDescriptor(67, 8, "P8");
    gpioDescriptors << GpioDescriptor(68, 10, "P8");
    gpioDescriptors << GpioDescriptor(69, 9, "P8");
    gpioDescriptors << GpioDescriptor(70, 45, "P8 - LCD_DATA0");
    gpioDescriptors << GpioDescriptor(71, 46, "P8 - LCD_DATA1");
    gpioDescriptors << GpioDescriptor(72, 43, "P8 - LCD_DATA2");
    gpioDescriptors << GpioDescriptor(73, 44, "P8 - LCD_DATA3");
    gpioDescriptors << GpioDescriptor(74, 41, "P8 - LCD_DATA4");
    gpioDescriptors << GpioDescriptor(75, 42, "P8 - LCD_DATA5");
    gpioDescriptors << GpioDescriptor(76, 39, "P8 - LCD_DATA6");
    gpioDescriptors << GpioDescriptor(77, 40, "P8 - LCD_DATA7");
    gpioDescriptors << GpioDescriptor(78, 37, "P8 - LCD_DATA8");
    gpioDescriptors << GpioDescriptor(79, 38, "P8 - LCD_DATA9");
    gpioDescriptors << GpioDescriptor(80, 36, "P8 - LCD_DATA10");
    gpioDescriptors << GpioDescriptor(81, 34, "P8 - LCD_DATA11");
    gpioDescriptors << GpioDescriptor(86, 27, "P8 - LCD_VSYNC");
    gpioDescriptors << GpioDescriptor(87, 29, "P8 - LCD_HSYNC");
    gpioDescriptors << GpioDescriptor(88, 28, "P8 - LCD_PCLK");
    gpioDescriptors << GpioDescriptor(89, 30, "P8 - LCD_AC_BIAS_E");
    return gpioDescriptors;
}

void IntegrationPluginGpio::onGpioValueChanged(const bool &value)
{
    GpioMonitor *monitor = static_cast<GpioMonitor *>(sender());

    Thing *thing = m_monitorDevices.value(monitor);
    if (!thing)
        return;

    if (thing->thingClassId() == gpioInputRpiThingClassId) {
        thing->setStateValue(gpioInputRpiPowerStateTypeId, value);
    } else if (thing->thingClassId() == gpioInputBbbThingClassId) {
        thing->setStateValue(gpioInputBbbPowerStateTypeId, value);
    } else if (thing->thingClassId() == counterRpiThingClassId || thing->thingClassId() == counterBbbThingClassId) {
        if (value) {
            m_counterValues[thing->id()] += 1;
        }
    }
}
