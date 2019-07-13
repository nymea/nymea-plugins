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

#ifndef UNIPI_H
#define UNIPI_H

#include <QObject>
#include "gpiodescriptor.h"
#include "mcp23008.h"

#include "hardware/gpio.h"
#include "hardware/gpiomonitor.h"

class UniPi : public QObject
{
    Q_OBJECT
    QList<GpioDescriptor> raspberryPiGpioDescriptors();
    void setOutput(int pin, bool status);
    bool getOutput(int pin);
    bool getInput(int pin);

public:
    enum UniPiType {
        UniPi1,
        UniPi1Lite
    };

    explicit UniPi(UniPiType unipiType, QObject *parent = nullptr);
    ~UniPi();

private:
    UniPiType m_unipiType = UniPiType::UniPi1;
    MCP23008 *m_mcp23008 = nullptr;

signals:
    void digitalOutputStatusChanged(QString &circuit, const bool &value);
    void digitalInputStatusChanged(QString &circuit, const bool &value);
    void analogInputStatusChanged(QString &circuit, double value);
    void analogOutputStatusChanged(QString &circuit,double value);

private slots:
    void onGpioValueChanged(const bool &value);
};

#endif // UNIPI_H
