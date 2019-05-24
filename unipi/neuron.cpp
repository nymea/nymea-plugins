#include "neuron.h"
#include "extern-plugininfo.h"

#include <QFile>
#include <QTextStream>
#include <QtConcurrent>
#include <QFuture>

Neuron::Neuron(NeuronTypes neuronType, ModbusTCPMaster *modbusInterface, QObject *parent) :
    QObject(parent),
    m_modbusInterface(modbusInterface),
    m_neuronType(neuronType)
{
    QTimer *m_inputPollingTimer = new QTimer(this);
    m_inputPollingTimer->setTimerType(Qt::TimerType::PreciseTimer);
    m_inputPollingTimer->setSingleShot(true);
    m_inputPollingTimer->start(100);
    connect(m_inputPollingTimer, &QTimer::timeout, this, &Neuron::onInputPollingTimer);

    QTimer *m_outputPollingTimer = new QTimer(this);
    m_outputPollingTimer->setTimerType(Qt::TimerType::PreciseTimer);
    m_outputPollingTimer->setSingleShot(true);
    m_outputPollingTimer->start(1000);
    connect(m_inputPollingTimer, &QTimer::timeout, this, &Neuron::onOutputPollingTimer);

    connect(this, &Neuron::finishedDigitalInputPolling, this, &Neuron::onDigitalInputPollingFinished, Qt::QueuedConnection);
    connect(this, &Neuron::finishedDigitalOutputPolling, this, &Neuron::onDigitalOutputPollingFinished, Qt::QueuedConnection);
}

Neuron::~Neuron(){
    m_inputPollingTimer->stop();
    m_inputPollingTimer->deleteLater();
    m_outputPollingTimer->stop();
    m_outputPollingTimer->deleteLater();
}

bool Neuron::loadModbusMap()
{
    QStringList fileCoilList;
    QStringList fileRegisterList;

    switch (m_neuronType) {
    case NeuronTypes::S103:
        fileCoilList.append(QString("/usr/share/nymea/modbus/Neuron_S103/Neuron_S103-Coils-group-1.csv"));
        break;
    case NeuronTypes::L403:
        fileCoilList.append(QString("/usr/share/nymea/modbus/Neuron_L403/Neuron_L403-Coils-group-1.csv"));
        fileCoilList.append(QString("/usr/share/nymea/modbus/Neuron_L403/Neuron_L403-Coils-group-2.csv"));
        fileCoilList.append(QString("/usr/share/nymea/modbus/Neuron_L403/Neuron_L403-Coils-group-3.csv"));
        break;
    default:
        fileCoilList.append(QString("/usr/share/nymea/modbus/Neuron_S103/Neuron_S103-Coils-group-1.csv"));
    }

    foreach(QString csvFilePath, fileCoilList) {
        qDebug(dcUniPi()) << "Open CSV File:" << csvFilePath;
        QFile *csvFile = new QFile(csvFilePath);
        if (!csvFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCDebug(dcUniPi()) << csvFile->errorString();
            csvFile->deleteLater();
            return false;
        }
        QTextStream *textStream = new QTextStream(csvFile);
        while (!textStream->atEnd()) {
            QString line = textStream->readLine();
            QStringList list = line.split(',');
            if (list[4] == "Basic") {
                QString circuit = list[3].split(" ").last();
                if (list[3].contains("Digital Input", Qt::CaseSensitivity::CaseInsensitive)) {
                    m_modbusDigitalInputRegisters.insert(circuit, list[0].toInt());
                    qDebug(dcUniPi()) << "Found input register" << circuit << list[0].toInt();
                } else if (list[3].contains("Digital Output", Qt::CaseSensitivity::CaseInsensitive)) {
                    m_modbusDigitalOutputRegisters.insert(circuit, list[0].toInt());
                    qDebug(dcUniPi()) << "Found output register" << circuit << list[0].toInt();
                } else if (list[3].contains("Relay Output", Qt::CaseSensitivity::CaseInsensitive)) {
                    m_modbusDigitalOutputRegisters.insert(circuit, list[0].toInt());
                    qDebug(dcUniPi()) << "Found relay register" << circuit << list[0].toInt();
                }
            }
        }
        csvFile->close();
        csvFile->deleteLater();
    }


    switch (m_neuronType) {
    case NeuronTypes::S103:
        fileRegisterList.append(QString("/usr/share/nymea/modbus/Neuron_S103/Neuron_S103-Registers-group-1.csv"));
        break;
    case NeuronTypes::L403:
        fileRegisterList.append(QString("/usr/share/nymea/modbus/Neuron_L403/Neuron_L403-Registers-group-1.csv"));
        fileRegisterList.append(QString("/usr/share/nymea/modbus/Neuron_L403/Neuron_L403-Registers-group-2.csv"));
        fileRegisterList.append(QString("/usr/share/nymea/modbus/Neuron_L403/Neuron_L403-Registers-group-3.csv"));
        break;
    default:
        fileRegisterList.append(QString("/usr/share/nymea/modbus/Neuron_S103/Neuron_S103-Registers-group-1.csv"));
    }
    foreach (QString csvFilePath, fileRegisterList) {
        qDebug(dcUniPi()) << "Open CSV File:" << csvFilePath;
        QFile *csvFile = new QFile(csvFilePath);
        if (!csvFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCDebug(dcUniPi()) << csvFile->errorString();
            csvFile->deleteLater();
            return false;
        }
        QTextStream *textStream = new QTextStream(csvFile);
        while (!textStream->atEnd()) {
            QString line = textStream->readLine();
            QStringList list = line.split(',');
            if (list[4] == "Basic") {
                QString circuit = list[3].split(" ").at(3);
                if (list[3].contains("Analog Input Value", Qt::CaseSensitivity::CaseInsensitive)) {
                    m_modbusAnalogInputRegisters.insert(circuit, list[0].toInt());
                    qDebug(dcUniPi()) << "Found analog input register" << circuit << list[0].toInt();
                } else if (list[3].contains("Analog Output Value", Qt::CaseSensitivity::CaseInsensitive)) {
                    m_modbusAnalogOutputRegisters.insert(circuit, list[0].toInt());
                    qDebug(dcUniPi()) << "Found analog output register" << circuit << list[0].toInt();
                }
            }
        }
        csvFile->close();
        csvFile->deleteLater();
    }
    return true;
}


