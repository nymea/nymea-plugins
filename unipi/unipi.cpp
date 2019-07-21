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
    m_mcp3422 = new MCP3422("i2c-1", 0x68, this);
}

UniPi::~UniPi()
{
    m_mcp23008->deleteLater();

    m_mcp3422->disable();
    m_mcp3422->deleteLater();
}

bool UniPi::init()
{
    //init MCP23008 Outputs
    if (m_mcp23008->init()) {
        m_mcp23008->writeRegister(MCP23008::RegisterAddress::IODIR, 0x00); //set all pins as outputs
        m_mcp23008->writeRegister(MCP23008::RegisterAddress::IPOL, 0x00);  //set all pins to non inverted mode 1 = high
        m_mcp23008->writeRegister(MCP23008::RegisterAddress::GPPU, 0x00);  //disable all pull up resistors
        m_mcp23008->writeRegister(MCP23008::RegisterAddress::OLAT, 0x00);  //Set all outputs to low
        return true;
    }

    //Init Raspberry Pi Inputs
    foreach (QString circuit, digitalInputs()){
        int pin = getPinFromCircuit(circuit);
        GpioMonitor *gpio = new GpioMonitor(pin, this);
        gpio->enable();
        connect(gpio, &GpioMonitor::valueChanged, this, &UniPi::onInputValueChanged);
        m_monitorGpios.insert(gpio, circuit);
    }

    //Init Raspberry Pi PWM outputs
    foreach (QString circuit, analogOutputs()) {
        int pin = getPinFromCircuit(circuit);
        Pwm *pwm = new Pwm(pin, this);
        pwm->enable();
        pwm->setPolarity(Pwm::PolarityNormal);
        pwm->setFrequency(400);
        pwm->setPercentage(0);
        m_pwms.insert(pwm, circuit);
    }

    foreach (QString circuit, analogInputs()){
        int pin = getPinFromCircuit(circuit);
        Q_UNUSED(pin)
        //TODO Init Raspberry Pi Analog Input
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
        for (int i = 1; i < 15; ++i) {
            inputs.append(QString("DI%1").arg(i));
        }
        break;
    case UniPiType::UniPi1Lite:
        for (int i = 1; i < 7; ++i) {
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
        for (int i = 1; i < 7; ++i) {
            outputs.append(QString("DO%1").arg(i));
        }
        break;
    case UniPiType::UniPi1Lite:
        for (int i = 1; i < 7; ++i) {
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

int UniPi::getPinFromCircuit(const QString &circuit)
{
    int pin = 0;
    if (circuit.startsWith("DI")) { //Raspberry Pi Input Pins
        switch (circuit.mid(2, 2).toInt()) {
        case 1: //DI01 GPIO04 Digital input
            pin = 4;
            break;
        case 2: //DI02 GPIO17 Digital input
            pin = 17;
            break;
        case 3: //DI03 GPIO27 Digital input
            pin = 27;
            break;
        case 4: //DI04 GPIO23 Digital input
            pin = 23;
            break;
        case 5: //DI05 GPIO22 Digital input
            pin = 22;
            break;
        case 6: //DI06 GPIO24 Digital input
            pin = 24;
            break;
        case 7: //I07 GPIO11 Digital Input
            pin = 11;
            break;
        case 8: //I08 GPIO07 Digital Input
            pin = 7;
            break;
        case 9: //I09 GPIO08 Digital Input
            pin = 8;
            break;
        case 10: //I10 GPIO09 Digital Input
            pin = 9;
            break;
        case 11: //I11 GPIO25 Digital Input
            pin = 25;
            break;
        case 12: //DI12 GPIO10 Digital input
            pin = 10;
            break;
        case 13: //DI13 GPIO31 Digital input
            pin = 31;
            break;
        case 14: //DI14 GPIO30 Digital input
            pin = 30;
            break;
        default:
            return 0;
        }
    }
    if (circuit.startsWith("DO")) { //MCP23008 Output Pins
        switch (circuit.mid(2, 2).toInt()) {
        case 01: //DO1 GP07 Digital Output
            pin = 7;
            break;
        case 02: //DO1 GP07 Digital Output
            pin = 6;
            break;
        case 03: //DO1 GP07 Digital Output
            pin = 5;
            break;
        case 04: //DO1 GP07 Digital Output
            pin = 4;
            break;
        case 05: //DO1 GP07 Digital Output
            pin = 3;
            break;
        case 06: //DO1 GP07 Digital Output
            pin = 2;
            break;
        default:
            return 0;
        }
    }

    if (circuit.startsWith("AO")) { //Raspberry Pi Analog Output
        switch (circuit.mid(2, 2).toInt()) {
        case 0: //AO GPIO18 PWM Analog Output 0-10V
            pin = 18;
            break;
        default:
            return 0;
        }
    }

    if (circuit.startsWith("AI")) { //MCP3422 analog input channels
        switch (circuit.mid(2, 2).toInt()) {
        case 1:
            pin = 1; //MCP3422 Channel 1
            break;
        case 2:
            pin = 2; //MCP3422 Channel 2
            break;
        default:
            return 0;
        }
    }
    return pin;
}

bool UniPi::setDigitalOutput(const QString &circuit, bool status)
{
    int pin = getPinFromCircuit(circuit);
    if (pin == 0) {
        qWarning(dcUniPi()) << "Out of range pin number";
        return false;
    }

    quint8 registerValue;
    if(!m_mcp23008->readRegister(MCP23008::RegisterAddress::OLAT, &registerValue))
        return false;
    if (status) {
        registerValue |= (1 << pin);
    } else {
        registerValue &= ~(1 << pin);
    }
    //write output register
    if(!m_mcp23008->writeRegister(MCP23008::RegisterAddress::OLAT, registerValue))
        return false;

    getDigitalOutput(circuit);
    return true;
}

bool UniPi::getDigitalOutput(const QString &circuit)
{
    int pin = getPinFromCircuit(circuit);
    if (pin > 7)
        return false;

    uint8_t registerValue;
    if(!m_mcp23008->readRegister(MCP23008::RegisterAddress::OLAT, &registerValue))
        return false;

    emit digitalOutputStatusChanged(circuit, ( registerValue & (1 << pin)));
    return  true;
}

bool UniPi::getDigitalInput(const QString &circuit)
{
    int pin = getPinFromCircuit(circuit);
    if (pin == 0) {
        qWarning(dcUniPi()) << "Out of range pin number";
        return false;
    }
    if (!m_monitorGpios.values().contains(circuit))
        return false;
    //Read RPi pins
    GpioMonitor *gpio = m_monitorGpios.key(circuit);
    digitalInputStatusChanged(circuit, gpio->value());
    return true;
}

bool UniPi::setAnalogOutput(const QString &circuit, double value)
{
    int pin = getPinFromCircuit(circuit);
    if (pin == 0) {
        qWarning(dcUniPi()) << "Out of range pin number";
        return false;
    }
    if (!m_pwms.values().contains(circuit))
        return false;
    int percentage = value * 10;     //convert volt to percentage
    Pwm *pwm = m_pwms.key(circuit);
    if(!pwm->setPercentage(percentage))
        return false;

    getAnalogOutput(circuit);
    return true;
}

bool UniPi::getAnalogOutput(const QString &circuit)
{
    int pin = getPinFromCircuit(circuit);
    if (pin == 0) {
        qWarning(dcUniPi()) << "Out of range pin number";
        return false;
    }
    Pwm *pwm = m_pwms.key(circuit);
    double voltage = pwm->percentage()/10.0;
    emit analogOutputStatusChanged(circuit, voltage);

    return true;
}

bool UniPi::getAnalogInput(const QString &circuit)
{
    int pin = getPinFromCircuit(circuit);
    if (pin == 0) {
        qWarning(dcUniPi()) << "Out of range pin number";
        return false;
    }

    double voltage;
    if (pin == 1) {
        voltage= m_mcp3422->getChannelValue(MCP3422::Channel1);
    } else {
        voltage= m_mcp3422->getChannelValue(MCP3422::Channel2);
    }

    emit analogInputStatusChanged(circuit, voltage);
    return false;
}

void UniPi::onInputValueChanged(const bool &value)
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
