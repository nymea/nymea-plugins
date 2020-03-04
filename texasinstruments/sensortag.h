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

#ifndef SENSORTAG_H
#define SENSORTAG_H

#include <QObject>

#include "integrations/thing.h"
#include "extern-plugininfo.h"
#include "sensordataprocessor.h"

#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"

// http://processors.wiki.ti.com/index.php/CC2650_SensorTag_User's_Guide

static QBluetoothUuid temperatureServiceUuid                        = QBluetoothUuid(QUuid("f000aa00-0451-4000-b000-000000000000"));
static QBluetoothUuid temperatureDataCharacteristicUuid             = QBluetoothUuid(QUuid("f000aa01-0451-4000-b000-000000000000"));
static QBluetoothUuid temperatureConfigurationCharacteristicUuid    = QBluetoothUuid(QUuid("f000aa02-0451-4000-b000-000000000000"));
static QBluetoothUuid temperaturePeriodCharacteristicUuid           = QBluetoothUuid(QUuid("f000aa03-0451-4000-b000-000000000000"));

static QBluetoothUuid humidityServiceUuid                       = QBluetoothUuid(QUuid("f000aa20-0451-4000-b000-000000000000"));
static QBluetoothUuid humidityDataCharacteristicUuid            = QBluetoothUuid(QUuid("f000aa21-0451-4000-b000-000000000000"));
static QBluetoothUuid humidityConfigurationCharacteristicUuid   = QBluetoothUuid(QUuid("f000aa22-0451-4000-b000-000000000000"));
static QBluetoothUuid humidityPeriodCharacteristicUuid          = QBluetoothUuid(QUuid("f000aa23-0451-4000-b000-000000000000"));

static QBluetoothUuid pressureServiceUuid                       = QBluetoothUuid(QUuid("f000aa40-0451-4000-b000-000000000000"));
static QBluetoothUuid pressureDataCharacteristicUuid            = QBluetoothUuid(QUuid("f000aa41-0451-4000-b000-000000000000"));
static QBluetoothUuid pressureConfigurationCharacteristicUuid   = QBluetoothUuid(QUuid("f000aa42-0451-4000-b000-000000000000"));
static QBluetoothUuid pressurePeriodCharacteristicUuid          = QBluetoothUuid(QUuid("f000aa44-0451-4000-b000-000000000000"));

static QBluetoothUuid opticalServiceUuid                        = QBluetoothUuid(QUuid("f000aa70-0451-4000-b000-000000000000"));
static QBluetoothUuid opticalDataCharacteristicUuid             = QBluetoothUuid(QUuid("f000aa71-0451-4000-b000-000000000000"));
static QBluetoothUuid opticalConfigurationCharacteristicUuid    = QBluetoothUuid(QUuid("f000aa72-0451-4000-b000-000000000000"));
static QBluetoothUuid opticalPeriodCharacteristicUuid           = QBluetoothUuid(QUuid("f000aa73-0451-4000-b000-000000000000"));

static QBluetoothUuid keyServiceUuid                            = QBluetoothUuid(QUuid("0000ffe0-0000-1000-8000-00805f9b34fb"));
static QBluetoothUuid keyDataCharacteristicUuid                 = QBluetoothUuid(QUuid("0000ffe1-0000-1000-8000-00805f9b34fb"));

static QBluetoothUuid ioServiceUuid                             = QBluetoothUuid(QUuid("f000aa64-0451-4000-b000-000000000000"));
static QBluetoothUuid ioDataCharacteristicUuid                  = QBluetoothUuid(QUuid("f000aa65-0451-4000-b000-000000000000"));
static QBluetoothUuid ioConfigurationCharacteristicUuid         = QBluetoothUuid(QUuid("f000aa66-0451-4000-b000-000000000000"));

static QBluetoothUuid movementServiceUuid                       = QBluetoothUuid(QUuid("f000aa80-0451-4000-b000-000000000000"));
static QBluetoothUuid movementDataCharacteristicUuid            = QBluetoothUuid(QUuid("f000aa81-0451-4000-b000-000000000000"));
static QBluetoothUuid movementConfigurationCharacteristicUuid   = QBluetoothUuid(QUuid("f000aa82-0451-4000-b000-000000000000"));
static QBluetoothUuid movementPeriodCharacteristicUuid          = QBluetoothUuid(QUuid("f000aa83-0451-4000-b000-000000000000"));


class SensorTag : public QObject
{
    Q_OBJECT
public:
    enum SensorAccelerometerRange {
        SensorAccelerometerRange2G = 2,
        SensorAccelerometerRange4G = 4,
        SensorAccelerometerRange8G = 8,
        SensorAccelerometerRange16G = 16
    };
    Q_ENUM(SensorAccelerometerRange)

    enum SensorMode {
        SensorModeLocal = 0,
        SensorModeRemote = 1,
        SensorModeTest = 2
    };
    Q_ENUM(SensorMode)

