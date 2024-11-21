#ifndef NYMEALIGHTSERIALINTERFACE_H
#define NYMEALIGHTSERIALINTERFACE_H

#include <QTimer>
#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>

#include "nymealightinterface.h"

class NymeaLightSerialInterface : public NymeaLightInterface
{
    Q_OBJECT
public:
    explicit NymeaLightSerialInterface(const QString &name, QObject *parent = nullptr);
    ~NymeaLightSerialInterface() override = default;

    bool open() override;
    void close() override;
    bool available() override;

    void sendData(const QByteArray &data) override;

private:
    enum SlipProtocol {
        SlipProtocolEnd = 0xC0,
        SlipProtocolEsc = 0xDB,
        SlipProtocolTransposedEnd = 0xDC,
        SlipProtocolTransposedEsc = 0xDD
    };

    QString m_serialPortName;
    QTimer *m_reconnectTimer = nullptr;
    QSerialPort *m_serialPort = nullptr;
    QByteArray m_buffer;
    bool m_protocolEscaping = false;

private slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);

};

#endif // NYMEALIGHTSERIALINTERFACE_H
