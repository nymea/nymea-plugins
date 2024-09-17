#include "nymealight.h"
#include "extern-plugininfo.h"

#include <QDataStream>

NymeaLight::NymeaLight(NymeaLightInterface *interface, QObject *parent) :
    QObject(parent),
    m_interface(interface)
{
    connect(m_interface, &NymeaLightInterface::dataReceived, this, &NymeaLight::onDataReceived);
    connect(m_interface, &NymeaLightInterface::availableChanged, this, [=](bool available){
        m_interfaceAvailable = available;
        if (m_interfaceAvailable) {
            qCDebug(dcWs2812fx()) << "Nymea light interface is now available. Start polling status of the light controller...";
            m_pollStatusRetryCount = 0;
            pollStatus();
        } else {
            m_ready = false;
            m_requestId = 0;
            emit availableChanged(false);
        }
    });

}

NymeaLightInterfaceReply *NymeaLight::setPower(bool power, quint16 fadeDuration)
{
    // Build the request
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(NymeaLightInterface::CommandSetPower);
    stream << m_requestId++;
    stream << fadeDuration;
    stream << static_cast<quint8>(power ? 0x01 : 0x00);

    NymeaLightInterfaceReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();
    return reply;
}

NymeaLightInterfaceReply *NymeaLight::setColor(const QColor &color, quint16 fadeDuration)
{
    // Build the request
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(NymeaLightInterface::CommandSetColor);
    stream << m_requestId++;
    stream << fadeDuration;
    stream << static_cast<quint8>(color.red());
    stream << static_cast<quint8>(color.green());
    stream << static_cast<quint8>(color.blue());

    NymeaLightInterfaceReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();
    return reply;
}

NymeaLightInterfaceReply *NymeaLight::setBrightness(quint8 brightness, quint16 fadeDuration)
{
    // Build the request
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(NymeaLightInterface::CommandSetBrightness);
    stream << m_requestId++;
    stream << fadeDuration;
    stream << static_cast<quint8>(brightness);

    NymeaLightInterfaceReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();
    return reply;
}

NymeaLightInterfaceReply *NymeaLight::setSpeed(quint16 speed, quint16 fadeDuration)
{
    // Build the request
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(NymeaLightInterface::CommandSetSpeed);
    stream << m_requestId++;
    stream << fadeDuration;
    stream << speed;

    NymeaLightInterfaceReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();
    return reply;
}

NymeaLightInterfaceReply *NymeaLight::setEffect(quint8 effect)
{
    // Build the request
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(NymeaLightInterface::CommandSetEffect);
    stream << m_requestId++;
    stream << effect;

    NymeaLightInterfaceReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();
    return reply;
}

bool NymeaLight::available() const
{
    return m_interfaceAvailable && m_ready;
}

void NymeaLight::enable()
{
    m_interface->open();
    qCDebug(dcWs2812fx()) << "Nymea light enabled";
}

void NymeaLight::disable()
{
    m_interface->close();
    qCDebug(dcWs2812fx()) << "Nymea light disabled";
}

NymeaLightInterfaceReply *NymeaLight::createReply(const QByteArray &requestData)
{
    NymeaLightInterfaceReply *reply = new NymeaLightInterfaceReply(requestData, this);
    connect(reply, &NymeaLightInterfaceReply::finished, reply, [=](){
        reply->deleteLater();
        if (reply == m_currentReply) {
            m_currentReply = nullptr;
        }
        sendNextRequest();
    });
    return reply;
}

void NymeaLight::sendNextRequest()
{
    if (m_currentReply)
        return;

    if (m_pendingRequests.isEmpty())
        return;

    // TODO: if not available, finish all replies with unknown error

    m_currentReply = m_pendingRequests.dequeue();
    qCDebug(dcWs2812fx()) << "Sending request" << m_currentReply->command() << m_currentReply->requestId() << m_currentReply->requestData().toHex();
    m_interface->sendData(m_currentReply->requestData());
    m_currentReply->m_timer->start();
}

void NymeaLight::pollStatus()
{
    // Request ready state from controller
    NymeaLightInterfaceReply *reply = getStatus();
    connect(reply, &NymeaLightInterfaceReply::finished, this, [=](){
        if (reply->status() == NymeaLightInterface::StatusSuccess) {
            qCDebug(dcWs2812fx()) << "Get status request finished successfully. The firmware is ready to operate.";
            m_ready = true;
            m_pollStatusRetryCount = 0;
            emit availableChanged(true);
        } else {
            m_pollStatusRetryCount++;
            if (m_pollStatusRetryCount >= m_pollStatusRetryLimit) {
                qCWarning(dcWs2812fx()) << "Firmware did not respond to get status request after" << m_pollStatusRetryCount << "attempts. Giving up.";
                m_ready = false;
            } else {
                if (!m_ready && m_interfaceAvailable) {
                    qCDebug(dcWs2812fx()) << "Get status request finished with error" << reply->status() << "Retry" << m_pollStatusRetryCount << "/" << m_pollStatusRetryLimit;
                    pollStatus();
                } else {
                    qCDebug(dcWs2812fx()) << "Get status request finished with error, but that's ok since we received the ready notification." << reply->status();
                }
            }
        }
    });
}

NymeaLightInterfaceReply *NymeaLight::getStatus()
{
    qCDebug(dcWs2812fx()) << "Request status of nymea light";
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(NymeaLightInterface::CommandGetStatus);
    stream << m_requestId++;

    NymeaLightInterfaceReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();
    return reply;
}

void NymeaLight::onDataReceived(const QByteArray &data)
{
    Q_ASSERT(data.length() >= 3);

    quint8 commandInt = static_cast<quint8>((data.at(0)));
    quint8 requestId = static_cast<quint8>(data.at(1));

    qCDebug(dcWs2812fx()) << "Recived data" << commandInt << requestId << data.toHex();

    // Check if command or notification
    if (commandInt < 0xF0) {
        NymeaLightInterface::Command command = static_cast<NymeaLightInterface::Command>(commandInt);

        NymeaLightInterface::Status status = static_cast<NymeaLightInterface::Status>(data.at(2));

        if (m_currentReply) {
            if (m_currentReply->command() == command && m_currentReply->requestId() == requestId) {
                m_currentReply->m_timer->stop();
                m_currentReply->m_status = status;

                if (status != NymeaLightInterface::StatusSuccess) {
                    qCWarning(dcWs2812fx()) << "Request finished with error" << command << m_currentReply->requestId() << status;
                } else {
                    qCDebug(dcWs2812fx()) << "Request finished" << command << m_currentReply->requestId() << status;
                }

                emit m_currentReply->finished();
            }
        } else {
            qCWarning(dcWs2812fx()) << "Received unhandled command response data" << data.toHex();
        }
    } else {
        NymeaLightInterface::Notification notification = static_cast<NymeaLightInterface::Notification>(commandInt);
        switch (notification) {
        case NymeaLightInterface::NotificationReady:
            qCDebug(dcWs2812fx()) << "Controller ready notification received";
            m_ready = true;
            emit availableChanged(true);
            break;
        case NymeaLightInterface::NotificationDebugMessage:
            qCDebug(dcWs2812fx()) << "Firmware debug:" << QString::fromUtf8(data.right(data.length() - 2));
            break;
        default:
            qCWarning(dcWs2812fx()) << "Unhandled notification received" << data.toHex();
            break;
        }
    }

}

