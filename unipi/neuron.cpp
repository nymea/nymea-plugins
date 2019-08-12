/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "neuron.h"
#include "extern-plugininfo.h"

#include <QFile>
#include <QTextStream>
#include <QStandardPaths>

Neuron::Neuron(NeuronTypes neuronType, QModbusTcpClient *modbusInterface,  QObject *parent) :
    QObject(parent),
    m_modbusInterface(modbusInterface),
    m_neuronType(neuronType)
{
    connect(&m_inputPollingTimer, &QTimer::timeout, this, &Neuron::onInputPollingTimer);
    m_inputPollingTimer.setTimerType(Qt::TimerType::PreciseTimer);
    m_inputPollingTimer.start(200);

    connect(&m_outputPollingTimer, &QTimer::timeout, this, &Neuron::onOutputPollingTimer);
    m_outputPollingTimer.setTimerType(Qt::TimerType::PreciseTimer);
    m_outputPollingTimer.setSingleShot(true);
    m_outputPollingTimer.start(1000);
}

Neuron::~Neuron(){
    m_inputPollingTimer.stop();
    m_inputPollingTimer.deleteLater();
    m_outputPollingTimer.stop();
    m_outputPollingTimer.deleteLater();
}

bool Neuron::init()
{
    if (!loadModbusMap()) {
        return false;
    }

    if (!m_modbusInterface) {
        qWarning(dcUniPi()) << "Modbus TCP interface not available";
    }

    if (m_modbusInterface->connectDevice()) {
        qWarning(dcUniPi()) << "Could not connect to modbus TCP device";
    }
    return true;
}

QString Neuron::type()
{
    switch (m_neuronType) {
    case NeuronTypes::S103:
        return  "S103";
    case NeuronTypes::M103:
        return  "M103";
    case NeuronTypes::M203:
        return  "M203";
    case NeuronTypes::M303:
        return  "M303";
    case NeuronTypes::M403:
        return  "M403";
    case NeuronTypes::M503:
        return  "M503";
    case NeuronTypes::L203:
        return  "L203";
    case NeuronTypes::L303:
        return  "L303";
    case NeuronTypes::L403:
        return  "L403";
    case NeuronTypes::L503:
        return  "L503";
    case NeuronTypes::L513:
        return  "L513";
    }
    return "Unknown";
}

QList<QString> Neuron::digitalInputs()
{
    return m_modbusDigitalInputRegisters.keys();
}

QList<QString> Neuron::digitalOutputs()
{
    return m_modbusDigitalOutputRegisters.keys();
}

QList<QString> Neuron::analogInputs()
{
    return m_modbusAnalogInputRegisters.keys();
}

QList<QString> Neuron::analogOutputs()
{
    return m_modbusAnalogOutputRegisters.keys();
}

QList<QString> Neuron::userLEDs()
{
    return m_modbusUserLEDRegisters.keys();
}


