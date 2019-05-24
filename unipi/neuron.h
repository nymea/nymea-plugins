#ifndef NEURON_H
#define NEURON_H

#include "modbustcpmaster.h"

#include <QObject>
#include <QHash>
#include <QTimer>

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
    ~Neuron();

    bool loadModbusMap();

    void setDigitalOutput(const QString &circuit, bool value);
    bool getDigitalOutput(const QString &circuit);
    bool getDigitalInput(const QString &circuit);

    void setAnalogOutput(const QString &circuit, double value);
    double getAnalogOutput(const QString &circuit);
    double getAnalogInput(const QString &circuit);

private:

    QTimer m_inputPollingTimer;
    QTimer m_outputPollingTimer;

    ModbusTCPMaster *m_modbusInterface = nullptr;

    QHash<QString, int> m_modbusDigitalOutputRegisters;
    QHash<QString, int> m_modbusDigitalInputRegisters;
    QHash<QString, int> m_modbusAnalogInputRegisters;
    QHash<QString, int> m_modbusAnalogOutputRegisters;

    NeuronTypes m_neuronType = NeuronTypes::S103;

    QHash<QString, bool> m_digitalInputValueBuffer;
    QHash<QString, bool> m_digitalOutputValueBuffer;

signals:
    void digitalInputStatusChanged(QString &circuit, bool value);
    void digitalOutputStatusChanged(QString &circuit, bool value);

    void finishedDigitalInputPolling(QHash<QString, bool> digitalOutputValues);
    void finishedDigitalOutputPolling(QHash<QString, bool> digitalOutputValues);

public slots:
    void onOutputPollingTimer();
    void onInputPollingTimer();

    void onDigitalInputPollingFinished(QHash<QString, bool> digitalInputValues);
    void onDigitalOutputPollingFinished(QHash<QString, bool> digitalOutputValues);
};

#endif // NEURON_H
