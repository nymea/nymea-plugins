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

#ifndef SENSORDATAPROCESSOR_H
#define SENSORDATAPROCESSOR_H

#include <QFile>
#include <QObject>

#include <integrations/thing.h>

#include "extern-plugininfo.h"
#include "sensorfilter.h"

class SensorDataProcessor : public QObject
{
    Q_OBJECT
public:
    explicit SensorDataProcessor(Thing *thing, QObject *parent = nullptr);
    ~SensorDataProcessor();

    void setAccelerometerRange(int accelerometerRange);
    void setMovementSensitivity(int movementSensitivity);

    static double roundValue(float value);
    static bool testBitUint8(quint8 value, int bitPosition);

    void processTemperatureData(const QByteArray &data);
    void processKeyData(const QByteArray &data);
    void processHumidityData(const QByteArray &data);
    void processPressureData(const QByteArray &data);
    void processOpticalData(const QByteArray &data);
    void processMovementData(const QByteArray &data);

    void reset();

private:
    Thing *m_thing = nullptr;
    double m_lastAccelerometerVectorLenght = -99999;

    int m_accelerometerRange = 16;
    double m_movementSensitivity = 0.5;

    bool m_leftButtonPressed = false;
    bool m_rightButtonPressed = false;
    bool m_magnetDetected = false;

    // Log sensor data for debugging filters
    // Note: set this to true for enable sensor filter logging
    bool m_filterDebug = true;
    QFile *m_logFile = nullptr;

    SensorFilter *m_temperatureFilter = nullptr;
    SensorFilter *m_objectTemperatureFilter = nullptr;
    SensorFilter *m_humidityFilter = nullptr;
    SensorFilter *m_pressureFilter = nullptr;
    SensorFilter *m_opticalFilter = nullptr;
    SensorFilter *m_accelerometerFilter = nullptr;

    // Set methods
    void setLeftButtonPressed(bool pressed);
    void setRightButtonPressed(bool pressed);
    void setMagnetDetected(bool detected);

    void logSensorValue(double originalValue, double filteredValue);

};

#endif // SENSORDATAPROCESSOR_H