bool Neuron::loadModbusMap()
{
    QStringList fileCoilList;
    QStringList fileRegisterList;

    switch (m_neuronType) {
    case NeuronTypes::S103:
        fileCoilList.append(QString("/Neuron_S103/Neuron_S103-Coils-group-1.csv"));
        break;
    case NeuronTypes::M103:
        fileCoilList.append(QString("/Neuron_M103/Neuron_L103-Coils-group-1.csv"));
        fileCoilList.append(QString("/Neuron_M103/Neuron_L103-Coils-group-2.csv"));
        break;
    case NeuronTypes::M203:
        fileCoilList.append(QString("/Neuron_M203/Neuron_M203-Coils-group-1.csv"));
        fileCoilList.append(QString("/Neuron_M203/Neuron_M203-Coils-group-2.csv"));
        break;
    case NeuronTypes::M303:
        fileCoilList.append(QString("/Neuron_M303/Neuron_M303-Coils-group-1.csv"));
        fileCoilList.append(QString("/Neuron_M303/Neuron_M303-Coils-group-2.csv"));
        break;
    case NeuronTypes::M403:
        fileCoilList.append(QString("/Neuron_M403/Neuron_M403-Coils-group-1.csv"));
        fileCoilList.append(QString("/Neuron_M403/Neuron_M403-Coils-group-2.csv"));
        break;
    case NeuronTypes::M503:

        fileCoilList.append(QString("/Neuron_M503/Neuron_M503-Coils-group-1.csv"));
        fileCoilList.append(QString("/Neuron_M503/Neuron_M503-Coils-group-2.csv"));
        break;
    case NeuronTypes::L203:
        fileCoilList.append(QString("/Neuron_L203/Neuron_L203-Coils-group-1.csv"));
        fileCoilList.append(QString("/Neuron_L203/Neuron_L203-Coils-group-2.csv"));
        fileCoilList.append(QString("/Neuron_L203/Neuron_L203-Coils-group-3.csv"));
        break;
    case NeuronTypes::L303:
        fileCoilList.append(QString("/Neuron_L303/Neuron_L303-Coils-group-1.csv"));
        fileCoilList.append(QString("/Neuron_L303/Neuron_L303-Coils-group-2.csv"));
        fileCoilList.append(QString("/Neuron_L303/Neuron_L303-Coils-group-3.csv"));
        break;
    case NeuronTypes::L403:
        fileCoilList.append(QString("/Neuron_L403/Neuron_L403-Coils-group-1.csv"));
        fileCoilList.append(QString("/Neuron_L403/Neuron_L403-Coils-group-2.csv"));
        fileCoilList.append(QString("/Neuron_L403/Neuron_L403-Coils-group-3.csv"));
        break;
    case NeuronTypes::L503:
        fileCoilList.append(QString("/Neuron_L503/Neuron_L503-Coils-group-1.csv"));
        fileCoilList.append(QString("/Neuron_L503/Neuron_L503-Coils-group-2.csv"));
        fileCoilList.append(QString("/Neuron_L503/Neuron_L503-Coils-group-3.csv"));
        break;
    case NeuronTypes::L513:
        fileCoilList.append(QString("/Neuron_L513/Neuron_L513-Coils-group-1.csv"));
        fileCoilList.append(QString("/Neuron_L513/Neuron_L513-Coils-group-2.csv"));
        fileCoilList.append(QString("/Neuron_L513/Neuron_L513-Coils-group-3.csv"));
        break;
    }

    foreach(QString relativeFilePath, fileCoilList) {
        QString absoluteFilePath = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).last() + "/nymea/modbus" + relativeFilePath;
        qDebug(dcUniPi()) << "Open CSV File:" << absoluteFilePath;
        QFile *csvFile = new QFile(absoluteFilePath);
        if (!csvFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCWarning(dcUniPi()) << csvFile->errorString() << "Path:" << absoluteFilePath;
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
                } else if (list[3].contains("User Programmable LED", Qt::CaseSensitivity::CaseInsensitive)) {
                    m_modbusUserLEDRegisters.insert(circuit, list[0].toInt());
                    qDebug(dcUniPi()) << "Found user programmable led" << circuit << list[0].toInt();
                }
            }
        }
        csvFile->close();
        csvFile->deleteLater();
    }

    switch (m_neuronType) {
    case NeuronTypes::S103:
        fileRegisterList.append(QString("/Neuron_S103/Neuron_S103-Registers-group-1.csv"));
        break;
    case NeuronTypes::M103:
        fileRegisterList.append(QString("/Neuron_M103/Neuron_M103-Registers-group-1.csv"));
        fileRegisterList.append(QString("/Neuron_M103/Neuron_M103-Registers-group-2.csv"));
        break;
    case NeuronTypes::M203:
        fileRegisterList.append(QString("/Neuron_M203/Neuron_M203-Registers-group-1.csv"));
        fileRegisterList.append(QString("/Neuron_M203/Neuron_M203-Registers-group-2.csv"));
        break;
    case NeuronTypes::M303:
        fileRegisterList.append(QString("/Neuron_M303/Neuron_M303-Registers-group-1.csv"));
        fileRegisterList.append(QString("/Neuron_M303/Neuron_M303-Registers-group-2.csv"));
        break;
    case NeuronTypes::M403:
        fileRegisterList.append(QString("/Neuron_M403/Neuron_M403-Registers-group-1.csv"));
        fileRegisterList.append(QString("/Neuron_M403/Neuron_M403-Registers-group-2.csv"));
        break;
    case NeuronTypes::M503:
        fileRegisterList.append(QString("/Neuron_M503/Neuron_M503-Registers-group-1.csv"));
        fileRegisterList.append(QString("/Neuron_M503/Neuron_M503-Registers-group-2.csv"));
        break;
    case NeuronTypes::L203:
        fileRegisterList.append(QString("/Neuron_L203/Neuron_L203-Registers-group-1.csv"));
        fileRegisterList.append(QString("/Neuron_L203/Neuron_L203-Registers-group-2.csv"));
        fileRegisterList.append(QString("/Neuron_L203/Neuron_L203-Registers-group-3.csv"));
        break;
    case NeuronTypes::L303:
        fileRegisterList.append(QString("/Neuron_L303/Neuron_L303-Registers-group-1.csv"));
        fileRegisterList.append(QString("/Neuron_L303/Neuron_L303-Registers-group-2.csv"));
        fileRegisterList.append(QString("/Neuron_L303/Neuron_L303-Registers-group-3.csv"));
        break;
    case NeuronTypes::L403:
        fileRegisterList.append(QString("/Neuron_L403/Neuron_L403-Registers-group-1.csv"));
        fileRegisterList.append(QString("/Neuron_L403/Neuron_L403-Registers-group-2.csv"));
        fileRegisterList.append(QString("/Neuron_L403/Neuron_L403-Registers-group-3.csv"));
        break;
    case NeuronTypes::L503:
        fileRegisterList.append(QString("/Neuron_L503/Neuron_L503-Registers-group-1.csv"));
        fileRegisterList.append(QString("/Neuron_L503/Neuron_L503-Registers-group-2.csv"));
        fileRegisterList.append(QString("/Neuron_L503/Neuron_L503-Registers-group-3.csv"));
        break;
    case NeuronTypes::L513:
        fileRegisterList.append(QString("/Neuron_L513/Neuron_L513-Registers-group-1.csv"));
        fileRegisterList.append(QString("/Neuron_L513/Neuron_L513-Registers-group-2.csv"));
        fileRegisterList.append(QString("/Neuron_L513/Neuron_L513-Registers-group-3.csv"));
        break;
    }
    foreach (QString relativeFilePath, fileRegisterList) {
         QString absoluteFilePath = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).last() + "/nymea/modbus" + relativeFilePath;
        qDebug(dcUniPi()) << "Open CSV File:" << absoluteFilePath;
        QFile *csvFile = new QFile(absoluteFilePath);
        if (!csvFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCWarning(dcUniPi()) << csvFile->errorString() << "Path:" << absoluteFilePath;
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


bool Neuron::getAllDigitalInputs()
{
    if (!m_modbusInterface)
        return false;

    QList<QModbusDataUnit> requests;
    QList<int> registerList = m_modbusDigitalInputRegisters.values();

    if (registerList.isEmpty()) {
        return true; //device has no digital inputs
    }

    qSort(registerList);
    int previousReg = registerList.first(); //first register to read and starting point to get the following registers
    int startAddress;

    QHash<int, int> registerGroups;

    foreach (int reg, registerList) {
        //qDebug(dcUniPi()) << "Register" << reg << "previous Register" << previousReg;
        if (reg == previousReg) { //first register
            startAddress = reg;
            registerGroups.insert(startAddress, 1);
        } else if (reg == (previousReg + 1)) { //next register in block
            previousReg = reg;
            registerGroups.insert(startAddress, (registerGroups.value(startAddress) + 1)); //update block length
        } else {    // new block
            startAddress = reg;
            previousReg = reg;
            registerGroups.insert(startAddress, 1);
        }
    }

    foreach (int startAddress, registerGroups.keys()) {
        QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, startAddress, registerGroups.value(startAddress));
        if (QModbusReply *reply = m_modbusInterface->sendReadRequest(request, m_slaveAddress)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, this, &Neuron::onFinished);
                connect(reply, &QModbusReply::errorOccurred, this, &Neuron::onErrorOccured);
                QTimer::singleShot(200, reply, SLOT(deleteLater()));
            } else {
                delete reply; // broadcast replies return immediately
            }
        } else {
            qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
        }
    }
    return true;
}

