#include "nymealightserialinterface.h"
#include "extern-plugininfo.h"

#include <QDataStream>

NymeaLightSerialInterface::NymeaLightSerialInterface(const QString &name, QObject *parent) :
    NymeaLightInterface(parent)
{
    m_serialPort = new QSerialPort(name, this);
    m_serialPort->setBaudRate(115200);
    m_serialPort->setDataBits(QSerialPort::DataBits::Data8);
    m_serialPort->setParity(QSerialPort::Parity::NoParity);
    m_serialPort->setStopBits(QSerialPort::StopBits::OneStop);
    m_serialPort->setFlowControl(QSerialPort::FlowControl::NoFlowControl);

    connect(m_serialPort, &QSerialPort::readyRead, this, &NymeaLightSerialInterface::onReadyRead);
    connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(onSerialError(QSerialPort::SerialPortError)));

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(5000);
    m_reconnectTimer->setSingleShot(false);
    connect(m_reconnectTimer, &QTimer::timeout, this, [=](){
        if (m_serialPort->isOpen()) {
            m_reconnectTimer->stop();
            return;
        } else {
            if (open()) {
                m_reconnectTimer->stop();
            }
        }
    });
}

bool NymeaLightSerialInterface::open()
{
    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        qCWarning(dcWs2812fx()) << "Could not open serial port" << m_serialPort->portName() << m_serialPort->errorString();
        return false;
    }

    emit availableChanged(true);
    return true;
}

void NymeaLightSerialInterface::close()
{
    m_serialPort->close();
    emit availableChanged(false);
}

bool NymeaLightSerialInterface::available()
{
    return m_serialPort->isOpen();
}

void NymeaLightSerialInterface::sendData(const QByteArray &data)
{
    // Stream bytes using SLIP
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    stream << static_cast<quint8>(SlipProtocolEnd);

    for (int i = 0; i < data.length(); i++) {
        quint8 dataByte = data.at(i);
        switch (dataByte) {
        case SlipProtocolEnd:
            stream << static_cast<quint8>(SlipProtocolEsc);
            stream << static_cast<quint8>(SlipProtocolTransposedEnd);
            break;
        case SlipProtocolEsc:
            stream << static_cast<quint8>(SlipProtocolEsc);
            stream << static_cast<quint8>(SlipProtocolTransposedEsc);
            break;
        default:
            stream << dataByte;
            break;
        }
    }
    stream << static_cast<quint8>(SlipProtocolEnd);

    qCDebug(dcWs2812fx()) << "UART -->" << message.toHex();
    m_serialPort->write(message);
    m_serialPort->flush();
}

void NymeaLightSerialInterface::onReadyRead()
{
    QByteArray data = m_serialPort->readAll();
    for (int i = 0; i < data.length(); i++) {
        quint8 receivedByte = data.at(i);

        if (m_protocolEscaping) {
            switch (receivedByte) {
            case SlipProtocolTransposedEnd:
                m_buffer.append(static_cast<quint8>(SlipProtocolEnd));
                m_protocolEscaping = false;
                break;
            case SlipProtocolTransposedEsc:
                m_buffer.append(static_cast<quint8>(SlipProtocolEsc));
                m_protocolEscaping = false;
                break;
            default:
                // SLIP protocol violation...received escape, but it is not an escaped byte
                break;
            }
        }

        switch (receivedByte) {
        case SlipProtocolEnd:
            // We are done with this package, process it and reset the buffer
            if (!m_buffer.isEmpty() && m_buffer.length() >= 3) {
                qCDebug(dcWs2812fx()) << "UART <--" << m_buffer.toHex();
                emit dataReceived(m_buffer);
            }
            m_buffer.clear();
            m_protocolEscaping = false;
            break;
        case SlipProtocolEsc:
            // The next byte will be escaped, lets wait for it
            m_protocolEscaping = true;
            break;
        default:
            // Nothing special, just add to buffer
            m_buffer.append(receivedByte);
            break;
        }
    }
}

void NymeaLightSerialInterface::onSerialError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError && m_serialPort->isOpen()) {
        qCCritical(dcWs2812fx()) << "Serial port error:" << error << m_serialPort->errorString();
        m_reconnectTimer->start();
        m_serialPort->close();
        emit availableChanged(false);
    }
}
