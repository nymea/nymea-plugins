/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "deviceplugingpio.h"
#include "types/param.h"
#include "devices/device.h"
#include "plugininfo.h"

DevicePluginGpio::DevicePluginGpio()
{
}

void DevicePluginGpio::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    qCDebug(dcGpioController()) << "Setup" << device->name() << device->params();

    // Check if GPIOs are available on this platform
    if (!Gpio::isAvailable()) {
        qCWarning(dcGpioController()) << "There are ou GPIOs on this plattform";
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("No GPIOs found on this system."));
    }

    // GPIO Switch
    if (device->deviceClassId() == gpioOutputRpiDeviceClassId || device->deviceClassId() == gpioOutputBbbDeviceClassId) {
        // Create and configure gpio
        int gpioId = -1;
        if (device->deviceClassId() == gpioOutputRpiDeviceClassId)
            gpioId = device->paramValue(gpioOutputRpiDeviceGpioParamTypeId).toInt();

        if (device->deviceClassId() == gpioOutputBbbDeviceClassId)
            gpioId = device->paramValue(gpioOutputBbbDeviceGpioParamTypeId).toInt();

        Gpio *gpio = new Gpio(gpioId, this);

        if (!gpio->exportGpio()) {
            qCWarning(dcGpioController()) << "Could not export gpio for device" << device->name();
            return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Exporting GPIO failed."));
        }

        if (!gpio->setDirection(Gpio::DirectionOutput)) {
            qCWarning(dcGpioController()) << "Could not configure output gpio for device" << device->name();
            return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Configuring output GPIO failed."));
        }

        if (!gpio->setValue(Gpio::ValueLow)) {
            qCWarning(dcGpioController()) << "Could not set gpio  value for device" << device->name();
            return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Setting GPIO value failed."));
        }

        m_gpioDevices.insert(gpio, device);

        if (device->deviceClassId() == gpioOutputRpiDeviceClassId)
            m_raspberryPiGpios.insert(gpio->gpioNumber(), gpio);

        if (device->deviceClassId() == gpioOutputBbbDeviceClassId)
            m_beagleboneBlackGpios.insert(gpio->gpioNumber(), gpio);

        return info->finish(Device::DeviceErrorNoError);
    }

    if (device->deviceClassId() == gpioInputRpiDeviceClassId || device->deviceClassId() == gpioInputBbbDeviceClassId) {

        int gpioId = -1;
        if (device->deviceClassId() == gpioInputRpiDeviceClassId)
            gpioId = device->paramValue(gpioInputRpiDeviceGpioParamTypeId).toInt();

        if (device->deviceClassId() == gpioInputBbbDeviceClassId)
            gpioId = device->paramValue(gpioInputBbbDeviceGpioParamTypeId).toInt();

        GpioMonitor *monitor = new GpioMonitor(gpioId, this);

        if (!monitor->enable()) {
            qCWarning(dcGpioController()) << "Could not enable gpio monitor for device" << device->name();
            return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Enabling GPIO monitor failed."));
        }

        connect(monitor, &GpioMonitor::valueChanged, this, &DevicePluginGpio::onGpioValueChanged);

        m_monitorDevices.insert(monitor, device);

        if (device->deviceClassId() == gpioInputRpiDeviceClassId)
            m_raspberryPiGpioMoniors.insert(monitor->gpio()->gpioNumber(), monitor);

        if (device->deviceClassId() == gpioInputBbbDeviceClassId)
            m_beagleboneBlackGpioMoniors.insert(monitor->gpio()->gpioNumber(), monitor);

        return info->finish(Device::DeviceErrorNoError);
    }

    if (device->deviceClassId() == counterRpiDeviceClassId || device->deviceClassId() == counterBbbDeviceClassId) {

        int gpioId = -1;
        if (device->deviceClassId() == counterRpiDeviceClassId)
            gpioId = device->paramValue(counterRpiDeviceGpioParamTypeId).toInt();

        if (device->deviceClassId() == counterBbbDeviceClassId)
            gpioId = device->paramValue(counterBbbDeviceGpioParamTypeId).toInt();

        GpioMonitor *monitor = new GpioMonitor(gpioId, this);

        if (!monitor->enable()) {
            qCWarning(dcGpioController()) << "Could not enable gpio monitor for device" << device->name();
            return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Enabling GPIO monitor failed."));
        }

        connect(monitor, &GpioMonitor::valueChanged, this, &DevicePluginGpio::onGpioValueChanged);

        m_monitorDevices.insert(monitor, device);

        if (device->deviceClassId() == counterRpiDeviceClassId)
            m_raspberryPiGpioMoniors.insert(monitor->gpio()->gpioNumber(), monitor);

        if (device->deviceClassId() == counterBbbDeviceClassId)
            m_beagleboneBlackGpioMoniors.insert(monitor->gpio()->gpioNumber(), monitor);

        m_counterValues.insert(device->id(), 0);
        return info->finish(Device::DeviceErrorNoError);
    }
    return info->finish(Device::DeviceErrorNoError);
}

