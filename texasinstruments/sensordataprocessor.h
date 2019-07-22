/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015-2018 Simon Stuerz <simon.stuerz@guh.io>             *
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

#ifndef SENSORDATAPROCESSOR_H
#define SENSORDATAPROCESSOR_H

#include <QFile>
#include <QObject>

#include "devices/device.h"
#include "extern-plugininfo.h"

#include "sensorfilter.h"

class SensorDataProcessor : public QObject
{
    Q_OBJECT
public:
    explicit SensorDataProcessor(Device *device, QObject *parent = nullptr);
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
    Device *m_device = nullptr;
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


signals:

private slots:

};

#endif // SENSORDATAPROCESSOR_H
