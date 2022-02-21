#include "owletserialclient.h"
#include "extern-plugininfo.h"

#include <QDataStream>

OwletSerialClient::OwletSerialClient(OwletTransport *transport, QObject *parent) :
    QObject(parent),
    m_transport(transport)
{
    connect(m_transport, &OwletTransport::dataReceived, this, &OwletSerialClient::dataReceived);
    connect(m_transport, &OwletTransport::error, this, &OwletSerialClient::error);
    connect(m_transport, &OwletTransport::connectedChanged, this, [=](bool transportConnected){
        if (!transportConnected) {
            m_ready = false;
            emit disconnected();

            // Clean up queue
            qDeleteAll(m_pendingRequests);
            m_pendingRequests.clear();
        } else {
            // Wait for the ready notification
        }
    });
}

OwletSerialClient::~OwletSerialClient()
{
    qCDebug(dcOwlet()) << "Destroy owlet serial client";
    transport()->disconnectTransport();
}

OwletTransport *OwletSerialClient::transport() const
{
    return m_transport;
}

bool OwletSerialClient::isConnected() const
{
    return m_transport->connected();
}

bool OwletSerialClient::isReady() const
{
    return m_ready;
}

OwletSerialClientReply *OwletSerialClient::getFirmwareVersion()
{
    qCDebug(dcOwlet()) << "Request owlet firmware version";
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(OwletSerialClient::CommandGetFirmwareVersion);
    stream << m_requestId++;

    OwletSerialClientReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();
    return reply;
}

OwletSerialClientReply *OwletSerialClient::configurePin(quint8 pinId, PinMode pinMode)
{
    qCDebug(dcOwlet()) << "Configure pin" << pinId << pinMode;
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(OwletSerialClient::CommandConfigurePin);
    stream << m_requestId++;
    stream << pinId << static_cast<quint8>(pinMode);

    OwletSerialClientReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();
    return reply;
}

OwletSerialClientReply *OwletSerialClient::writeDigitalValue(quint8 pinId, bool power)
{
    qCDebug(dcOwlet()) << "Setting gpio output power of pin" << pinId << power;
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(OwletSerialClient::CommandWriteDigitalPin);
    stream << m_requestId++;
    stream << pinId << static_cast<quint8>(power ? 0x01 : 0x00);

    OwletSerialClientReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();
    return reply;
}

OwletSerialClientReply *OwletSerialClient::readDigitalValue(quint8 pinId)
{
    qCDebug(dcOwlet()) << "Reading digital gpio value of pin" << pinId;
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(OwletSerialClient::CommandReadDigitalPin);
    stream << m_requestId++;
    stream << pinId;

    OwletSerialClientReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();
    return reply;
}

OwletSerialClientReply *OwletSerialClient::writeAnalogValue(quint8 pinId, quint8 dutyCycle)
{
    qCDebug(dcOwlet()) << "Write analog gpio value of pin" << pinId << dutyCycle;
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(OwletSerialClient::CommandWriteAnalogPin);
    stream << m_requestId++;
    stream << pinId;
    stream << dutyCycle;

    OwletSerialClientReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();
    return reply;
}

OwletSerialClientReply *OwletSerialClient::readAnalogValue(quint8 pinId)
{
    qCDebug(dcOwlet()) << "Reading analog gpio value of pin" << pinId;
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(OwletSerialClient::CommandReadAnalogPin);
    stream << m_requestId++;
    stream << pinId;

    OwletSerialClientReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();
    return reply;
}

OwletSerialClientReply *OwletSerialClient::writeServoValue(quint8 pinId, quint8 angle)
{
    qCDebug(dcOwlet()) << "Setting servo angle of pin" << pinId << angle;
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(OwletSerialClient::CommandWriteServoPin);
    stream << m_requestId++;
    stream << pinId;
    stream << angle;

    OwletSerialClientReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();
    return reply;
}

QString OwletSerialClient::firmwareVersion() const
{
    return m_firmwareVersion;
}

