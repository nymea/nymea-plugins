#include "neuronextension.h"
#include "extern-plugininfo.h"

#include <QFile>
#include <QTextStream>

NeuronExtension::NeuronExtension(ExtensionTypes extensionType, ModbusRTUMaster *modbusInterface, int slaveAddress, QObject *parent) :
    QObject(parent),
    m_modbusInterface(modbusInterface),
    m_slaveAddress(slaveAddress),
    m_extensionType(extensionType)
{

}

bool NeuronExtension::loadModbusMap()
{
    QStringList fileList;

    switch(m_extensionType) {
    case ExtensionTypes::xS10:
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_xS10/Neuron_xS10-Coils-group-1.csv"));
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_xS10/Neuron_xS10-Coils-group-2.csv"));
        break;
    case ExtensionTypes::xS20:
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_xS20/Neuron_xS20-Coils-group-1.csv"));
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_xS20/Neuron_xS20-Coils-group-2.csv"));
        break;
    case ExtensionTypes::xS30:
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_xS30/Neuron_xS30-Coils-group-1.csv"));
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_xS30/Neuron_xS30-Coils-group-2.csv"));
        break;
    case ExtensionTypes::xS40:
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_xS40/Neuron_xS40-Coils-group-1.csv"));
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_xS40/Neuron_xS40-Coils-group-2.csv"));
        break;
    case ExtensionTypes::xS50:
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_xS50/Neuron_xS50-Coils-group-1.csv"));
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_xS50/Neuron_xS50-Coils-group-2.csv"));
        break;
    }

    foreach (QString csvFilePath, fileList) {
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
            m_modbusMap.insert(list[3], list[0].toInt());
        }
        csvFile->close();
        csvFile->deleteLater();
    }
    return true;
}


bool NeuronExtension::getDigitalInput(const QString &circuit)
{
    bool value;
    int modbusAddress = getModbusAddress(circuit); //TODO add prefix to circuit
    if (!m_modbusInterface->getCoil(m_slaveAddress, modbusAddress, &value)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }
    return value;
}


void NeuronExtension::setDigitalOutput(const QString &circuit, bool value)
{
    int modbusAddress = getModbusAddress(circuit); //TODO add prefix to circuit
    if (!m_modbusInterface->setCoil(m_slaveAddress, modbusAddress, value)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }
    return;
}

bool NeuronExtension::getDigitalOutput(const QString &circuit)
{
    bool value;
    int modbusAddress = getModbusAddress(circuit); //TODO add prefix to circuit
    if (!m_modbusInterface->getCoil(m_slaveAddress, modbusAddress, &value)) {
        qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
    }
    return value;
}


void NeuronExtension::setAnalogOutput(const QString &circuit, double value)
{
    int modbusAddress = getModbusAddress(circuit); //TODO add prefix to circuit
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
