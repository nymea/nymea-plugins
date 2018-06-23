#ifndef MODBUSTCPCLIENT_H
#define MODBUSTCPCLIENT_H

#include <QObject>
#include <QHostAddress>
#include <modbus/modbus.h>

class ModbusTCPClient : public QObject
{
    Q_OBJECT
public:
    explicit ModbusTCPClient(QHostAddress IPv4Address, int port, int slaveAddress, QObject *parent = 0);
    ~ModbusTCPClient();

    bool getCoil(int coilAddress);
    int getRegister(int registerAddress);
    void setCoil(int coilAddress, bool status);
    void setRegister(int registerAddress, int data);
    void reconnect(int address);
    QHostAddress ipv4Address();
    int port();
    int slaveAddress();
    bool connected();
private:
     modbus_t *m_mb;
     QHostAddress m_IPv4Address;
     int m_port;
     int m_slaveAddress;

signals:

public slots:
};

#endif // MODBUSTCPCLIENT_H
