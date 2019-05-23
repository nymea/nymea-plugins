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
    QStringList fileList;

    switch (m_neuronType) {
    case NeuronTypes::S103:
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_S103/Neuron_S103-Coils-group-2.csv"));
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_S103/Neuron_S103-Coils-group-3.csv"));
        break;
    case NeuronTypes::L403:
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_L403/Neuron_L403-Coils-group-2.csv"));
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_L403/Neuron_L403-Coils-group-3.csv"));
        break;
    default:
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_S103/Neuron_S103-Coils-group-2.csv"));
        fileList.append(QString("/usr/share/nymea/modbus/Neuron_S103/Neuron_S103-Coils-group-3.csv"));
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
