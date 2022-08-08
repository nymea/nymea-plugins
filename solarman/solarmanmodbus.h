#ifndef SOLARMANMODBUS_H
#define SOLARMANMODBUS_H

#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>

class SolarmanModbusReply;

class SolarmanModbus : public QObject
{
    Q_OBJECT
public:
    enum FunctionCode {
        Invalid =                 0x00,
        ReadCoilStatus =          0x01,
        ReadInputStatus =         0x02,
        ReadHoldingRegisters =    0x03,
        ReadInputRegisters =      0x04,
        ForceSingleCoil =         0x05,
        ForceSingleRegister =     0x06,
        ForceMultipleCoils =      0x0F,
        PresetMultipleRegisters = 0x10
    };
    Q_ENUM(FunctionCode)

    explicit SolarmanModbus(QObject *parent = nullptr);

    void connectToHost(const QHostAddress &host, quint16 port, const QString &serial);
    void disconnectFromHost();
    bool isConnected() const;

    SolarmanModbusReply* readRegisters(int slaveId, quint16 startRegister, quint16 endRegister, FunctionCode functionCode);

signals:
    void connectedChanged(bool connected);

private slots:
    void processData(const QByteArray &data);

private:
    QByteArray createRequest(quint8 requestId, quint8 slaveId, quint16 startRegister, quint16 endRegister, FunctionCode functionCode, const QString &serialNumber);
    QByteArray createModbusPayload(quint8 slaveId, quint16 startRegister, quint16 endRegister, FunctionCode functionCode);
    quint16 createModbusCRC(const QByteArray &data);
    QByteArray getSerialHex(const QString &serialNumber);

    quint8 createChecksum(const QByteArray &data);
    bool validateChecksum(const QByteArray &data);

    QString m_serial;
    QHostAddress m_host;
    quint16 m_port = 0;
    QTcpSocket *m_socket = nullptr;
    quint8 m_requestIdCounter = 0;

    QList<SolarmanModbusReply*> m_pendingReplies;
};

#endif // SOLARMANMODBUS_H
