#ifndef OWLETSERIALTRANSPORT_H
#define OWLETSERIALTRANSPORT_H

#include <QObject>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>

#include "owlettransport.h"

class OwletSerialTransport : public OwletTransport
{
    Q_OBJECT
public:
    explicit OwletSerialTransport(const QString &serialPortName, uint baudrate, QObject *parent = nullptr);

    bool connected() const override;
    void sendData(const QByteArray &data) override;

public slots:
    void connectTransport() override;
    void disconnectTransport() override;

private slots:
    void onReadyRead();
    void onError(QSerialPort::SerialPortError serialPortError);

private:

    enum SlipProtocol {
        SlipProtocolEnd = 0xC0,
        SlipProtocolEsc = 0xDB,
        SlipProtocolTransposedEnd = 0xDC,
        SlipProtocolTransposedEsc = 0xDD
    };

    QSerialPort *m_serialPort = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    QString m_serialPortName;
    uint m_baudrate;

    QByteArray m_buffer;
    bool m_protocolEscaping = false;
};

Q_DECLARE_METATYPE(QSerialPort::SerialPortError)

#endif // OWLETSERIALTRANSPORT_H