void DevicePluginGpio::discoverDevices(DeviceDiscoveryInfo *info)
{
    DeviceClassId deviceClassId = info->deviceClassId();

    // Check if GPIOs are available on this platform
    if (!Gpio::isAvailable()) {
        qCWarning(dcGpioController()) << "There are no GPIOs on this plattform";
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("No GPIOs available on this system."));
    }

    // Check which board / gpio configuration
    const DeviceClass deviceClass = supportedDevices().findById(deviceClassId);
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

            DeviceDescriptor descriptor(deviceClassId, QString("GPIO %1").arg(gpioDescriptor.gpio()), description);
            ParamList parameters;
            if (deviceClass.id() == gpioOutputRpiDeviceClassId) {
                parameters.append(Param(gpioOutputRpiDeviceGpioParamTypeId, gpioDescriptor.gpio()));
                parameters.append(Param(gpioOutputRpiDevicePinParamTypeId, gpioDescriptor.pin()));
                parameters.append(Param(gpioOutputRpiDeviceDescriptionParamTypeId, gpioDescriptor.description()));
            } else if (deviceClass.id() == gpioInputRpiDeviceClassId) {
                parameters.append(Param(gpioInputRpiDeviceGpioParamTypeId, gpioDescriptor.gpio()));
                parameters.append(Param(gpioInputRpiDevicePinParamTypeId, gpioDescriptor.pin()));
                parameters.append(Param(gpioInputRpiDeviceDescriptionParamTypeId, gpioDescriptor.description()));
            }
            descriptor.setParams(parameters);

            foreach (Device *existingDevice, myDevices()) {
                if (existingDevice->paramValue(gpioOutputRpiDeviceGpioParamTypeId).toInt() == gpioDescriptor.gpio()) {
                    descriptor.setDeviceId(existingDevice->id());
                    break;
                }
            }
            info->addDeviceDescriptor(descriptor);
        }

        return info->finish(Device::DeviceErrorNoError);
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

            DeviceDescriptor descriptor(deviceClassId, QString("GPIO %1").arg(gpioDescriptor.gpio()), description);
            ParamList parameters;
            if (deviceClass.id() == gpioOutputBbbDeviceClassId) {
                parameters.append(Param(gpioOutputBbbDeviceGpioParamTypeId, gpioDescriptor.gpio()));
                parameters.append(Param(gpioOutputBbbDevicePinParamTypeId, gpioDescriptor.pin()));
                parameters.append(Param(gpioOutputBbbDeviceDescriptionParamTypeId, gpioDescriptor.description()));
            } else if (deviceClass.id() == gpioInputBbbDeviceClassId) {
                parameters.append(Param(gpioInputBbbDeviceGpioParamTypeId, gpioDescriptor.gpio()));
                parameters.append(Param(gpioInputBbbDevicePinParamTypeId, gpioDescriptor.pin()));
                parameters.append(Param(gpioInputBbbDeviceDescriptionParamTypeId, gpioDescriptor.description()));
            }
            descriptor.setParams(parameters);

            info->addDeviceDescriptor(descriptor);
        }

        return info->finish(Device::DeviceErrorNoError);
    }
}

