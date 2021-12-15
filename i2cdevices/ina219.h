#ifndef INA219_H
#define INA219_H

#include <QObject>
#include <hardware/i2c/i2cdevice.h>

class Ina219 : public I2CDevice
{
    Q_OBJECT
public:
    enum VoltageRange {
        VoltageRange16 = 0,
        VoltageRange32 = 1
    };
    Q_ENUM(VoltageRange)

    enum GainVolts {
        GainVolts004 = 0,
        GainVolts008 = 1,
        GainVolts016 = 2,
        GainVolts032 = 3,
    };
    Q_ENUM(GainVolts)

    enum ADCBits {
        ADCBits9 = 0,
        ADCBits10 = 1,
        ADCBits11 = 2,
        ADCBits12 = 3
    };
    Q_ENUM(ADCBits)

    enum OperationMode {
        OperationModePowerDown = 0,
        OperationModeShuntVoltageTriggered = 1,
        OperationModeBusVoltageTriggered = 2,
        OperationModeShuntAndBusTriggered = 3,
        OperationModeADCOff = 4,
        OperationModeShuntVoltageContinuous = 5,
        OperationModeBusVoltageContinuous = 6,
        OperationModeShuntAndBusContinuous = 7
    };
    Q_ENUM(OperationMode)

    explicit Ina219(const QString &portName, int address, double shuntOhms, VoltageRange voltageRange, QObject *parent = nullptr);

    bool writeData(int fileDescriptor, const QByteArray &data) override;
    QByteArray readData(int fileDescriptor) override;

signals:
    void measurementAvailable();

private:
    double m_shuntOhms = 0.1;
    VoltageRange m_voltageRange = VoltageRange32;
    GainVolts m_gainVolts = GainVolts004;
    ADCBits m_busADC = ADCBits12;
    ADCBits m_shuntADC = ADCBits12;
    OperationMode m_operationMode = OperationModeShuntAndBusContinuous;

    double m_currentLSB = 0;
};

#endif // INA219_H
