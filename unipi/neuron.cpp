#include "neuron.h"
#include "extern-plugininfo.h"

#include <QFile>
#include <QTextStream>

Neuron::Neuron(NeuronTypes neuronType, QModbusTcpClient *modbusInterface,  QObject *parent) :
    QObject(parent),
    m_modbusInterface(modbusInterface),
    m_neuronType(neuronType)
{
    connect(&m_inputPollingTimer, &QTimer::timeout, this, &Neuron::onInputPollingTimer);
    m_inputPollingTimer.setTimerType(Qt::TimerType::PreciseTimer);
    m_inputPollingTimer.start(1000); //TODO

    connect(&m_outputPollingTimer, &QTimer::timeout, this, &Neuron::onOutputPollingTimer);
    m_outputPollingTimer.setTimerType(Qt::TimerType::PreciseTimer);
    m_outputPollingTimer.setSingleShot(true);
    m_outputPollingTimer.start(10000); //TODO
}

Neuron::~Neuron(){
    m_inputPollingTimer.stop();
    m_inputPollingTimer.deleteLater();
    m_outputPollingTimer.stop();
    m_outputPollingTimer.deleteLater();
}

bool Neuron::init()
{
    return loadModbusMap();
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
    int modbusAddress = m_modbusDigitalInputRegisters.value(circuit);
    qDebug(dcUniPi()) << "Reading digital Input" << circuit << modbusAddress;

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, modbusAddress, 1);

    if (QModbusReply *reply = m_modbusInterface->sendReadRequest(request, m_slaveAddress)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &Neuron::onFinished);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }

    return true;
}


bool Neuron::setDigitalOutput(const QString &circuit, bool value)
{
    int modbusAddress = m_modbusDigitalOutputRegisters.value(circuit);
    qDebug(dcUniPi()) << "Setting digital ouput" << circuit << modbusAddress << value;

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, modbusAddress, 1);
    request.setValue(modbusAddress, value);

    if (QModbusReply *reply = m_modbusInterface->sendWriteRequest(request, m_slaveAddress)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &Neuron::onFinished);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }

    return true;
}


bool Neuron::getDigitalOutput(const QString &circuit)
{
    int modbusAddress = m_modbusDigitalOutputRegisters.value(circuit);
    qDebug(dcUniPi()) << "Reading digital Output" << circuit << modbusAddress;

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, modbusAddress, 1);

    if (QModbusReply *reply = m_modbusInterface->sendReadRequest(request, m_slaveAddress)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &Neuron::onFinished);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }
    return true;
}


bool Neuron::setAnalogOutput(const QString &circuit, double value)
{
    Q_UNUSED(value);

    int modbusAddress = m_modbusAnalogOutputRegisters.value(circuit);
    qDebug(dcUniPi()) << "Reading analog Output" << circuit << modbusAddress;

    if (!m_modbusInterface)
        return false;

    return true;
}


bool Neuron::getAnalogInput(const QString &circuit)
{
    int modbusAddress = m_modbusAnalogInputRegisters.value(circuit);
    qDebug(dcUniPi()) << "Reading analog Input" << circuit << modbusAddress;

    if (!m_modbusInterface)
        return false;

    return false;
}

void Neuron::onOutputPollingTimer()
{
    QHash<QString, bool> digitalOutputValues;
    foreach(QString circuit, m_modbusDigitalOutputRegisters.keys()){
        getDigitalOutput(circuit);
        break; //TODO this is a test
    }
}

void Neuron::onInputPollingTimer()
{
    QHash<QString, bool> digitalInputValues;
    foreach(QString circuit, m_modbusDigitalInputRegisters.keys()){
        getDigitalInput(circuit);
        break; //TODO this is a test
    }
}

void Neuron::onFinished()
{
    QModbusReply *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;
    int modbusAddress = 0;

    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();

        for (uint i = 0; i < unit.valueCount(); i++) {
            qCDebug(dcUniPi()) << "Start Address:" << unit.startAddress() << "Register Type:" << unit.registerType() << "Value:" << unit.value(i);
            modbusAddress = unit.startAddress() + i;
            QString circuit;
            switch (unit.registerType()) {
            case QModbusDataUnit::RegisterType::Coils:
                if(m_modbusDigitalInputRegisters.values().contains(unit.startAddress())){
                    circuit = m_modbusDigitalInputRegisters.key(modbusAddress);
                    emit digitalInputStatusChanged(circuit, unit.value(i));
                }

                if(m_modbusDigitalOutputRegisters.values().contains(unit.startAddress())){
                    circuit = m_modbusDigitalOutputRegisters.key(modbusAddress);
                    emit digitalOutputStatusChanged(circuit, unit.value(i));
                }
                break;

            case QModbusDataUnit::RegisterType::DiscreteInputs:
                break;
            case QModbusDataUnit::RegisterType::InputRegisters:
                if(m_modbusAnalogInputRegisters.values().contains(unit.startAddress())){
                    circuit = m_modbusAnalogInputRegisters.key(modbusAddress);
                    emit analogInputStatusChanged(circuit, unit.value(i));
                }

                if(m_modbusAnalogOutputRegisters.values().contains(unit.startAddress())){
                    circuit = m_modbusAnalogOutputRegisters.key(modbusAddress);
                    emit analogOutputStatusChanged(circuit, unit.value(i));
                }
                break;
            case QModbusDataUnit::RegisterType::HoldingRegisters:
                break;
            case QModbusDataUnit::RegisterType::Invalid:
                qCWarning(dcUniPi()) << "Invalide register type";
                break;
            }
        }

    } else if (reply->error() == QModbusDevice::ProtocolError) {
        qCWarning(dcUniPi()) << "Read response error:" << reply->errorString() << reply->rawResult().exceptionCode();
    } else {
        qCWarning(dcUniPi()) << "Read response error:" << reply->error();
    }
    reply->deleteLater();
}
