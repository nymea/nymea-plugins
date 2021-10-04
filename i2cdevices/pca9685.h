#ifndef PCA9685_H
#define PCA9685_H

#include <QObject>

#include <hardware/i2c/i2cdevice.h>

#define MODE1			0x00
#define MODE2			0x01
#define SUBADR1			0x02
#define SUBADR2			0x03
#define SUBADR3			0x04
#define PRESCALE		0xFE
#define LED0_ON_L		0x06
#define LED0_ON_H		0x07
#define LED0_OFF_L		0x08
#define LED0_OFF_H		0x09
#define ALL_LED_ON_L    0xFA
#define ALL_LED_ON_H	0xFB
#define ALL_LED_OFF_L	0xFC
#define ALL_LED_OFF_H	0xFD
// Bits
#define RESTART			0x80
#define SLEEP			0x10
#define ALLCALL			0x01
#define INVRT			0x10
#define OUTDRV			0x04

#define PAN			    0
#define TILT			1
#define FREQUENCY		50
#define CLOCKFREQ		25000000
#define PANOFFSET		1
#define PANSCALE		1.4
#define TILTOFFSET		30
#define TILTSCALE		1.43
#define PANMAX			270
#define PANMIN			90
#define TILTMAX			90
#define TILTMIN			-45

class Pca9685 : public I2CDevice
{
    Q_OBJECT
public:
    explicit Pca9685(const QString &portName, int address, QObject *parent = nullptr);


    bool writeData(int fd, const QByteArray &data) override;

private:
    bool writeByte8(int fd, uint8_t registerAddress, uint8_t data);
    uint8_t readByte8(int fd, uint8_t registerAddress);

};

#endif // PCA9685_H
