#include "neuron.h"
#include "extern-plugininfo.h"

#include <QFile>
#include <QTextStream>

Neuron::Neuron(NeuronTypes neuronType, ModbusTCPMaster *modbusInterface, QObject *parent) :
    QObject(parent),
    m_modbusInterface(modbusInterface),
    m_neuronType(neuronType)
{

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
    qDebug(dcUniPi()) << "Reading digital Input" << circuit << modbusAddress << value;
    return value;
}


void Neuron::setDigitalOutput(const QString &circuit, bool value)
{
    int modbusAddress = m_modbusDigitalOutputRegisters.value(circuit);
    if (!m_modbusInterface->setCoil(0, modbusAddress, value)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }
    qDebug(dcUniPi()) << "Setting digital ouput" << circuit << modbusAddress << value;
    return;
}

bool Neuron::getDigitalOutput(const QString &circuit)
{
    bool value;
    int modbusAddress = m_modbusDigitalOutputRegisters.value(circuit);
    if (!m_modbusInterface->getCoil(0, modbusAddress, &value)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }
    qDebug(dcUniPi()) << "Reading digital Output" << circuit << modbusAddress << value;
    return value;
}


void Neuron::setAnalogOutput(const QString &circuit, double value)
{
    Q_UNUSED(circuit);
    Q_UNUSED(value);
    //int modbusAddress = m_modbusAnalogOutputRegisters.value(circuit);
}


double Neuron::getAnalogInput(const QString &circuit)
{
    Q_UNUSED(circuit);
    return 0.00;
}
