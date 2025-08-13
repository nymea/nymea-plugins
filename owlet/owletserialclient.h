#ifndef OWLETSERIALCLIENT_H
#define OWLETSERIALCLIENT_H

#include <QTimer>
#include <QObject>
#include <QQueue>

#include "owlettransport.h"

class OwletSerialClientReply;

class OwletSerialClient : public QObject
{
    Q_OBJECT
public:
    enum Command {
        CommandGetFirmwareVersion = 0x00,
        CommandConfigurePin = 0x01,
        CommandWriteDigitalPin = 0x02,
        CommandReadDigitalPin = 0x03,
        CommandWriteAnalogPin = 0x04,
        CommandReadAnalogPin = 0x05,
        CommandWriteServoPin = 0x06
    };
    Q_ENUM(Command)

    enum Notification {
        NotificationReady = 0xf0,
        NotificationGpioPinChanged = 0xf1,
        NotificationDebugMessage = 0xff
    };
    Q_ENUM(Notification)

    enum Status {
        StatusSuccess = 0x00,
        StatusInvalidProtocol = 0x01,
        StatusInvalidCommand = 0x02,
        StatusInvalidPlayload = 0x03,
        StatusTimeout = 0xfe,
        StatusUnknownError = 0xff
    };
    Q_ENUM(Status)

    enum GPIOError {
        GPIOErrorNoError = 0x00,
        GPIOErrorUnconfigured = 0x01,
        GPIOErrorUnsupported = 0x02,
        GPIOErrorConfigurationMismatch = 0x03,
        GPIOErrorInvalidParameter = 0x04,
        GPIOErrorInvalidPin = 0x05
    };
    Q_ENUM(GPIOError)

    enum PinMode {
        PinModeUnconfigured = 0x00,
        PinModeDigitalInput = 0x01,
        PinModeDigitalOutput = 0x02,
        PinModeAnalogInput = 0x03,
        PinModeAnalogOutput = 0x04,
        PinModeServo = 0x05
    };
    Q_ENUM(PinMode)

    explicit OwletSerialClient(OwletTransport *transport, QObject *parent = nullptr);
    ~OwletSerialClient();

    OwletTransport *transport() const;

    bool isConnected() const;
    bool isReady() const;

    QString firmwareVersion() const;

    OwletSerialClientReply *getFirmwareVersion();
    OwletSerialClientReply *configurePin(quint8 pinId, PinMode pinMode);
    OwletSerialClientReply *writeDigitalValue(quint8 pinId, bool power);
    OwletSerialClientReply *readDigitalValue(quint8 pinId);
    OwletSerialClientReply *writeAnalogValue(quint8 pinId, quint8 dutyCycle);
    OwletSerialClientReply *readAnalogValue(quint8 pinId);
    OwletSerialClientReply *writeServoValue(quint8 pinId, quint8 angle);


signals:
    void connected();
    void disconnected();
    void readyChanged(bool ready);
    void error();

    void pinValueChanged(quint8 pinId, bool power);

private slots:
    void dataReceived(const QByteArray &data);

private:
    OwletTransport *m_transport = nullptr;
    bool m_ready = false;

    quint8 m_requestId = 0;

    OwletSerialClientReply *m_currentReply = nullptr;
    QQueue<OwletSerialClientReply *> m_pendingRequests;

    QString m_firmwareVersion;

    OwletSerialClientReply *createReply(const QByteArray &requestData);
    void sendNextRequest();
};


class OwletSerialClientReply : public QObject
{
    Q_OBJECT

    friend class OwletSerialClient;

public:
    QByteArray requestData() const { return m_requestData; };
    OwletSerialClient::Command command() const { return m_command; };
    quint8 requestId() const { return m_requestId; };
    OwletSerialClient::Status status() const { return m_status; };
    QByteArray responsePayload() const { return m_responsePayload; };

signals:
    void finished();

private:
    explicit OwletSerialClientReply(const QByteArray &requestData, QObject *parent = nullptr) :
        QObject(parent),
        m_requestData(requestData)
    {
        Q_ASSERT(m_requestData.length() >= 2);
        m_command = static_cast<OwletSerialClient::Command>(m_requestData.at(0));
        m_requestId = static_cast<quint8>(m_requestData.at(1));

        m_timer.setInterval(1000);
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, [=](){
            m_status = OwletSerialClient::StatusTimeout;
            emit finished();
        });
    }

    QTimer m_timer;
    QByteArray m_requestData;
    OwletSerialClient::Command m_command;
    quint8 m_requestId = 0;

    // Response
    OwletSerialClient::Status m_status = OwletSerialClient::StatusUnknownError;
    QByteArray m_responsePayload;
};


#endif // OWLETSERIALCLIENT_H