void DevicePluginGpio::deviceRemoved(Device *device)
{
    if (m_gpioDevices.values().contains(device)) {
        Gpio *gpio = m_gpioDevices.key(device);
        if (!gpio)
            return;

        m_gpioDevices.remove(gpio);

        if (m_raspberryPiGpios.values().contains(gpio))
            m_raspberryPiGpios.remove(gpio->gpioNumber());

        if (m_beagleboneBlackGpios.values().contains(gpio))
            m_beagleboneBlackGpios.remove(gpio->gpioNumber());

        delete gpio;
    }

    if (m_monitorDevices.values().contains(device)) {
        GpioMonitor *monitor = m_monitorDevices.key(device);
        if (!monitor)
            return;

        m_monitorDevices.remove(monitor);

        if (m_raspberryPiGpioMoniors.values().contains(monitor))
            m_raspberryPiGpios.remove(monitor->gpio()->gpioNumber());

        if (m_beagleboneBlackGpioMoniors.values().contains(monitor))
            m_beagleboneBlackGpioMoniors.remove(monitor->gpio()->gpioNumber());

        delete monitor;
    }

    if (m_longPressTimers.contains(device)) {
        QTimer *timer = m_longPressTimers.take(device);
        timer->stop();
        timer->deleteLater();
    }

    if (m_counterValues.contains(device->id())) {
        m_counterValues.remove(device->id());
    }

    if (myDevices().filterByDeviceClassId(counterRpiDeviceClassId).isEmpty() && myDevices().filterByDeviceClassId(counterBbbDeviceClassId).isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_counterTimer);
        m_counterTimer = nullptr;
    }
}

void DevicePluginGpio::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    // Get the gpio
    const DeviceClass deviceClass = supportedDevices().findById(device->deviceClassId());
    Gpio *gpio = Q_NULLPTR;

    // Find the gpio in the corresponding hash
    if (deviceClass.vendorId() == raspberryPiVendorId)
        gpio = m_raspberryPiGpios.value(device->paramValue(gpioOutputRpiDeviceGpioParamTypeId).toInt());

    if (deviceClass.vendorId() == beagleboneBlackVendorId)
        gpio = m_beagleboneBlackGpios.value(device->paramValue(gpioOutputBbbDeviceGpioParamTypeId).toInt());

    // Check if gpio was found
    if (!gpio) {
        qCWarning(dcGpioController()) << "Could not find gpio for executing action on" << device->name();
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("GPIO not found"));
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
                qCWarning(dcGpioController()) << "Could not set gpio value while execute action on" << device->name();
                return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Setting GPIO value failed."));
            }

            // Set the current state
            device->setStateValue(gpioOutputRpiPowerStateTypeId, action.param(gpioOutputRpiPowerActionPowerParamTypeId).value());

            return info->finish(Device::DeviceErrorNoError);
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
                qCWarning(dcGpioController()) << "Could not set gpio value while execute action on" << device->name();
                return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Setting GPIO value failed."));
            }

            // Set the current state
            device->setStateValue(gpioOutputBbbPowerStateTypeId, action.param(gpioOutputBbbPowerActionPowerParamTypeId).value());

            info->finish(Device::DeviceErrorNoError);
        }
    }

    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginGpio::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == gpioOutputRpiDeviceClassId || device->deviceClassId() == gpioOutputBbbDeviceClassId) {
        Gpio *gpio = m_gpioDevices.key(device);
        if (!gpio)
            return;

        gpio->setValue(Gpio::ValueLow);
        if (device->deviceClassId() == gpioOutputRpiDeviceClassId) {
            device->setStateValue(gpioOutputRpiPowerStateTypeId, false);
        }
        if (device->deviceClassId() == gpioOutputBbbDeviceClassId) {
            device->setStateValue(gpioOutputBbbPowerStateTypeId, false);
        }
    }

    if (device->deviceClassId() == gpioInputRpiDeviceClassId || device->deviceClassId() == gpioInputBbbDeviceClassId) {
        QTimer *timer = new QTimer(this);
        timer->setSingleShot(true);
        m_longPressTimers.insert(device, timer);

        GpioMonitor *monitor = m_monitorDevices.key(device);
        if (!monitor)
            return;

        if (device->deviceClassId() == gpioInputRpiDeviceClassId) {
            device->setStateValue(gpioInputRpiPressedStateTypeId, monitor->value());
        } else if (device->deviceClassId() == gpioInputBbbDeviceClassId) {
            device->setStateValue(gpioInputBbbPressedStateTypeId, monitor->value());
        }
    }

    if (device->deviceClassId() == counterRpiDeviceClassId || device->deviceClassId() == counterBbbDeviceClassId) {
        if (!m_counterTimer) {
            m_counterTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
            connect(m_counterTimer, &PluginTimer::timeout, this, [this] (){

                foreach (Device *device, myDevices()) {
                    if (device->deviceClassId() == counterRpiDeviceClassId) {
                        int counterValue = m_counterValues.value(device->id());
                        device->setStateValue(counterRpiCounterStateTypeId, counterValue);
                        m_counterValues[device->id()] = 0;
                    }
                    if (device->deviceClassId() == counterBbbDeviceClassId) {
                        int counterValue = m_counterValues.value(device->id());
                        device->setStateValue(counterBbbCounterStateTypeId, counterValue);
                    }
                }
            });
        }
    }
}

