#include "speedwiremeter.h"
#include "extern-plugininfo.h"

SpeedwireMeter::SpeedwireMeter(const QHostAddress &address, quint16 modelId, quint32 serialNumber, QObject *parent) :
    QObject(parent),
    m_address(address),
    m_modelId(modelId),
    m_serialNumber(serialNumber)
{
    m_interface = new SpeedwireInterface(m_address, true, this);
    connect(m_interface, &SpeedwireInterface::dataReceived, this, &SpeedwireMeter::processData);
}

bool SpeedwireMeter::initialize()
{
    return m_interface->initialize();
}

bool SpeedwireMeter::initialized() const
{
    return m_interface->initialized();
}

double SpeedwireMeter::currentPower() const
{
    return m_currentPower;
}

double SpeedwireMeter::totalEnergyProduced() const
{
    return m_totalEnergyProduced;
}

double SpeedwireMeter::totalEnergyConsumed() const
{
    return m_totalEnergyConsumed;
}

double SpeedwireMeter::energyConsumedPhaseA() const
{
    return m_energyConsumedPhaseA;
}

double SpeedwireMeter::energyConsumedPhaseB() const
{
    return m_energyConsumedPhaseB;
}

double SpeedwireMeter::energyConsumedPhaseC() const
{
    return m_energyConsumedPhaseC;
}

double SpeedwireMeter::energyProducedPhaseA() const
{
    return m_energyProducedPhaseA;
}

double SpeedwireMeter::energyProducedPhaseB() const
{
    return m_energyProducedPhaseB;
}

double SpeedwireMeter::energyProducedPhaseC() const
{
    return m_energyProducedPhaseC;
}

double SpeedwireMeter::currentPowerPhaseA() const
{
    return m_currentPowerPhaseA;
}

double SpeedwireMeter::currentPowerPhaseB() const
{
    return m_currentPowerPhaseB;
}

double SpeedwireMeter::currentPowerPhaseC() const
{
    return m_currentPowerPhaseC;
}

double SpeedwireMeter::voltagePhaseA() const
{
    return m_voltagePhaseA;
}

double SpeedwireMeter::voltagePhaseB() const
{
    return m_voltagePhaseB;
}

double SpeedwireMeter::voltagePhaseC() const
{
    return m_voltagePhaseC;
}

double SpeedwireMeter::amperePhaseA() const
{
    return m_amperePhaseA;
}

double SpeedwireMeter::amperePhaseB() const
{
    return m_amperePhaseB;
}

double SpeedwireMeter::amperePhaseC() const
{
    return m_amperePhaseC;
}

QString SpeedwireMeter::softwareVersion() const
{
    return m_softwareVersion;
}