bool Neuron::getAllDigitalOutputs()
{
    if (!m_modbusInterface)
        return false;

    QList<QModbusDataUnit> requests;
    QList<int> registerList = m_modbusDigitalOutputRegisters.values();

    if (registerList.isEmpty()) {
        return true; //device has no digital outputs
    }

    qSort(registerList);
    int previousReg = registerList.first(); //first register to read and starting point to get the following registers
    int startAddress;

    QHash<int, int> registerGroups;

    foreach (int reg, registerList) {
        //qDebug(dcUniPi()) << "Register" << reg << "previous Register" << previousReg;
        if (reg == previousReg) { //first register
            startAddress = reg;
            registerGroups.insert(startAddress, 1);
        } else if (reg == (previousReg + 1)) { //next register in block
            previousReg = reg;
            registerGroups.insert(startAddress, (registerGroups.value(startAddress) + 1)); //update block length
        } else {    // new block
            startAddress = reg;
            previousReg = reg;
            registerGroups.insert(startAddress, 1);
        }
    }

    foreach (int startAddress, registerGroups.keys()) {
        QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, startAddress, registerGroups.value(startAddress));
        if (QModbusReply *reply = m_modbusInterface->sendReadRequest(request, m_slaveAddress)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, this, &Neuron::onFinished);
                connect(reply, &QModbusReply::errorOccurred, this, &Neuron::onErrorOccured);
                QTimer::singleShot(200, reply, SLOT(deleteLater()));
            } else {
                delete reply; // broadcast replies return immediately
            }
        } else {
            qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
        }
    }
    return true;
}


