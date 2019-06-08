#ifndef NEURON_H
#define NEURON_H

#include <QObject>
#include <QHash>
#include <QTimer>
#include <QtSerialBus>

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

    explicit Neuron(NeuronTypes neuronType, QModbusTcpClient *modbusInterface, QObject *parent = nullptr);
    ~Neuron();

    bool init();
    QString type();

    QList<QString> digitalInputs();
    QList<QString> digitalOutputs();
    QList<QString> analogInputs();
    QList<QString> analogOutputs();
    QList<QString> userLEDs();

    bool setDigitalOutput(const QString &circuit, bool value);
    bool getDigitalOutput(const QString &circuit);
    bool getDigitalInput(const QString &circuit);

    bool setAnalogOutput(const QString &circuit, double value);
    bool getAnalogOutput(const QString &circuit);
    bool getAnalogInput(const QString &circuit);

    bool getAllDigitalOutputs();
    bool getAllDigitalInputs();

    bool setUserLED(const QString &circuit, bool value);
    bool getUserLED(const QString &circuit);
private:
    int m_slaveAddress = 0;

    QTimer m_inputPollingTimer;
    QTimer m_outputPollingTimer;

    QModbusTcpClient *m_modbusInterface = nullptr;

    QHash<QString, int> m_modbusDigitalOutputRegisters;
    QHash<QString, int> m_modbusDigitalInputRegisters;
    QHash<QString, int> m_modbusAnalogInputRegisters;
    QHash<QString, int> m_modbusAnalogOutputRegisters;
    QHash<QString, int> m_modbusUserLEDRegisters;

    NeuronTypes m_neuronType = NeuronTypes::S103;

    bool loadModbusMap();

signals:
    void digitalInputStatusChanged(QString &circuit, bool value);
    void digitalOutputStatusChanged(QString &circuit, bool value);

    void analogInputStatusChanged(QString &circuit, double value);
    void analogOutputStatusChanged(QString &circuit, double value);

    void userLEDStatusChanged(QString &circuit, bool value);

    void connectionStateChanged(bool state);

public slots:
    void onOutputPollingTimer();
    void onInputPollingTimer();

    void onFinished();
    void onErrorOccured(QModbusDevice::Error error);
};

#endif // NEURON_H
