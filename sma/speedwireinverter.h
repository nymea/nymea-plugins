#ifndef SPEEDWIREINVERTER_H
#define SPEEDWIREINVERTER_H

#include <QObject>

#include "speedwireinterface.h"

class SpeedwireInverter : public QObject
{
    Q_OBJECT
public:
    enum Command {
        CommandQueryAc = 0x51000200,
        CommandQueryStatus = 0x51800200,
        CommandQueryDevice = 0x58000200,
        CommandQueryDc = 0x53800200,
        CommandQueryLogin = 0xfffd040c
    };


    Q_ENUM(Command)


    class Request
    {
    public:
        Request();

        SpeedwireInverter::Command command() const;

        quint16 requestId() const;

    private:
        SpeedwireInverter::Command m_command;
        quint16 m_requestId = 0;

    };


    explicit SpeedwireInverter(const QHostAddress &address, quint16 modelId, quint32 serialNumber, QObject *parent = nullptr);

    bool initialize();
    bool initialized() const;

    double currentPower() const;
    double totalEnergyProduced() const;

    // Query methods
    void sendLoginRequest(const QString &password = "0000", bool loginAsUser = true);
    void querySoftwareVersion();
    void queryDeviceType();

signals:
    void valuesUpdated();

private:
    SpeedwireInterface *m_interface = nullptr;
    QHostAddress m_address;
    bool m_initialized = false;
    quint16 m_modelId = 0;
    quint32 m_serialNumber = 0;
    quint16 m_packetId = 0;

    // Properties
    double m_currentPower = 0;
    double m_totalEnergyProduced = 0;

    QString m_softwareVersion;

private slots:
    void processData(const QByteArray &data);

};

#endif // SPEEDWIREINVERTER_H
