#ifndef NEURON_H
#define NEURON_H

#include "modbustcpmaster.h"

#include <QObject>
#include <QHash>

class Neuron : public QObject
{
    Q_OBJECT
public:

    enum NeuronTypes {
        S103,
        M103,
        M203,
        M303,
        M403,
        M503,
        L203,
        L303,
        L403,
        L503,
        L513
    };

    explicit Neuron(NeuronTypes neuronType, ModbusTCPMaster *modbusInterface, QObject *parent = nullptr);
    bool loadModbusMap();

    void setDigitalOutput(const QString &circuit, bool value);
    bool getDigitalOutput(const QString &circuit);
    bool getDigitalInput(const QString &circuit);

    void setAnalogOutput(const QString &circuit, double value);
    double getAnalogOutput(const QString &circuit);
    double getAnalogInput(const QString &circuit);

private:

    ModbusTCPMaster *m_modbusInterface = nullptr;
    QHash<QString, int> m_modbusMap;
    NeuronTypes m_neuronType = NeuronTypes::S103;

    int getModbusAddress(const QString &circuit);

signals:

public slots:
};

#endif // NEURON_H
