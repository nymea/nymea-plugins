#include "modbustcpclient.h"
#include "extern-plugininfo.h"

ModbusTCPClient::ModbusTCPClient(QHostAddress IPv4Address, int port, int slaveAddress, QObject *parent) :
    QObject(parent),
    m_IPv4Address(IPv4Address),
    m_port(port),
    m_slaveAddress(slaveAddress)
{
    // TCP connction to target device
    qDebug(dcModbusCommander()) << "Setting up TCP connecion" << m_IPv4Address.toString() << "port:" << m_port;
    const char *address = m_IPv4Address.toString().toLatin1().data();
    m_mb = modbus_new_tcp(address, m_port);

    if(m_mb == NULL){
        qCWarning(dcModbusCommander()) << "Error modbus TCP: " << modbus_strerror(errno) ;
       this->deleteLater();
        return;
    }

    struct timeval response_timeout;

    response_timeout.tv_sec = 3;
    response_timeout.tv_usec = 0;
    modbus_set_response_timeout(m_mb, &response_timeout);

    if(modbus_connect(m_mb) == -1){
        qCWarning(dcModbusCommander()) << "Error connecting modbus:" << modbus_strerror(errno) ;
        return;
    }

    if(modbus_set_slave(m_mb, m_slaveAddress) == -1){
        qCWarning(dcModbusCommander()) << "Error setting slave ID" << m_slaveAddress << "Reason:" << modbus_strerror(errno) ;
        //free((void*)(m_mb));
        this->deleteLater();
        return;
    }
}

ModbusTCPClient::~ModbusTCPClient()
{
    if (m_mb != NULL) {
        modbus_close(m_mb);
    }
     modbus_free(m_mb);
}

int ModbusTCPClient::port()
{
    return m_port;
}

QHostAddress ModbusTCPClient::ipv4Address()
{
    return m_IPv4Address;
}

int ModbusTCPClient::slaveAddress()
{
    return m_slaveAddress;
}

bool ModbusTCPClient::connected()
{
    if (!m_mb){
        return false;
    }
    // Check if already connected
    if (modbus_read_input_registers(m_mb, 1, 1, NULL) == -1) { //Register address 1 bloody workaround
        return false;
    } else {
        return true;
    }
}

void ModbusTCPClient::reconnect(int registerAddress)
{
    if (!m_mb){
        return;
    }
    // Check if already connected
    if (modbus_read_input_registers(m_mb, registerAddress, 1, NULL) == -1) {
        qDebug(dcModbusCommander()) << "Could not read register" << registerAddress << "Reason:" << modbus_strerror(errno) ;
        // Try to connect to device
        if (modbus_connect(m_mb) == -1) {
            qCWarning(dcModbusCommander()) << "Connection failed: " << modbus_strerror(errno);
            return;
        }else{
            // recheck the connection
            if (modbus_read_input_registers(m_mb, registerAddress, 1, NULL) == -1)
                return;
        }
    }
}

void ModbusTCPClient::setCoil(int coilAddress, bool status)
{
    if (!m_mb) {
        return;
    }

    if (modbus_write_bit(m_mb, coilAddress, status) == -1)
        qCWarning(dcModbusCommander()) << "Could not write Coil" << coilAddress << "Reason:" << modbus_strerror(errno);
}

void ModbusTCPClient::setRegister(int registerAddress, int data)
{
    if (!m_mb) {
        return;
    }

    if (modbus_write_register(m_mb, registerAddress, data) == -1)
        qCWarning(dcModbusCommander()) << "Could not write Register" << registerAddress << "Reason:" << modbus_strerror(errno);
}

bool ModbusTCPClient::getCoil(int coilAddress)
{
    if (!m_mb){
        return false;
    }

    uint8_t bits;
    if (modbus_read_bits(m_mb, coilAddress, 1, &bits) == -1){
        qCWarning(dcModbusCommander()) << "Could not read bits" << coilAddress << "Reason:"<< modbus_strerror(errno);
    }
    return bits;
}

int ModbusTCPClient::getRegister(int registerAddress)
{
    uint16_t reg;

    if (!m_mb){
        return 0;
    }

    if (modbus_read_registers(m_mb, registerAddress, 1, &reg) == -1){
        qCWarning(dcModbusCommander()) << "Could not read register" << registerAddress << "Reason:" << modbus_strerror(errno);
    }
    return reg;
}