    explicit SensorTag(Thing *thing, BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent = nullptr);

    Thing *thing();
    BluetoothLowEnergyDevice *bluetoothDevice();

    // Configurations
    void setTemperatureSensorEnabled(bool enabled);
    void setHumiditySensorEnabled(bool enabled);
    void setPressureSensorEnabled(bool enabled);
    void setOpticalSensorEnabled(bool enabled);
    void setAccelerometerEnabled(bool enabled);
    void setGyroscopeEnabled(bool enabled);
    void setMagnetometerEnabled(bool enabled);

    void setAccelerometerRange(const SensorAccelerometerRange &range);
    void setMeasurementPeriod(int period);
    void setMeasurementPeriodMovement(int period);
    void setMovementSensitivity(int percentage);

    // Actions
    void setGreenLedPower(bool power);
    void setRedLedPower(bool power);
    void setBuzzerPower(bool power);
    void buzzerImpulse();

private:
    Thing *m_thing;
    BluetoothLowEnergyDevice *m_bluetoothDevice;

    // Services
    QLowEnergyService *m_temperatureService = nullptr;
    QLowEnergyService *m_humidityService = nullptr;
    QLowEnergyService *m_pressureService = nullptr;
    QLowEnergyService *m_opticalService = nullptr;
    QLowEnergyService *m_keyService = nullptr;
    QLowEnergyService *m_movementService = nullptr;
    QLowEnergyService *m_ioService = nullptr;

    // Characteristics
    QLowEnergyCharacteristic m_temperatureDataCharacteristic;
    QLowEnergyCharacteristic m_temperatureConfigurationCharacteristic;
    QLowEnergyCharacteristic m_temperaturePeriodCharacteristic;

    QLowEnergyCharacteristic m_humidityDataCharacteristic;
    QLowEnergyCharacteristic m_humidityConfigurationCharacteristic;
    QLowEnergyCharacteristic m_humidityPeriodCharacteristic;

    QLowEnergyCharacteristic m_pressureDataCharacteristic;
    QLowEnergyCharacteristic m_pressureConfigurationCharacteristic;
    QLowEnergyCharacteristic m_pressurePeriodCharacteristic;

    QLowEnergyCharacteristic m_opticalDataCharacteristic;
    QLowEnergyCharacteristic m_opticalConfigurationCharacteristic;
    QLowEnergyCharacteristic m_opticalPeriodCharacteristic;

    QLowEnergyCharacteristic m_keyDataCharacteristic;

    QLowEnergyCharacteristic m_movementDataCharacteristic;
    QLowEnergyCharacteristic m_movementConfigurationCharacteristic;
    QLowEnergyCharacteristic m_movementPeriodCharacteristic;

    QLowEnergyCharacteristic m_ioDataCharacteristic;
    QLowEnergyCharacteristic m_ioConfigurationCharacteristic;

    // Measure periods
    int m_temperaturePeriod = 2500;
    int m_humidityPeriod = 2500;
    int m_pressurePeriod = 2500;
    int m_opticalPeriod = 2500;
    int m_movementPeriod = 500;
    double m_movementSensitivity = 0.5;
    SensorAccelerometerRange m_accelerometerRange = SensorAccelerometerRange16G;

    // States
    bool m_greenLedEnabled = false;
    bool m_redLedEnabled = false;
    bool m_buzzerEnabled = false;

    // Plugin configs
    bool m_temperatureEnabled = true;
    bool m_humidityEnabled = true;
    bool m_pressureEnabled = true;
    bool m_opticalEnabled = true;
    bool m_accelerometerEnabled = true;
    bool m_gyroscopeEnabled = false;
    bool m_magnetometerEnabled = false;

    SensorDataProcessor *m_dataProcessor = nullptr;

    // Configuration methods
    void configurePeriod(QLowEnergyService *serice, const QLowEnergyCharacteristic &characteristic, int measurementPeriod);
    void configureMovement();
    void configureSensorMode(const SensorMode &mode);
    void configureIo();

    void setTemperatureSensorPower(bool power);
    void setHumiditySensorPower(bool power);
    void setPressureSensorPower(bool power);
    void setOpticalSensorPower(bool power);

signals:
    void leftButtonPressedChanged(bool pressed);
    void rightButtonPressedChanged(bool pressed);
    void magnetDetectedChanged(bool detected);

private slots:
    void onConnectedChanged(const bool &connected);
    void onServiceDiscoveryFinished();

    void onBuzzerImpulseTimeout();

    // Temperature sensor service
    void onTemperatureServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onTemperatureServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    // Humidity sensor service
    void onHumidityServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onHumidityServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    // Pressure sensor service
    void onPressureServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onPressureServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    // Optical sensor service
    void onOpticalServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onOpticalServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    // Button service
    void onKeyServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onKeyServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    // Movement service
    void onMovementServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onMovementServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    // IO service
    void onIoServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onIoServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
};

#endif // SENSORTAG_H
