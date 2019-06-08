#include "neuronextension.h"
#include "extern-plugininfo.h"

#include <QFile>
#include <QTextStream>
#include <QModbusDataUnit>

NeuronExtension::NeuronExtension(ExtensionTypes extensionType, QModbusRtuSerialMaster *modbusInterface, int slaveAddress, QObject *parent) :
    QObject(parent),
    m_modbusInterface(modbusInterface),
    m_slaveAddress(slaveAddress),
    m_extensionType(extensionType)
{
    connect(&m_inputPollingTimer, &QTimer::timeout, this, &NeuronExtension::onInputPollingTimer);
    m_inputPollingTimer.setTimerType(Qt::TimerType::PreciseTimer);
    m_inputPollingTimer.start(200);

    connect(&m_outputPollingTimer, &QTimer::timeout, this, &NeuronExtension::onOutputPollingTimer);
    m_outputPollingTimer.setTimerType(Qt::TimerType::PreciseTimer);
    m_outputPollingTimer.start(5000);
}

NeuronExtension::~NeuronExtension(){
    m_inputPollingTimer.stop();
    m_inputPollingTimer.deleteLater();
    m_outputPollingTimer.stop();
    m_outputPollingTimer.deleteLater();
}

bool NeuronExtension::init() {

    if (!loadModbusMap()) {
        return false;
    }

    if (!m_modbusInterface) {
        qWarning(dcUniPi()) << "Modbus RTU interface not available";
    }

    if (m_modbusInterface->connectDevice()) {
        qWarning(dcUniPi()) << "Could not connect to RTU device";
    }
    return true;
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
    int modbusAddress = m_modbusDigitalInputRegisters.value(circuit);
    //qDebug(dcUniPi()) << "Reading digital input" << circuit << modbusAddress;

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, modbusAddress, 1);

    if (QModbusReply *reply = m_modbusInterface->sendReadRequest(request, m_slaveAddress)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &NeuronExtension::onFinished);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }
    return true;
}


bool NeuronExtension::setDigitalOutput(const QString &circuit, bool value)
{
    int modbusAddress = m_modbusDigitalOutputRegisters.value(circuit);
    //qDebug(dcUniPi()) << "Setting digital ouput" << circuit << modbusAddress;

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, modbusAddress, 1);
    request.setValue(0, static_cast<uint16_t>(value));

    if (QModbusReply *reply = m_modbusInterface->sendWriteRequest(request, m_slaveAddress)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &NeuronExtension::onFinished);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }
    return true;
}

bool NeuronExtension::getDigitalOutput(const QString &circuit)
{
    int modbusAddress = m_modbusDigitalOutputRegisters.value(circuit);
    //qDebug(dcUniPi()) << "Reading digital output" << circuit << modbusAddress;

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, modbusAddress, 1);

    if (QModbusReply *reply = m_modbusInterface->sendReadRequest(request, m_slaveAddress)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &NeuronExtension::onFinished);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }
    return true;
}


bool NeuronExtension::getAllDigitalInputs()
{
    if (!m_modbusInterface)
        return false;

    QList<QModbusDataUnit> requests;
    QList<int> registerList = m_modbusDigitalInputRegisters.values();

    if (registerList.isEmpty()) {
        return true; //device has no inputs
    }

    qSort(registerList);
    int previousReg = 0;
    int count = 0;
    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils);
    request.setStartAddress(registerList.first());
    requests.append(request);

    foreach (int reg, registerList) {

        if (reg == (previousReg + 1)) {
            count++;
        } else {
            requests.last().setValueCount(count);
            count = 0;
            QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils);
            request.setStartAddress(reg);
            requests.last().setValueCount(1);
            requests.append(request);
        }
    }

    foreach (QModbusDataUnit request, requests) {
        if (QModbusReply *reply = m_modbusInterface->sendReadRequest(request, m_slaveAddress)) {
            if (!reply->isFinished())
                connect(reply, &QModbusReply::finished, this, &NeuronExtension::onFinished);
            else
                delete reply; // broadcast replies return immediately
        } else {
            qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
        }
    }
    return true;
}

bool NeuronExtension::getAllDigitalOutputs()
{
    if (!m_modbusInterface)
        return false;

    QList<QModbusDataUnit> requests;
    QList<int> registerList = m_modbusDigitalOutputRegisters.values();

    if (registerList.isEmpty()) {
        return true; //device has no digital outputs
    }

    qSort(registerList);
    int previousReg = 0;
    int count = 0;
    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils);
    request.setStartAddress(registerList.first());
    requests.append(request);

    foreach (int reg, registerList) {

        if (reg == (previousReg + 1)) {
            count++;
        } else {
            requests.last().setValueCount(count);
            count = 0;
            QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils);
            request.setStartAddress(reg);
            requests.last().setValueCount(1);
            requests.append(request);
        }
    }

    foreach (QModbusDataUnit request, requests) {
        if (QModbusReply *reply = m_modbusInterface->sendReadRequest(request, m_slaveAddress)) {
            if (!reply->isFinished())
                connect(reply, &QModbusReply::finished, this, &NeuronExtension::onFinished);
            else
                delete reply; // broadcast replies return immediately
        } else {
            qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
        }
    }
    return true;
}

bool NeuronExtension::setAnalogOutput(const QString &circuit, double value)
{
    int modbusAddress = m_modbusAnalogOutputRegisters.value(circuit);
    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::InputRegisters, modbusAddress, 2);
    request.setValue(0, static_cast<uint16_t>(value));
    //TODO cast double to 2 uint16_t

    if (QModbusReply *reply = m_modbusInterface->sendWriteRequest(request, m_slaveAddress)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &NeuronExtension::onFinished);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }
    return true;
}


bool NeuronExtension::getAnalogOutput(const QString &circuit)
{
    int modbusAddress = m_modbusAnalogOutputRegisters.value(circuit);

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::InputRegisters, modbusAddress, 1);

    if (QModbusReply *reply = m_modbusInterface->sendReadRequest(request, m_slaveAddress)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &NeuronExtension::onFinished);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }
    return true;
}


bool NeuronExtension::getAnalogInput(const QString &circuit)
{
    int modbusAddress =  m_modbusAnalogInputRegisters.value(circuit);

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::InputRegisters, modbusAddress, 1);

    if (QModbusReply *reply = m_modbusInterface->sendReadRequest(request, m_slaveAddress)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &NeuronExtension::onFinished);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }
    return true;
}

void NeuronExtension::onOutputPollingTimer()
{
    getAllDigitalOutputs();
}

void NeuronExtension::onInputPollingTimer()
{
    getAllDigitalInputs();
}


void NeuronExtension::onFinished()
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
