#include "solarmanmodbusreply.h"

#include <QTimer>
#include <QDataStream>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(dcSolarman)

SolarmanModbusReply::SolarmanModbusReply(quint8 requestId, quint8 slaveId, quint16 startRegister, quint16 endRegister, SolarmanModbus::FunctionCode functionCode, QObject *parent)
    : QObject{parent},
      m_requestId(requestId),
      m_slaveId(slaveId),
      m_startRegister(startRegister),
      m_endRegister(endRegister),
      m_functionCode(functionCode)
{
    QTimer::singleShot(20000, this, [this](){
        finish(false);
    });

}

quint8 SolarmanModbusReply::requestId() const
{
    return m_requestId;
}

quint8 SolarmanModbusReply::slaveId() const
{
    return m_slaveId;
}

quint16 SolarmanModbusReply::startRegister() const
{
    return m_startRegister;
}

quint16 SolarmanModbusReply::endRegister() const
{
    return m_endRegister;
}

quint16 SolarmanModbusReply::readRegister16(quint16 reg) const
{
    int index = (reg - m_startRegister) * 2;
    if (index < 0 || index + 1 >= m_data.length()) {
        qCWarning(dcSolarman()) << "Register" << reg << "out of range:" << m_startRegister << "-" << m_endRegister;
        return 0;
    }
    QByteArray clipped = m_data.right(m_data.length() - (reg - m_startRegister) * 2);
    QDataStream stream(clipped);
    quint16 ret = 0;
    stream >> ret;
    return ret;
}

quint32 SolarmanModbusReply::readRegister32(quint16 reg) const
{
    int index = (reg - m_startRegister) * 2;
    if (index < 0 || index + 3 >= m_data.length()) {
        qCWarning(dcSolarman()) << "Register" << reg << "out of range:" << m_startRegister << "-" << m_endRegister;
        return 0;
    }
    QByteArray clipped = m_data.right(m_data.length() - (reg - m_startRegister) * 2);
    QDataStream stream(clipped);
    quint32 ret = 0;
    quint16 tmp;
    stream >> tmp;
    ret = tmp;
    stream >> tmp;
    ret += (tmp << 16);
    return ret;
}

QString SolarmanModbusReply::readRegisterString(quint16 reg, quint8 registerCount) const
{
    int index = (reg - m_startRegister) * 2;
    if (index < 0 || (index + registerCount) * 2 >= m_data.length()) {
        return QString();
    }
    QByteArray string = m_data.right(m_data.length() - (reg - m_startRegister) * 2).left(registerCount * 2);
    return string;
}

void SolarmanModbusReply::finish(bool success, const QByteArray &data)
{
    if (m_finished) {
        return;
    }
    m_finished = true;
    m_success = success;
    m_data = data;
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection, Q_ARG(bool, success));
}
