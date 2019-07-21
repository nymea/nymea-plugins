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
#include "mcp3422.h"

#include "hardware/gpio.h"
#include "hardware/gpiomonitor.h"
#include "hardware/pwm.h"

class UniPi : public QObject
{
    Q_OBJECT


public:
    enum UniPiType {
        UniPi1,
        UniPi1Lite
    };

    explicit UniPi(UniPiType unipiType, QObject *parent = nullptr);
    ~UniPi();

    bool init();
    QString type();

    bool setDigitalOutput(const QString &cicuit, bool status);
    bool getDigitalOutput(const QString &circuit);
    bool getDigitalInput(const QString &circuit);

    bool setAnalogOutput(const QString &circuit, double value);
    bool getAnalogOutput(const QString &circuit);
    bool getAnalogInput(const QString &circuit);

    QList<QString> digitalInputs();
    QList<QString> digitalOutputs();
    QList<QString> analogInputs();
    QList<QString> analogOutputs();

private:
    UniPiType m_unipiType = UniPiType::UniPi1;
    MCP23008 *m_mcp23008 = nullptr;
    MCP3422 *m_mcp3422 = nullptr;

    int getPinFromCircuit(const QString &cicuit);
    QHash<GpioMonitor *, QString> m_monitorGpios;
    QHash<Pwm *, QString> m_pwms;

signals:
    void digitalOutputStatusChanged(const QString &circuit, const bool &value);
    void digitalInputStatusChanged(const QString &circuit, const bool &value);
    void analogInputStatusChanged(const QString &circuit, double value);
    void analogOutputStatusChanged(const QString &circuit, double value);

private slots:
    void onInputValueChanged(const bool &value);
};

#endif // UNIPI_H
