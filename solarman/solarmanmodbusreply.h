#ifndef SOLARMANMODBUSREPLY_H
#define SOLARMANMODBUSREPLY_H

#include <QObject>
#include "solarmanmodbus.h"

class SolarmanModbusReply : public QObject
{
    friend class SolarmanModbus;
    Q_OBJECT
public:
    explicit SolarmanModbusReply(quint8 requestId, quint8 slaveId, quint16 startRegister, quint16 endRegister, SolarmanModbus::FunctionCode, QObject *parent = nullptr);

    quint8 requestId() const;
    quint8 slaveId() const;
    quint16 startRegister() const;
    quint16 endRegister() const;

    quint16 readRegister16(quint16 reg) const;
    quint32 readRegister32(quint16 reg) const;
    QString readRegisterString(quint16 reg, quint8 registerCount) const;

signals:
    void finished(bool success);

private:
    void finish(bool success, const QByteArray &data = QByteArray());

    quint8 m_requestId = 0;
    quint8 m_slaveId = 0;
    quint16 m_startRegister = 0;
    quint16 m_endRegister = 0;
    SolarmanModbus::FunctionCode m_functionCode = SolarmanModbus::FunctionCode::Invalid;
    QByteArray m_data;
    bool m_success = false;
    bool m_finished = false;

};

#endif // SOLARMANMODBUSREPLY_H