bool Neuron::getDigitalInput(const QString &circuit)
{
    int modbusAddress = m_modbusDigitalInputRegisters.value(circuit);
    //qDebug(dcUniPi()) << "Reading digital Input" << circuit << modbusAddress;

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, modbusAddress, 1);

    if (QModbusReply *reply = m_modbusInterface->sendReadRequest(request, m_slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &Neuron::onFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &Neuron::onErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }
    return true;
}


bool Neuron::setDigitalOutput(const QString &circuit, bool value)
{
    int modbusAddress = m_modbusDigitalOutputRegisters.value(circuit);
    //qDebug(dcUniPi()) << "Setting digital ouput" << circuit << modbusAddress << value;

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, modbusAddress, 1);
    request.setValue(0, static_cast<uint16_t>(value));

    if (QModbusReply *reply = m_modbusInterface->sendWriteRequest(request, m_slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &Neuron::onFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &Neuron::onErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }
    return true;
}


bool Neuron::getDigitalOutput(const QString &circuit)
{
    int modbusAddress = m_modbusDigitalOutputRegisters.value(circuit);
    //qDebug(dcUniPi()) << "Reading digital Output" << circuit << modbusAddress;

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, modbusAddress, 1);

    if (QModbusReply *reply = m_modbusInterface->sendReadRequest(request, m_slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &Neuron::onFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &Neuron::onErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }
    return true;
}