bool Neuron::getDigitalInput(const QString &circuit)
{
    bool value;

    int modbusAddress = m_modbusDigitalInputRegisters.value(circuit);
    if (!m_modbusInterface->getCoil(0, modbusAddress, &value)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }
    //qDebug(dcUniPi()) << "Reading digital Input" << circuit << modbusAddress << value;
    return value;
}


void Neuron::setDigitalOutput(const QString &circuit, bool value)
{
    int modbusAddress = m_modbusDigitalOutputRegisters.value(circuit);
    if (!m_modbusInterface->setCoil(0, modbusAddress, value)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }
    //qDebug(dcUniPi()) << "Setting digital ouput" << circuit << modbusAddress << value;
    return;
}


bool Neuron::getDigitalOutput(const QString &circuit)
{
    bool value;
    int modbusAddress = m_modbusDigitalOutputRegisters.value(circuit);
    if (!m_modbusInterface->getCoil(0, modbusAddress, &value)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }
    //qDebug(dcUniPi()) << "Reading digital Output" << circuit << modbusAddress << value;
    return value;
}


void Neuron::setAnalogOutput(const QString &circuit, double value)
{
    Q_UNUSED(circuit);
    Q_UNUSED(value);
    //int modbusAddress = m_modbusAnalogOutputRegisters.value(circuit);
    //TODO
}


double Neuron::getAnalogInput(const QString &circuit)
{
    Q_UNUSED(circuit);
    //TODO
    return 0.00;
}

void Neuron::onOutputPollingTimer()
{
    QtConcurrent::run([this](QHash<QString, int> modbusDigitalOutputRegisters)
    {
        QHash<QString, bool> digitalOutputValues;
        foreach(QString circuit, modbusDigitalOutputRegisters.keys()){
            digitalOutputValues.insert(circuit, getDigitalOutput(circuit));
        }
        emit finishedDigitalOutputPolling(digitalOutputValues);
   }, m_modbusDigitalOutputRegisters);
}

void Neuron::onInputPollingTimer()
{
    QtConcurrent::run([this](QHash<QString, int> modbusDigitalInputRegisters)
    {
        QHash<QString, bool> digitalInputValues;
        foreach(QString circuit, modbusDigitalInputRegisters.keys()){
            digitalInputValues.insert(circuit, getDigitalInput(circuit));
        }
        emit finishedDigitalInputPolling(digitalInputValues);
   }, m_modbusDigitalInputRegisters);
}

void Neuron::onDigitalInputPollingFinished(QHash<QString, bool> digitalInputValues)
{
    foreach(QString circuit, digitalInputValues.keys()) {
        if (m_digitalInputValueBuffer.value(circuit) != digitalInputValues.value(circuit)) {
            m_digitalInputValueBuffer.insert(circuit, digitalInputValues.value(circuit));
            emit digitalInputStatusChanged(circuit, digitalInputValues.value(circuit));
        }
    }
    m_outputPollingTimer->start(1000);
}

void Neuron::onDigitalOutputPollingFinished(QHash<QString, bool> digitalOutputValues)
{
    foreach(QString circuit, digitalOutputValues.keys()) {
        if (m_digitalOutputValueBuffer.value(circuit) != digitalOutputValues.value(circuit)) {
            m_digitalOutputValueBuffer.insert(circuit, digitalOutputValues.value(circuit));
            emit digitalOutputStatusChanged(circuit, digitalOutputValues.value(circuit));
        }
    }
    m_inputPollingTimer->start(100);
}
