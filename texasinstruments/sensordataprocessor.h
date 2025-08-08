/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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
