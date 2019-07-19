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

#ifndef MCP23008_H
#define MCP23008_H

#include <QObject>
#include <QFile>

class MCP23008 : public QObject
{
    Q_OBJECT
public:
    enum Pinout {
        GP0 = 0x00,
        GP1 = 0x01,
        GP2 = 0x02,
        GP3 = 0x03
    };
    Q_ENUM(Pinout)

    enum RegisterAddress {
        IODIR   = 0x00, //Controls the direction of the data I/O. When a bit is set, the corresponding pin becomes an input. When a bit is clear, the corresponding pin becomes an output.
        IPOL    = 0x01, //The IPOL register allows the user to configure the polarity on the corresponding GPIO port bits. If a bit is set, the corresponding GPIO register bit will reflect the inverted value on the pin.
        GPINTEN = 0x02, //The GPINTEN register controls the interrupt-on- change feature for each pin. If a bit is set, the corresponding pin is enabled for interrupt-on-change. The DEFVAL and INTCON registers must also be configured if any pins are enabled for interrupt-on-change.
        DEFVAL  = 0x03, //The default comparison value is configured in the DEFVAL register. If enabled (via GPINTEN and INTCON) to compare against the DEFVAL register, an opposite value on the associated pin will cause an interrupt to occur.
        INTCON  = 0x04, //The INTCON register controls how the associated pin value is compared for the interrupt-on-change feature. If a bit is set, the corresponding I/O pin is compared against the associated bit in the DEFVAL register. If a bit value is clear, the corresponding I/O pin is compared against the previous value.
        IOCON   = 0x05, //The IOCON register contains several bits for configuring the device:
        GPPU    = 0x06, //The GPPU register controls the pull-up resistors for the port pins. If a bit is set and the corresponding pin is configured as an input, the corresponding port pin is internally pulled up with a 100 kΩ resistor.
        INTF    = 0x07, //The INTF register reflects the interrupt condition on the port pins of any pin that is enabled for interrupts via the GPINTEN register. A ‘set’ bit indicates that the associated pin caused the interrupt. This register is ‘read-only’. Writes to this register will be ignored.
        INTCAP  = 0x08, //The INTCAP register captures the GPIO port value at the time the interrupt occurred. The register is ‘read- only’ and is updated only when an interrupt occurs. The register will remain unchanged until the interrupt is cleared via a read of INTCAP or GPIO.
        GPIO    = 0x09, //GENERAL PURPOSE I/O PORT REGISTER
        OLAT    = 0x0A  //The OLAT register provides access to the output latches. A read from this register results in a read of the OLAT and not the port itself. A write to this register modifies the output latches that modify the pins configured as outputs.
    };
    Q_ENUM(RegisterAddress)

    explicit MCP23008(const QString &i2cPortName, int i2cAddress = 0x48, QObject *parent = nullptr);
    ~MCP23008() override;

    bool init();

    bool writeRegister(RegisterAddress registerAddress, uint8_t value);
    bool readRegister(RegisterAddress registerAddress, uint8_t *value);

private:
    QFile m_i2cFile;
    QString m_i2cPortName;
    int m_i2cAddress;

    int m_fileDescriptor = -1;
};

#endif // MCP23008_H

