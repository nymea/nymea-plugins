#ifndef NYMEALIGHTINTERFACE_H
#define NYMEALIGHTINTERFACE_H

#include <QObject>

class NymeaLightInterface : public QObject
{
    Q_OBJECT
public:
    enum Command {
        CommandSetColor = 0x00,
        CommandSetBrightness = 0x01,
        CommandSetSpeed = 0x02,
        CommandSetEffect = 0x03,
        CommandCustom = 0xFF
    };
    Q_ENUM(Command)

    enum Status {
        StatusSuccess = 0x00,
        StatusInvalidProtocol = 0x01,
        StatusInvalidCommand = 0x02,
        StatusInvalidPlayload = 0x03,
        StatusUnknownError = 0xff
    };
    Q_ENUM(Status)

    enum Mode {
        ModeDirect = 0x00,
        ModeFade = 0x01
    };
    Q_ENUM(Mode)

    explicit NymeaLightInterface(QObject *parent = nullptr);

    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool available() = 0;

    virtual void sendData(const QByteArray &data) = 0;

signals:
    void availableChanged(bool available);
    void dataReceived(const QByteArray &data);

};

class NymeaLightInterfaceReply : public QObject
{
    Q_OBJECT

    friend class NymeaLight;

public:
    QByteArray requestData() const { return m_requestData; };
    NymeaLightInterface::Command command() const { return m_command; };
    quint8 requestId() const { return m_requestId; };
    NymeaLightInterface::Status status() const { return m_status; };

signals:
    void finished();

private:
    explicit NymeaLightInterfaceReply(const QByteArray &requestData, QObject *parent = nullptr) : QObject(parent), m_requestData(requestData) {
        Q_ASSERT(m_requestData.length() >= 2);
        m_command = static_cast<NymeaLightInterface::Command>(m_requestData.at(0));
        m_requestId = static_cast<quint8>(m_requestData.at(1));
    }

    QByteArray m_requestData;
    NymeaLightInterface::Command m_command;
    quint8 m_requestId;
    NymeaLightInterface::Status m_status = NymeaLightInterface::StatusUnknownError;
};

#endif // NYMEALIGHTINTERFACE_H
