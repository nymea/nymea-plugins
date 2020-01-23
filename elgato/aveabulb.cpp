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

#include "aveabulb.h"
#include "extern-plugininfo.h"

#include <QDataStream>

AveaBulb::AveaBulb(Device *device, BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent) :
    QObject(parent),
    m_device(device),
    m_bluetoothDevice(bluetoothDevice)
{
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::connectedChanged, this, &AveaBulb::onConnectedChanged);
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::servicesDiscoveryFinished, this, &AveaBulb::onServiceDiscoveryFinished);
}

Device *AveaBulb::device()
{
    return m_device;
}

BluetoothLowEnergyDevice *AveaBulb::bluetoothDevice()
{
    return m_bluetoothDevice;
}

bool AveaBulb::setPower(bool power)
{
    if (!m_bluetoothDevice->connected())
        return false;

    if (!m_colorService)
        return false;

    qCDebug(dcElgato()) << "Set power" << (power ? "on" : "off");

    // Sync current color
    if (power)
        return syncColor();

    // Power off
    QByteArray command;
    QDataStream stream(&command, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint8 commandId = static_cast<quint8>(ColorMessageColor);
    quint16 fade = static_cast<quint16>(m_fade);
    quint16 constant = 0x000a;
    quint16 white = 0 | 0x8000;
    quint16 red = 0  | 0x3000;
    quint16 green = 0 | 0x2000;
    quint16 blue = 0 | 0x1000;

    stream << commandId << fade << constant << white << red << green << blue;

    qCDebug(dcElgato()) << "Set color data -->" << command.toHex();

    m_colorService->writeCharacteristic(m_colorCharacteristic, command);
    return true;

}

bool AveaBulb::setBulbName(const QString &name)
{
    m_bulbName = name;

    // TODO:
    return true;
}

bool AveaBulb::setColor(const QColor &color)
{
    if (!m_bluetoothDevice->connected())
        return false;

    if (!m_colorService)
        return false;

    qCDebug(dcElgato()) << "-->" << color.toRgb();

    // Convert rgb to wrgb
    QByteArray command;
    QDataStream stream(&command, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    m_red = scaleColorValueUp(color.red());
    m_green = scaleColorValueUp(color.green());
    m_blue = scaleColorValueUp(color.blue());
    m_white = scaleColorValueUp(color.alpha());

    m_color = color;

    return syncColor();
}

bool AveaBulb::setBrightness(int percentage)
{
    if (!m_bluetoothDevice->connected())
        return false;

    if (!m_colorService)
        return false;

    quint16 brightnessValue = qRound(4095.0 * percentage / 100.0);
    qCDebug(dcElgato()) << "Brightness value" << percentage << "% -->" << brightnessValue;

    QByteArray command;
    QDataStream stream(&command, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << static_cast<quint8>(ColorMessageBrightness);
    stream << brightnessValue;

    m_colorService->writeCharacteristic(m_colorCharacteristic, command);
    return true;
}

bool AveaBulb::setFade(int fade)
{
    m_fade = fade;
    return syncColor();
}

bool AveaBulb::setWhite(int white)
{
    m_white = white;
    return syncColor();
}

bool AveaBulb::setRed(int red)
{
    m_red = red;
    return syncColor();
}

bool AveaBulb::setGreen(int green)
{
    m_green = green;
    return syncColor();
}

bool AveaBulb::setBlue(int blue)
{
    m_blue = blue;
    return syncColor();
}

bool AveaBulb::loadValues()
{
    if (!m_bluetoothDevice->connected())
        return false;

    if (!m_colorService)
        return false;

    // Request color
    m_colorService->writeCharacteristic(m_colorCharacteristic, QByteArray::fromHex("35"));

    // Request brightness
    m_colorService->writeCharacteristic(m_colorCharacteristic, QByteArray::fromHex("57"));

    // Request name
    m_colorService->writeCharacteristic(m_colorCharacteristic, QByteArray::fromHex("58"));
    return true;
}

quint16 AveaBulb::scaleColorValueUp(int colorValue)
{
    return static_cast<quint16>(qRound(4095.0 * colorValue / 255.0));
}

int AveaBulb::scaleColorValueDown(quint16 colorValue)
{
    return qRound(255.0 * colorValue / 4095.0);
}

bool AveaBulb::syncColor()
{
    if (!m_bluetoothDevice->connected())
        return false;

    if (!m_colorService)
        return false;

    if (!m_device->stateValue(aveaPowerStateTypeId).toBool()) {
        qCWarning(dcElgato()) << "Not syncing color because power off";
        return false;
    }

    // Convert rgb to wrgb
    QByteArray command;
    QDataStream stream(&command, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint8 commandId = static_cast<quint8>(ColorMessageColor);
    quint16 fade = static_cast<quint16>(m_fade);
    quint16 constant = 0x000a;
    quint16 white = m_white | 0x8000;
    quint16 red = m_red  | 0x3000;
    quint16 green = m_green | 0x2000;
    quint16 blue = m_blue | 0x1000;

    stream << commandId << fade << constant << white << red << green << blue;

    qCDebug(dcElgato()) << "----> Sync" << command.toHex();
    m_colorService->writeCharacteristic(m_colorCharacteristic, command);
    return true;
}

void AveaBulb::onConnectedChanged(const bool &connected)
{
    qCDebug(dcElgato()) << "Bulb" << m_bluetoothDevice->name() << m_bluetoothDevice->address().toString() << (connected ? "connected" : "disconnected");
    m_device->setStateValue(aveaConnectedStateTypeId, connected);

    if (!connected) {
        // Clean up services
        m_colorService->deleteLater();
        //m_imageService->deleteLater();

        m_colorService = nullptr;
        m_imageService = nullptr;
    }
}

void AveaBulb::onServiceDiscoveryFinished()
{
    qCDebug(dcElgato()) << "Service discovery finished";

    if (!m_bluetoothDevice->serviceUuids().contains(colorServiceUuid)) {
        qCWarning(dcElgato()) << "Could not find color service";
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(imageServiceUuid)) {
        qCWarning(dcElgato()) << "Could not find image service";
        return;
    }

    // Color service
    if (!m_colorService) {
        m_colorService = m_bluetoothDevice->controller()->createServiceObject(colorServiceUuid, this);
        if (!m_colorService) {
            qCWarning(dcElgato()) << "Could not create color service.";
            return;
        }

        connect(m_colorService, &QLowEnergyService::stateChanged, this, &AveaBulb::onColorServiceStateChanged);
        connect(m_colorService, &QLowEnergyService::characteristicChanged, this, &AveaBulb::onColorServiceCharacteristicChanged);

        m_colorService->discoverDetails();
    }

}

void AveaBulb::onColorServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcElgato()) << "Color service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_colorService->characteristics()) {
        qCDebug(dcElgato()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &descriptor, characteristic.descriptors()) {
            qCDebug(dcElgato()) << "        -->" << descriptor.name() << descriptor.uuid().toString() << descriptor.value();
        }
    }

    // Data characteristic
    m_colorCharacteristic = m_colorService->characteristic(colorCharacteristicUuid);
    if (!m_colorCharacteristic.isValid()) {
        qCWarning(dcElgato()) << "Invalid color data characteristic.";
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_colorCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_colorService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Load current configuration
    loadValues();
}

void AveaBulb::onColorServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() != colorCharacteristicUuid) {
        qCWarning(dcElgato()) << "Unhandled color service characteristic notification received" << value.toHex();
        return;
    }

    qCDebug(dcElgato()) << "Color characteristic changed" << characteristic.uuid().toString() << value.toHex();

    QByteArray payload = value;
    QDataStream stream(&payload, QIODevice::ReadOnly);
    quint16 messageType;
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> messageType;

    qCDebug(dcElgato()) << "Message type" << messageType << static_cast<ColorMessage>(messageType);

    switch (messageType) {
    case ColorMessageColor: {
        quint16 fade;
        quint16 whiteCurrentValue = 0; quint16 blueCurrentValue = 0; quint16 greenCurrentValue = 0; quint16 redCurrentValue = 0;
        quint16 whiteTargetValue = 0; quint16 blueTargetValue = 0; quint16 greenTargetValue = 0; quint16 redTargetValue = 0;

        stream >> fade;

        // Read current color
        stream >> whiteCurrentValue >> blueCurrentValue >> greenCurrentValue >> redCurrentValue;

        // Read target color
        stream >> whiteTargetValue >> blueTargetValue >> greenTargetValue >> redTargetValue;

        // Convert color values
        quint16 whiteCurrentAdjustedValue = whiteCurrentValue;
        quint16 blueCurrentAdjustedValue = blueCurrentValue ^ 0x1000;
        quint16 greenCurrentAdjustedValue = greenCurrentValue ^ 0x2000;
        quint16 redCurrentAdjustedValue = redCurrentValue ^ 0x3000;

        quint16 whiteTargetAdjustedValue = whiteTargetValue;
        quint16 blueTargetAdjustedValue = blueTargetValue ^ 0x1000;
        quint16 greenTargetAdjustedValue = greenTargetValue ^ 0x2000;
        quint16 redTargetAdjustedValue = redTargetValue ^ 0x3000;

        qCDebug(dcElgato()) << "Received color notification:";
        qCDebug(dcElgato()) << "    white (current):" << QString::number(whiteCurrentValue, 16) << whiteCurrentValue << whiteCurrentAdjustedValue;
        qCDebug(dcElgato()) << "    blue  (current):" << QString::number(blueCurrentValue, 16) << blueCurrentValue << blueCurrentAdjustedValue;
        qCDebug(dcElgato()) << "    green (current):" << QString::number(greenCurrentValue, 16) << greenCurrentValue << greenCurrentAdjustedValue;
        qCDebug(dcElgato()) << "    red   (current):" << QString::number(redCurrentValue, 16) << redCurrentValue << redCurrentAdjustedValue;
        qCDebug(dcElgato()) << "    white (target) :" << QString::number(whiteTargetValue, 16) << whiteTargetValue << whiteTargetAdjustedValue;
        qCDebug(dcElgato()) << "    blue  (target) :" << QString::number(blueTargetValue, 16) << blueTargetValue << blueTargetAdjustedValue;
        qCDebug(dcElgato()) << "    green (target) :" << QString::number(greenTargetValue, 16) << greenTargetValue << greenTargetAdjustedValue;
        qCDebug(dcElgato()) << "    red   (target) :" << QString::number(redTargetValue, 16) << redTargetValue << redTargetAdjustedValue;

        m_device->setStateValue(aveaFadeStateTypeId, fade);
        m_device->setStateValue(aveaWhiteStateTypeId, scaleColorValueDown(whiteTargetAdjustedValue));
        m_device->setStateValue(aveaRedStateTypeId, scaleColorValueDown(redTargetAdjustedValue));
        m_device->setStateValue(aveaGreenStateTypeId, scaleColorValueDown(greenTargetAdjustedValue));
        m_device->setStateValue(aveaBlueStateTypeId, scaleColorValueDown(blueTargetAdjustedValue));
        m_device->setStateValue(aveaColorStateTypeId, QColor(scaleColorValueDown(redTargetAdjustedValue), scaleColorValueDown(greenTargetAdjustedValue), scaleColorValueDown(blueTargetAdjustedValue)));

        break;
    }
    case ColorMessageBrightness: {
        quint16 brightnessValue = 0;
        stream >> brightnessValue;
        int brightnessPercentage = qRound(brightnessValue * 100 / 4095.0);
        qCDebug(dcElgato()) << "Brightness notification" << value.mid(1, 2).toHex() << brightnessValue << "-->" << brightnessPercentage << "%";
        m_device->setStateValue(aveaBrightnessStateTypeId, brightnessPercentage);
        break;
    }
    case ColorMessageName: {
        qCDebug(dcElgato()) << "Name notification:" << value.mid(1, value.count() - 2);
        break;
    }
    default:
        qCWarning(dcElgato()) << "Unhandled color characteristic notification received" << value.toHex();
        break;
    }

}