void SpeedwireMeter::processData(const QByteArray &data)
{
    qCDebug(dcSma()) << "Meter: data received" << data.toHex();
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);

    SpeedwireInterface::SpeedwireHeader header = SpeedwireInterface::parseHeader(stream);
    if (!header.isValid()) {
        qCDebug(dcSma()) << "Meter: Datagram header is not valid. Ignoring data...";
        return;
    }

    if (header.protocolId != SpeedwireInterface::ProtocolIdMeter) {
        qCDebug(dcSma()) << "Meter: received header protocol which is not from the meter protocol. Ignoring data...";
        return;
    }

    quint16 modelId;
    quint32 serialNumber;
    stream >> modelId >> serialNumber;
    if (m_modelId != modelId && serialNumber != m_serialNumber) {
        qCDebug(dcSma()) << "Meter: received meter data from an other meter. Ignoring data...";
    }

    qCDebug(dcSma()) << "Meter: Model ID:" << modelId;
    qCDebug(dcSma()) << "Meter: Serial number:" << serialNumber;

    // Parse the packet data
    // Timestamp e618a416
    qCDebug(dcSma()) << "Meter: ======================= Meter measurements";
    quint32 timestamp;
    stream >> timestamp;
    qCDebug(dcSma()) << "Meter: Timestamp:" << timestamp;

    // Obis data
    //00 01 04 00 00000000 00 01 08 00 0000002139122910 00 02 04 00 00004415 00 02 08 00 0000001575a137d8 00 03 04 00 00000000 00 03 08 00 00000003debed0e8 00040400000017c6000408000000001008c2070000090400000000000009080000000027c77bed20000a04000000481d000a08000000001722823410000d0400000003b00015040000000000001508000000000d1e1e0e3000160400000015120016080000000006c5a2d8b800170400000000000017080000000001bd6f680000180400000007990018080000000004def712b8001d040000000000001d08000000000eeefaafd0001e040000001666001e0800000000074b38bf88001f040000000a300020040000037bcb00210400000003ad0029040000000000002908000000000a9b1afec8002a040000001a81002a08000000000803e62b88002b040000000000002b080000000001511459b8002c0400000006d5002c0800000000052c8455b80031040000000000003108000000000cf83b37100032040000001b5f0032080000000008a6e257f80033040000000c3f003404000003747900350400000003c8003d040000000000003d08000000000a53d0ba08003e040000001482003e080000000007800fd188003f040000000000003f080000000001185820c8004004000000095800400800000000064563b1900045040000000000004508000000000d26d3eae0004604000000168900460800000000082b4fc5a80047040000000a440048040000037ed1004904000000038e90000000 01020852 00000000
    while (!stream.atEnd()) {
        quint8 measurementChannel;
        quint8 measurementIndex;
        quint8 measurmentType;
        quint8 measurmentTariff;

        stream >> measurementChannel >> measurementIndex >> measurmentType >> measurmentTariff;

        if (measurmentType == 4) {
            qint32 measurement;
            stream >> measurement;

            if (measurementIndex == 1 && measurement != 0) {
                m_currentPower = measurement / 10.0;
                qCDebug(dcSma()) << "Meter: Current power" << m_currentPower << "W";
            } else if (measurementIndex == 2 && measurement != 0) {
                m_currentPower = -measurement / 10.0;
                qCDebug(dcSma()) << "Meter: Current power" << m_currentPower << "W";
            } else if (measurementIndex == 21 && measurement != 0) {
                m_currentPowerPhaseA = measurement / 10.0;
                qCDebug(dcSma()) << "Meter: Current power phase A" << m_currentPowerPhaseA << "W";
            } else if (measurementIndex == 22 && measurement != 0) {
                m_currentPowerPhaseA = -measurement / 10.0;
                qCDebug(dcSma()) << "Meter: Current power phase A" << m_currentPowerPhaseA << "W";
            } else if (measurementIndex == 41 && measurement != 0) {
                m_currentPowerPhaseB = measurement / 10.0;
                qCDebug(dcSma()) << "Meter: Current power phase B" << m_currentPowerPhaseB << "W";
            } else if (measurementIndex == 42 && measurement != 0) {
                m_currentPowerPhaseB = -measurement / 10.0;
                qCDebug(dcSma()) << "Meter: Current power phase B" << m_currentPowerPhaseB << "W";
            } else if (measurementIndex == 61 && measurement != 0) {
                m_currentPowerPhaseC = measurement / 10.0;
                qCDebug(dcSma()) << "Meter: Current power phase C" << m_currentPowerPhaseC << "W";
            } else if (measurementIndex == 62 && measurement != 0) {
                m_currentPowerPhaseC = -measurement / 10.0;
                qCDebug(dcSma()) << "Meter: Current power phase C" << m_currentPowerPhaseC << "W";
            } else if (measurementIndex == 31) {
                m_amperePhaseA = measurement / 1000.0;
                qCDebug(dcSma()) << "Meter: Ampere phase A" << m_amperePhaseA << "A";
            } else if (measurementIndex == 51) {
                m_amperePhaseB = measurement / 1000.0;
                qCDebug(dcSma()) << "Meter: Ampere phase B" << m_amperePhaseB << "A";
            } else if (measurementIndex == 71) {
                m_amperePhaseC = measurement / 1000.0;
                qCDebug(dcSma()) << "Meter: Ampere phase C" << m_amperePhaseC << "A";
            } else if (measurementIndex == 32) {
                m_voltagePhaseA = measurement / 1000.0;
                qCDebug(dcSma()) << "Meter: Voltage phase A" << m_voltagePhaseA << "V";
            } else if (measurementIndex == 52) {
                m_voltagePhaseB = measurement / 1000.0;
                qCDebug(dcSma()) << "Meter: Voltage phase B" << m_voltagePhaseB << "V";
            } else if (measurementIndex == 72) {
                m_voltagePhaseC = measurement / 1000.0;
                qCDebug(dcSma()) << "Meter: Voltage phase C" << m_voltagePhaseC << "V";
            } else  {
//                qCDebug(dcSma()) << "Meter: --> Channel:" << measurementChannel << "Index:" <<  measurementIndex << "Type:" << measurmentType << "Rate:" << measurmentTariff;
//                qCDebug(dcSma()) << "Meter: Value:" << measurement;
            }


        } else if (measurmentType == 8) {
            qint64 measurement;
            stream >> measurement;

            if (measurementIndex == 1 && measurement != 0) {
                m_totalEnergyConsumed = measurement / 3600000.0;
                qCDebug(dcSma()) << "Total energy consumed" << m_totalEnergyConsumed << "kWh";
            } else if (measurementIndex == 2 && measurement != 0) {
                m_totalEnergyProduced = measurement / 3600000.0;
                qCDebug(dcSma()) << "Total energy produced" << m_totalEnergyProduced << "kWh";
            } else if (measurementIndex == 21 && measurement != 0) {
                m_energyConsumedPhaseA = measurement / 3600000.0;
                qCDebug(dcSma()) << "Energy consumed phase A" << m_energyConsumedPhaseA << "kWh";
            } else if (measurementIndex == 41 && measurement != 0) {
                m_energyConsumedPhaseB = measurement / 3600000.0;
                qCDebug(dcSma()) << "Energy consumed phase B" << m_energyConsumedPhaseB << "kWh";
            } else if (measurementIndex == 61 && measurement != 0) {
                m_energyConsumedPhaseC = measurement / 3600000.0;
                qCDebug(dcSma()) << "Energy consumed phase C" << m_energyConsumedPhaseC << "kWh";
            } else if (measurementIndex == 22 && measurement != 0) {
                m_energyProducedPhaseA = measurement / 3600000.0;
                qCDebug(dcSma()) << "Energy produced phase A" << m_energyProducedPhaseA << "kWh";
            } else if (measurementIndex == 42 && measurement != 0) {
                m_energyProducedPhaseB = measurement / 3600000.0;
                qCDebug(dcSma()) << "Energy produced phase B" << m_energyProducedPhaseB << "kWh";
            } else if (measurementIndex == 62 && measurement != 0) {
                m_energyProducedPhaseC = measurement / 3600000.0;
                qCDebug(dcSma()) << "Energy produced phase C" << m_energyProducedPhaseC << "kWh";
            } else {
//                qCDebug(dcSma()) << "Meter: --> Channel:" << measurementChannel << "Index:" <<  measurementIndex << "Type:" << measurmentType << "Rate:" << measurmentTariff;
//                qCDebug(dcSma()) << "Meter: Value:" << measurement;
            }

        } if (measurementChannel == 144 && measurementIndex == 0 && measurmentType == 0 && measurmentTariff == 0) {
            // Software version
            // 90000000 01 02 08 52
            quint8 major, minor, build, revision;
            stream >> major >> minor >> build >> revision;
            // Revision types:
            //  S: Special version
            //  A: Alpha version
            //  B: Beta version
            //  R: Release version
            //  E: Experimental version
            //  N: No revision
            m_softwareVersion = QString("%1.%2.%3-%4").arg(major).arg(minor).arg(build).arg(QChar(revision));

            qCDebug(dcSma()) << "Meter: Software version" << m_softwareVersion;
        } else if (measurementChannel == 0 && measurementIndex == 0 && measurmentType == 0 && measurmentTariff == 0) {
            //  00 00 00 00
            //qCDebug(dcSma()) << "Meter: End of data reached.";
        }
    }

    emit valuesUpdated();
}
