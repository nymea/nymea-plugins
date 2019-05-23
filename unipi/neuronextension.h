#ifndef NEURONEXTENSION_H
#define NEURONEXTENSION_H

#include "modbusrtumaster.h"

#include <QObject>
#include <QHash>

class NeuronExtension : public QObject
{
    Q_OBJECT
public:

    enum ExtensionTypes {
        xS10,
        xS20,
        xS30,
        xS40,
        xS50
    };

    explicit NeuronExtension(ExtensionTypes extensionType, ModbusRTUMaster *modbusInterface, int slaveAddress, QObject *parent = nullptr);
    bool loadModbusMap();

    void setDigitalOutput(const QString &circuit, bool value);
    bool getDigitalOutput(const QString &circuit);
    bool getDigitalInput(const QString &circuit);

    void setAnalogOutput(const QString &circuit, double value);
    double getAnalogOutput(const QString &circuit);
    double getAnalogInput(const QString &circuit);

private:
    QHash<QString, int> m_modbusMap;
    ModbusRTUMaster *m_modbusInterface = nullptr;
    int m_slaveAddress = 0;
    ExtensionTypes m_extensionType = ExtensionTypes::xS10;


    int getModbusAddress(const QString &circuit);

signals:

public slots:
};

#endif // NEURONEXTENSION_H