void OwletSerialClient::dataReceived(const QByteArray &data)
{
    QDataStream stream(data);
    quint8 commandId; quint8 requestId;
    stream >> commandId >> requestId;

    // Check if command or notification
    if (commandId < 0xF0) {
        quint8 statusId;
        stream >> statusId;
        OwletSerialClient::Command command = static_cast<OwletSerialClient::Command>(commandId);
        OwletSerialClient::Status status = static_cast<OwletSerialClient::Status>(statusId);
        if (m_currentReply) {
            if (m_currentReply->command() == command && m_currentReply->requestId() == requestId) {
                m_currentReply->m_timer.stop();
                m_currentReply->m_status = status;
                m_currentReply->m_responsePayload = data.right(data.length() - 3);

                if (status != OwletSerialClient::StatusSuccess) {
                    qCWarning(dcOwlet()) << "Request finished with error" << command << "ID:" << m_currentReply->requestId() << status;
                } else {
                    qCDebug(dcOwlet()) << "Request finished" << command << "ID:" << m_currentReply->requestId() << status << "Payload:" << m_currentReply->responsePayload().toHex();
                }

                emit m_currentReply->finished();
            }
        } else {
            qCWarning(dcOwlet()) << "Received unhandled command response data" << data.toHex();
        }
    } else {
        OwletSerialClient::Notification notification = static_cast<OwletSerialClient::Notification>(commandId);
        QByteArray notificationPayload = data.right(data.length() - 2);
        qCDebug(dcOwlet()) << "Notification received" << notification << "ID:" << requestId << notificationPayload.toHex();
        switch (notification) {
        case OwletSerialClient::NotificationReady: {
            OwletSerialClientReply *reply = getFirmwareVersion();
            connect(reply, &OwletSerialClientReply::finished, this, [this, reply](){
                if (reply->status() != OwletSerialClient::StatusSuccess) {
                    qCWarning(dcOwlet()) << "Failed to get firmware version" << reply->status();
                    emit error();
                    return;
                }

                if (reply->responsePayload().count() != 3) {
                    qCWarning(dcOwlet()) << "Invalid firmware version payload size";
                    emit error();
                    return;
                }

                quint8 major = reply->responsePayload().at(0);
                quint8 minor = reply->responsePayload().at(1);
                quint8 patch = reply->responsePayload().at(2);

                m_firmwareVersion = QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
                qCDebug(dcOwlet()) << "Connected successfully to firmware" << m_firmwareVersion;
                m_ready = true;
                emit readyChanged(m_ready);
                emit connected();
            });
            break;
        }
        case OwletSerialClient::NotificationGpioPinChanged: {
            quint8 pinId; quint8 powerValue;
            stream >> pinId >> powerValue;
            bool power = powerValue != 0x00;
            qCDebug(dcOwlet()) << "Pin value changed" << pinId << power;
            emit pinValueChanged(pinId, power);
            break;
        }
        case OwletSerialClient::NotificationDebugMessage:
            qCDebug(dcOwlet()) << "Firmware debug:" << QString::fromUtf8(data.right(data.length() - 2));
            break;
        default:
            qCWarning(dcOwlet()) << "Unhandled notification received" << data.toHex();
            break;
        }
    }
}

OwletSerialClientReply *OwletSerialClient::createReply(const QByteArray &requestData)
{
    OwletSerialClientReply *reply = new OwletSerialClientReply(requestData, this);
    connect(reply, &OwletSerialClientReply::finished, reply, [=](){
        reply->deleteLater();
        if (reply == m_currentReply) {
            m_currentReply = nullptr;
        }
        sendNextRequest();
    });

    return reply;
}

void OwletSerialClient::sendNextRequest()
{
    if (m_currentReply)
        return;

    if (m_pendingRequests.isEmpty())
        return;

    m_currentReply = m_pendingRequests.dequeue();
    qCDebug(dcOwlet()) << "Sending request" << m_currentReply->command() << "ID:" << m_currentReply->requestId() << "Payload:" << m_currentReply->requestData().right(m_currentReply->requestData().length() - 2).toHex();
    m_transport->sendData(m_currentReply->requestData());
    m_currentReply->m_timer.start();
}