QList<GpioDescriptor> DevicePluginGpio::raspberryPiGpioDescriptors()
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

QList<GpioDescriptor> DevicePluginGpio::beagleboneBlackGpioDescriptors()
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

void DevicePluginGpio::onGpioValueChanged(const bool &value)
{
    GpioMonitor *monitor = static_cast<GpioMonitor *>(sender());

    Device *device = m_monitorDevices.value(monitor);
    if (!device)
        return;

    if (device->deviceClassId() == gpioInputRpiDeviceClassId) {
        device->setStateValue(gpioInputRpiPressedStateTypeId, value);
        //start longpresss timer
        QTimer *timer = m_longPressTimers.value(device);
        if (!timer){
            qWarning(dcGpioController()) << "Long press timer not available";
            return;
        }
        if (value) {
            int seconds = configValue( gpioControllerPluginLongPressTimeParamTypeId).toInt();
            timer->start(seconds * 1000);
        } else {
            if (timer->isActive()) {
                timer->stop();
                //emit timer pressed
            }
        }
    } else if (device->deviceClassId() == gpioInputBbbDeviceClassId) {
        device->setStateValue(gpioInputBbbPressedStateTypeId, value);
        //start longpresss timer
        QTimer *timer = m_longPressTimers.value(device);
        if (!timer){
            qWarning(dcGpioController()) << "Long press timer not available";
            return;
        }
        if (value) {
            int seconds = configValue( gpioControllerPluginLongPressTimeParamTypeId).toInt();
            timer->start(seconds * 1000);
        } else {
            if (timer->isActive()) {
                timer->stop();
                //emit timer pressed
            }
        }
    } else if (device->deviceClassId() == counterRpiDeviceClassId || device->deviceClassId() == counterBbbDeviceClassId) {
        if (value) {
            m_counterValues[device->id()] += 1;
        }
    }
}


void DevicePluginGpio::onLongPressedTimeout()
{
    QTimer *timer = static_cast<QTimer *>(sender());
    qCDebug(dcGpioController()) << "Button long pressed";
    timer->stop();
    Device *device = m_longPressTimers.key(timer);
    if (!device)
        return;

    if (device->deviceClassId() == gpioInputRpiDeviceClassId){
        emitEvent(Event(gpioInputRpiLongPressedEventTypeId, device->id()));
    } else if (device->deviceClassId() == gpioInputBbbDeviceClassId){
        emitEvent(Event(gpioInputBbbLongPressedEventTypeId, device->id()));
    }
}

