#ifndef SENSORTAG_H
#define SENSORTAG_H

#include <QObject>

#include "plugin/device.h"
#include "extern-plugininfo.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"

static QBluetoothUuid infraredServiceUuid       = QBluetoothUuid(QUuid("f000aa00-0451-4000-b000-000000000000"));
static QBluetoothUuid accelerometerServiceUuid  = QBluetoothUuid(QUuid("f000aa10-0451-4000-b000-000000000000"));
static QBluetoothUuid humidityServiceUuid       = QBluetoothUuid(QUuid("f000aa20-0451-4000-b000-000000000000"));
static QBluetoothUuid magnetometerServiceUuid   = QBluetoothUuid(QUuid("f000aa30-0451-4000-b000-000000000000"));
static QBluetoothUuid pressureServiceUuid       = QBluetoothUuid(QUuid("f000aa40-0451-4000-b000-000000000000"));
static QBluetoothUuid gyroscopeServiceUuid      = QBluetoothUuid(QUuid("f000aa50-0451-4000-b000-000000000000"));
static QBluetoothUuid testServiceUuid           = QBluetoothUuid(QUuid("f000aa60-0451-4000-b000-000000000000"));
static QBluetoothUuid otaServiceUuid            = QBluetoothUuid(QUuid("f000aac0-0451-4000-b000-000000000000"));
static QBluetoothUuid buttonServiceUuid         = QBluetoothUuid(QUuid("0000ffe0-0000-1000-8000-00805f9b34fb"));

// Currently unused services

class SensorTag : public QObject
{
    Q_OBJECT
public:
    explicit SensorTag(Device *device, BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent = nullptr);

    Device *device();
    BluetoothLowEnergyDevice *bluetoothDevice();

private:
    Device *m_device;
    BluetoothLowEnergyDevice *m_bluetoothDevice;

    // Services
    QLowEnergyService *m_infraredService = nullptr;
    QLowEnergyService *m_buttonService = nullptr;
    QLowEnergyService *m_humidityService = nullptr;
    QLowEnergyService *m_pressureService = nullptr;
    QLowEnergyService *m_accelerometerService = nullptr;
    QLowEnergyService *m_magnetometerService = nullptr;
    QLowEnergyService *m_gyroscopeService = nullptr;

    // Characteristics
    QLowEnergyCharacteristic m_infraredDataCharacteristic;
    QLowEnergyCharacteristic m_infraredConfigCharacteristic;
    QLowEnergyCharacteristic m_buttonCharacteristic;
    QLowEnergyCharacteristic m_humidityDataCharacteristic;
    QLowEnergyCharacteristic m_humidityConfigCharacteristic;
    QLowEnergyCharacteristic m_pressureDataCharacteristic;
    QLowEnergyCharacteristic m_pressureConfigCharacteristic;


    void updateInfraredValue(const QByteArray &value);
    void updateButtonValue(const QByteArray &value);
    void updateHumidityValue(const QByteArray &value);
    void updatePressureValue(const QByteArray &value);

signals:
    void leftKeyPressed();
    void rightKeyPressed();

private slots:
    void onConnectedChanged(const bool &connected);
    void onServiceDiscoveryFinished();

    // Infrared sensor service
    void onInfraredServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onInfraredServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    // Button service
    void onButtonServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onButtonServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    // Humidity sensor service
    void onHumidityServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onHumidityServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    // Pressure sensor service
    void onPressureServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onPressureServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);


public slots:
    void measure();
};

#endif // SENSORTAG_H