bool Neuron::setAnalogOutput(const QString &circuit, double value)
{
    int modbusAddress = m_modbusAnalogOutputRegisters.value(circuit);
    qDebug(dcUniPi()) << "Writing analog Output" << circuit << modbusAddress;

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::HoldingRegisters, modbusAddress, 1);
    request.setValue(0, static_cast<uint16_t>(value));

    if (QModbusReply *reply = m_modbusInterface->sendWriteRequest(request, m_slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &Neuron::onFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &Neuron::onErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }
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

bool Neuron::setUserLED(const QString &circuit, bool value)
{
    int modbusAddress = m_modbusUserLEDRegisters.value(circuit);
    //qDebug(dcUniPi()) << "Setting digital ouput" << circuit << modbusAddress << value;

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, modbusAddress, 1);
    request.setValue(0, static_cast<uint16_t>(value));

    if (QModbusReply *reply = m_modbusInterface->sendWriteRequest(request, m_slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &Neuron::onFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &Neuron::onErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }
    return true;
}


bool Neuron::getUserLED(const QString &circuit)
{
    int modbusAddress = m_modbusUserLEDRegisters.value(circuit);
    //qDebug(dcUniPi()) << "Reading digital Output" << circuit << modbusAddress;

    if (!m_modbusInterface)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, modbusAddress, 1);

    if (QModbusReply *reply = m_modbusInterface->sendReadRequest(request, m_slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &Neuron::onFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &Neuron::onErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcUniPi()) << "Read error: " << m_modbusInterface->errorString();
    }
    return true;
}



void Neuron::onOutputPollingTimer()
{
    getAllDigitalOutputs();
    /*foreach (QString circuit, m_modbusDigitalOutputRegisters.keys()) {
        getDigitalOutput(circuit);
    }*/
}

void Neuron::onInputPollingTimer()
{
    getAllDigitalInputs();
    /*foreach (QString circuit, m_modbusDigitalInputRegisters.keys()) {
        getDigitalInput(circuit);
    }*/
}

void Neuron::onFinished()
{
    QModbusReply *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;
    int modbusAddress = 0;

    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();

        for (int i = 0; i < static_cast<int>(unit.valueCount()); i++) {
            //qCDebug(dcUniPi()) << "Start Address:" << unit.startAddress() << "Register Type:" << unit.registerType() << "Value:" << unit.value(i);
            modbusAddress = unit.startAddress() + i;

            if (m_previousModbusRegisterValue.contains(modbusAddress)) {
                if (m_previousModbusRegisterValue.value(modbusAddress) == unit.value(i)) {
                    continue;
                } else  {
                    m_previousModbusRegisterValue.insert(modbusAddress, unit.value(i)); //update existing value
                }
            } else {
                m_previousModbusRegisterValue.insert(modbusAddress, unit.value(i));
            }

            QString circuit;
            switch (unit.registerType()) {
            case QModbusDataUnit::RegisterType::Coils:
                if(m_modbusDigitalInputRegisters.values().contains(modbusAddress)){
                    circuit = m_modbusDigitalInputRegisters.key(modbusAddress);
                    emit digitalInputStatusChanged(circuit, unit.value(i));
                }

                if(m_modbusDigitalOutputRegisters.values().contains(modbusAddress)){
                    circuit = m_modbusDigitalOutputRegisters.key(modbusAddress);
                    emit digitalOutputStatusChanged(circuit, unit.value(i));
                }

                if(m_modbusUserLEDRegisters.values().contains(modbusAddress)){
                    circuit = m_modbusUserLEDRegisters.key(modbusAddress);
                    emit userLEDStatusChanged(circuit, unit.value(i));
                }
                break;

            case QModbusDataUnit::RegisterType::DiscreteInputs:
                break;
            case QModbusDataUnit::RegisterType::InputRegisters:
                if(m_modbusAnalogInputRegisters.values().contains(modbusAddress)){
                    circuit = m_modbusAnalogInputRegisters.key(modbusAddress);
                    emit analogInputStatusChanged(circuit, unit.value(i));
                }

                if(m_modbusAnalogOutputRegisters.values().contains(modbusAddress)){
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

void Neuron::onErrorOccured(QModbusDevice::Error error)
{
    qCWarning(dcUniPi()) << "Modbus replay error:" << error;
    QModbusReply *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;
    reply->finished(); //to make sure it will be deleted
}
