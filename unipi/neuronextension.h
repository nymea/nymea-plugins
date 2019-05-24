#ifndef NEURONEXTENSION_H
#define NEURONEXTENSION_H

#include "modbusrtumaster.h"

#include <QObject>
#include <QHash>
#include <QTimer>

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
    ~NeuronExtension();

    bool loadModbusMap();

    bool setDigitalOutput(const QString &circuit, bool value);
    bool getDigitalOutput(const QString &circuit, bool *value);
    bool getDigitalInput(const QString &circuit, bool *value);

    bool setAnalogOutput(const QString &circuit, double value);
    bool getAnalogOutput(const QString &circuit, double *value);
    bool getAnalogInput(const QString &circuit, double *value);

private:

    QTimer m_inputPollingTimer;
    QTimer m_outputPollingTimer;

    QHash<QString, int> m_modbusDigitalOutputRegisters;
    QHash<QString, int> m_modbusDigitalInputRegisters;
    QHash<QString, int> m_modbusAnalogInputRegisters;
    QHash<QString, int> m_modbusAnalogOutputRegisters;

    ModbusRTUMaster *m_modbusInterface = nullptr;
    int m_slaveAddress = 0;
    ExtensionTypes m_extensionType = ExtensionTypes::xS10;

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

#endif // NEURONEXTENSION_H
