#include "neuronextension.h"
#include "extern-plugininfo.h"

#include <QFile>
#include <QTextStream>
#include <QtConcurrent>
#include <qtconcurrentrun.h>
#include <QFuture>

NeuronExtension::NeuronExtension(ExtensionTypes extensionType, ModbusRTUMaster *modbusInterface, int slaveAddress, QObject *parent) :
    QObject(parent),
    m_modbusInterface(modbusInterface),
    m_slaveAddress(slaveAddress),
    m_extensionType(extensionType)
{
    connect(this, &NeuronExtension::finishedDigitalInputPolling, this, &NeuronExtension::onDigitalInputPollingFinished, Qt::QueuedConnection);
    connect(this, &NeuronExtension::finishedDigitalOutputPolling, this, &NeuronExtension::onDigitalOutputPollingFinished, Qt::QueuedConnection);

    connect(&m_inputPollingTimer, &QTimer::timeout, this, &NeuronExtension::onInputPollingTimer);
    m_inputPollingTimer.setTimerType(Qt::TimerType::PreciseTimer);
    m_inputPollingTimer.setSingleShot(true);
    m_inputPollingTimer.start(100);


    connect(&m_outputPollingTimer, &QTimer::timeout, this, &NeuronExtension::onOutputPollingTimer);
    m_outputPollingTimer.setTimerType(Qt::TimerType::PreciseTimer);
    m_outputPollingTimer.setSingleShot(true);
    m_outputPollingTimer.start(1000);
}

NeuronExtension::~NeuronExtension(){
    m_inputPollingTimer.stop();
    m_inputPollingTimer.deleteLater();
    m_outputPollingTimer.stop();
    m_outputPollingTimer.deleteLater();
}

bool NeuronExtension::loadModbusMap()
{
    QStringList fileCoilList;
    QStringList fileRegisterList;

    switch(m_extensionType) {
    case ExtensionTypes::xS10:
        fileCoilList.append(QString("/usr/share/nymea/modbus/Neuron_xS10/Neuron_xS10-Coils-group-1.csv"));
        break;
    case ExtensionTypes::xS20:
        fileCoilList.append(QString("/usr/share/nymea/modbus/Neuron_xS20/Neuron_xS20-Coils-group-1.csv"));
        break;
    case ExtensionTypes::xS30:
        fileCoilList.append(QString("/usr/share/nymea/modbus/Neuron_xS30/Neuron_xS30-Coils-group-1.csv"));
        break;
    case ExtensionTypes::xS40:
        fileCoilList.append(QString("/usr/share/nymea/modbus/Neuron_xS40/Neuron_xS40-Coils-group-1.csv"));
        break;
    case ExtensionTypes::xS50:
        fileCoilList.append(QString("/usr/share/nymea/modbus/Neuron_xS50/Neuron_xS50-Coils-group-1.csv"));
        break;
    }

    foreach (QString csvFilePath, fileCoilList) {
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


    switch(m_extensionType) {
    case ExtensionTypes::xS10:
        fileRegisterList.append(QString("/usr/share/nymea/modbus/Neuron_xS10/Neuron_xS10-Registers-group-1.csv"));
        break;
    case ExtensionTypes::xS20:
        fileRegisterList.append(QString("/usr/share/nymea/modbus/Neuron_xS20/Neuron_xS20-Registers-group-1.csv"));
        break;
    case ExtensionTypes::xS30:
        fileRegisterList.append(QString("/usr/share/nymea/modbus/Neuron_xS30/Neuron_xS30-Registers-group-1.csv"));
        break;
    case ExtensionTypes::xS40:
        fileRegisterList.append(QString("/usr/share/nymea/modbus/Neuron_xS40/Neuron_xS40-Registers-group-1.csv"));
        break;
    case ExtensionTypes::xS50:
        fileRegisterList.append(QString("/usr/share/nymea/modbus/Neuron_xS50/Neuron_xS50-Registers-group-1.csv"));
        break;
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


bool NeuronExtension::getDigitalInput(const QString &circuit)
{
    bool value;
    int modbusAddress = m_modbusDigitalInputRegisters.value(circuit);
    if (!m_modbusInterface->getCoil(m_slaveAddress, modbusAddress, &value)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }

    //qDebug(dcUniPi()) << "Reading digital input" << circuit << modbusAddress << value;
    return value;
}


void NeuronExtension::setDigitalOutput(const QString &circuit, bool value)
{
    int modbusAddress = m_modbusDigitalOutputRegisters.value(circuit);
    if (!m_modbusInterface->setCoil(m_slaveAddress, modbusAddress, value)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }

    //qDebug(dcUniPi()) << "Setting digital ouput" << circuit << modbusAddress << value;
    return;
}

bool NeuronExtension::getDigitalOutput(const QString &circuit)
{
    bool value;
    int modbusAddress = m_modbusDigitalOutputRegisters.value(circuit);
    if (!m_modbusInterface->getCoil(m_slaveAddress, modbusAddress, &value)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }

    //qDebug(dcUniPi()) << "Reading digital output" << circuit << modbusAddress << value;
    return value;
}


void NeuronExtension::setAnalogOutput(const QString &circuit, double value)
{
    int modbusAddress = m_modbusAnalogOutputRegisters.value(circuit);
    int rawValue = static_cast<int>(value);
    if (!m_modbusInterface->setRegister(m_slaveAddress, modbusAddress, rawValue)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }
    return;
}

/*
double NeuronExtension::getAnalogOutput(const QString &circuit)
{
    double value;
    int modbusAddress = getModbusAddress(circuit); //TODO add prefix to circuit
    if (!m_modbusInterface->getRegister(m_slaveAddress, modbusAddress, &value)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }
    return value;
}


double NeuronExtension::getAnalogInput(const QString &circuit)
{
    double value;
    int modbusAddress = getModbusAddress(circuit); //TODO add prefix to circuit
    if (!m_modbusInterface->getRegister(m_slaveAddress, modbusAddress, &value)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }
    return value;
}
*/

void NeuronExtension::onOutputPollingTimer()
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

void NeuronExtension::onInputPollingTimer()
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

void NeuronExtension::onDigitalInputPollingFinished(QHash<QString, bool> digitalInputValues)
{
    foreach(QString circuit, digitalInputValues.keys()) {
        if (m_digitalInputValueBuffer.value(circuit) != digitalInputValues.value(circuit)) {
            m_digitalInputValueBuffer.insert(circuit, digitalInputValues.value(circuit));
            emit digitalInputStatusChanged(circuit, digitalInputValues.value(circuit));
        }
    }
    m_inputPollingTimer.start(100);
}

void NeuronExtension::onDigitalOutputPollingFinished(QHash<QString, bool> digitalOutputValues)
{
    foreach(QString circuit, digitalOutputValues.keys()) {
        if (m_digitalOutputValueBuffer.value(circuit) != digitalOutputValues.value(circuit)) {
            m_digitalOutputValueBuffer.insert(circuit, digitalOutputValues.value(circuit));
            emit digitalOutputStatusChanged(circuit, digitalOutputValues.value(circuit));
        }
    }
    m_outputPollingTimer.start(1000);
}
