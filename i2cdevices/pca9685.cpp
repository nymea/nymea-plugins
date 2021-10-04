#include "pca9685.h"

#include <unistd.h>

#include "extern-plugininfo.h"

#include <QThread>
#include <qmath.h>

Pca9685::Pca9685(const QString &portName, int address, QObject *parent) : I2CDevice(portName, address, parent)
{

}

bool Pca9685::writeData(int fd, const QByteArray &command)
{
    if (command == "init") {
        writeByte8(fd, MODE2, OUTDRV);
        writeByte8(fd, MODE1, ALLCALL);
        QThread::msleep(5);
        quint8 mode1 = readByte8(fd, MODE1);
        mode1 = mode1 & ~SLEEP;
        writeByte8(fd, MODE1, mode1);
        QThread::msleep(5);
        qCInfo(dcI2cDevices())<<"PCA9685 Initialization: SUCCESS";
        return true;
    }

    if (command.startsWith("PWM")) {
        QList<QByteArray> parts = command.split(':');
        quint8 channel = parts.at(1).toUInt();
        quint16 on = parts.at(2).toUInt();
        quint16 off = parts.at(3).toUInt();
        writeByte8(fd, LED0_ON_L+4*channel, on & 0xFF);
        writeByte8(fd, LED0_ON_H+4*channel, on >> 8);
        writeByte8(fd, LED0_OFF_L+4*channel, off & 0xFF);
        writeByte8(fd, LED0_OFF_H+4*channel, off >> 8);
        return true;
    }

    if (command.startsWith("FREQ")) {
        QList<QByteArray> parts = command.split(':');
        quint16 frequency = parts.at(1).toUInt();
        float prescaleval = 25000000.0; //25MHz
        prescaleval /= 4096.0;         //12-bit
        prescaleval /= float(frequency);
        prescaleval -= 1.0;
        qInfo()<<"Setting PWM frequency to "<<frequency<<" hz";
        qInfo()<<"Estimated pre-scale: "<<prescaleval;
        int prescale = (int)qFloor(prescaleval + 0.5);
        qInfo()<<"Final pre-scale: "<<prescale;
        uint8_t oldmode = readByte8(fd, MODE1);
        uint8_t newmode = (oldmode & 0x7F) | 0x10; // sleep
        writeByte8(fd, MODE1, newmode);      //go to sleep
        writeByte8(fd, PRESCALE, prescale);
        writeByte8(fd, MODE1, oldmode);
        //time.sleep(0.005)  # wait
        QThread::msleep(5);
        writeByte8(fd, MODE1, oldmode | 0x80);
        return true;
    }

    return false;
}

bool Pca9685::writeByte8(int fd, uint8_t registerAddress, uint8_t data)
{
    uint8_t buffer[2] = {0};
    buffer[0] = registerAddress & 0x00FF;
    buffer[1] = data;

    if (write(fd, buffer, 2) != 2){
        qCDebug(dcI2cDevices())<<"I2C Transaction failed!";
        return false;
    }
    return true;
}

uint8_t Pca9685::readByte8(int fd, uint8_t registerAddress)
{
    uint8_t buffer[1] = {0};
    buffer[0] = registerAddress & 0x00FF;
    write(fd, buffer, 1);

    uint8_t result_buffer[1] = {0};
    if (read(fd, result_buffer, 1) != 1){
        qDebug()<<"I2C Transaction failed!";
        return 0;
    }

    return result_buffer[0];
}
