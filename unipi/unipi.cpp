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

#include "unipi.h"
#include "extern-plugininfo.h"


UniPi::UniPi(UniPiType unipiType, QObject *parent) :
    QObject(parent),
    m_unipiType(unipiType)
{
    m_mcp23008 = new MCP23008("i2c-1", 0x20, this);
}

UniPi::~UniPi()
{
    m_mcp23008->disable();
    m_mcp23008->deleteLater();
}

bool UniPi::init()
{
    if (m_mcp23008->enable()) {
        m_mcp23008->writeRegister(MCP23008::RegisterAddress::IODIR, 0x00); //set all pins as outputs
        m_mcp23008->writeRegister(MCP23008::RegisterAddress::IPOL, 0x00);  //set all pins to non inverted mode 1 = high
        m_mcp23008->writeRegister(MCP23008::RegisterAddress::GPPU, 0x00);  //disable all pull up resistors
        m_mcp23008->writeRegister(MCP23008::RegisterAddress::OLAT, 0x00);  //Set all outputs to low
        return true;
    }
    return false;
}

QString UniPi::type()
{
    QString type;
    switch (m_unipiType) {
    case UniPiType::UniPi1:
        type = "UniPi 1";
        break;
    case UniPiType::UniPi1Lite:
        type = "UniPi 1 Lite";
        break;
    }
    return type;
}

QList<QString> UniPi::digitalInputs()
{
    QList<QString> inputs;
    switch (m_unipiType) {
    case UniPiType::UniPi1:
        for (int i = 0; i < 13; ++i) {
            inputs.append(QString("DI%1").arg(i));
        }
        break;
    case UniPiType::UniPi1Lite:
        for (int i = 0; i < 7; ++i) {
            inputs.append(QString("DI%1").arg(i));
        }
        break;
    }
    return inputs;
}

QList<QString> UniPi::digitalOutputs()
{
    QList<QString> outputs;
    switch (m_unipiType) {
    case UniPiType::UniPi1:
        for (int i = 0; i < 6; ++i) {
            outputs.append(QString("DO%1").arg(i));
        }
        break;
    case UniPiType::UniPi1Lite:
        for (int i = 0; i < 6; ++i) {
            outputs.append(QString("DO%1").arg(i));
        }
        break;
    }
    return outputs;
}

QList<QString> UniPi::analogInputs()
{
    QList<QString> inputs;
    switch (m_unipiType) {
    case UniPiType::UniPi1:
        for (int i = 0; i < 2; ++i) {
            inputs.append(QString("AI%1").arg(i));
        }
        break;
    case UniPiType::UniPi1Lite:
        for (int i = 0; i < 2; ++i) {
            inputs.append(QString("AI%1").arg(i));
        }
        break;
    }
    return inputs;
}

QList<QString> UniPi::analogOutputs()
{
    QList<QString> outputs;
    switch (m_unipiType) {
    case UniPiType::UniPi1:
        for (int i = 0; i < 1; ++i) {
            outputs.append(QString("AO%1").arg(i));
        }
        break;
    case UniPiType::UniPi1Lite:
        for (int i = 0; i < 1; ++i) {
            outputs.append(QString("AO%1").arg(i));
        }
        break;
    }
    return outputs;
}


QList<GpioDescriptor> UniPi::raspberryPiGpioDescriptors()
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

void UniPi::setOutput(int pin, bool status)
{
    Q_UNUSED(pin)
    Q_UNUSED(status)
    //read output register
    if (pin > 7)
        return;
    //set bit

    //write output register
    m_mcp23008->writeRegister(MCP23008::RegisterAddress::OLAT, (static_cast<uint8_t>(status) << pin));
}

bool UniPi::getOutput(int pin)
{
    if (pin > 7)
        return false;

    uint8_t registerValue;
    registerValue = m_mcp23008->readRegister(MCP23008::RegisterAddress::OLAT);
    return ( registerValue & (static_cast<uint8_t>(registerValue) << pin)) ;
}

bool UniPi::getInput(int pin)
{
    Q_UNUSED(pin)
    //Read RPi pins
    return true;
}

void UniPi::onGpioValueChanged(const bool &value)
{
    GpioMonitor *monitor = static_cast<GpioMonitor *>(sender());
    QString circuit;
    switch (monitor->gpio()->gpioNumber()) {
    case 4: //DI01 GPIO04 Digital input
        circuit = "DI01";
        break;
    case 17: //DI02 GPIO17 Digital input
        circuit = "DI02";
        break;
    case 27: //DI03 GPIO27 Digital input
        circuit = "DI03";
        break;
    case 23: //DI04 GPIO23 Digital input
        circuit = "DI04";
        break;
    case 22: //DI05 GPIO22 Digital input
        circuit = "DI05";
        break;
    case 24: //DI06 GPIO24 Digital input
        circuit = "DI06";
        break;
    case 11: //I07 GPIO11 Digital Input
        circuit = "DI07";
        break;
    case 7: //I08 GPIO07 Digital Input
        circuit = "DI08";
        break;
    case 8: //I09 GPIO08 Digital Input
        circuit = "DI09";
        break;
    case 9: //I10 GPIO09 Digital Input
        circuit = "DI10";
        break;
    case 25: //I11 GPIO25 Digital Input
        circuit = "DI11";
        break;
    case 10: //DI12 GPIO10 Digital input
        circuit = "DI12";
        break;
    case 31: //DI13 GPIO31 Digital input
        circuit = "DI13";
        break;
    case 30: //DI14 GPIO30 Digital input
        circuit = "DI14";
        break;

    default:
        return;
    }
    emit digitalInputStatusChanged(circuit, value);
}
