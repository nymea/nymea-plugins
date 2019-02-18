#ifndef KEBACONNECTION_H
#define KEBACONNECTION_H

#include <QTimer>
#include <QObject>
#include <QHostAddress>
#include <QByteArray>

class KebaConnection : public QObject
{
    Q_OBJECT
public:
    explicit KebaConnection(QHostAddress address, QObject *parent = 0);
    void onAnswerReceived();

    QHostAddress getAddress();
    void setAddress(QHostAddress address);
    bool getDeviceConnectedStatus();
    bool getDeviceBlockedStatus();

    void enableOutput(bool state);
    void setMaxAmpere(int milliAmpere);
    void getDeviceInformation();
    void getReport1();
    void getReport2();
    void getReport3();
    void unlockCharger();
    void displayMessage(const QByteArray &message);

private:
    QHostAddress m_address;
    QByteArrayList m_commandList;
    bool m_deviceBlocked = false;
    bool m_connected;
    QTimer *m_timeoutTimer = nullptr;

    void sendCommand(const QByteArray &data);
signals:
    void connectionChanged(bool status);
    void sendData(const QByteArray &data);

private slots:
    void onTimeout();
};

#endif // KEBACONNECTION_H
