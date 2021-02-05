#include "nymealight.h"
#include "extern-plugininfo.h"

#include <QDataStream>

NymeaLight::NymeaLight(NymeaLightInterface *interface, QObject *parent) :
    QObject(parent),
    m_interface(interface)
{
    connect(m_interface, &NymeaLightInterface::availableChanged, this, [=](bool available){
       qCDebug(dcWs2812fx()) << "Interface available changed" << available;
       emit availableChanged(available);
    });

    connect(m_interface, &NymeaLightInterface::dataReceived, this, &NymeaLight::onDataReceived);
}

NymeaLightInterfaceReply *NymeaLight::setColor(const QColor &color, quint16 fadeDuration)
{
    // Build the request
    QByteArray requestData;
    QDataStream stream(&requestData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(NymeaLightInterface::CommandSetColor);
    stream << m_requestId++;
    if (fadeDuration > 0) {
        stream << static_cast<quint8>(NymeaLightInterface::ModeFade);
    }
    stream << static_cast<quint8>(color.red());
    stream << static_cast<quint8>(color.green());
    stream << static_cast<quint8>(color.blue());
    if (fadeDuration > 0) {
        stream << fadeDuration;
    }

    NymeaLightInterfaceReply *reply = createReply(requestData);
    m_pendingRequests.enqueue(reply);
    sendNextRequest();

    return reply;
}

bool NymeaLight::available() const
{
    return m_interface->available();
}

NymeaLightInterfaceReply *NymeaLight::createReply(const QByteArray &requestData)
{
    NymeaLightInterfaceReply *reply = new NymeaLightInterfaceReply(requestData, this);
    connect(reply, &NymeaLightInterfaceReply::finished, reply, &NymeaLightInterfaceReply::deleteLater);
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
}

void NymeaLight::onDataReceived(const QByteArray &data)
{
    qCDebug(dcWs2812fx()) << "Recived data" << data;
    Q_ASSERT(data.length() >= 3);

    NymeaLightInterface::Command command = static_cast<NymeaLightInterface::Command>(data.at(0));
    quint8 requestId = static_cast<quint8>(data.at(1));
    NymeaLightInterface::Status status = static_cast<NymeaLightInterface::Status>(data.at(2));

    if (m_currentReply) {
        if (m_currentReply->command() == command && m_currentReply->requestId() == requestId) {
            m_currentReply->m_status = status;
            qCDebug(dcWs2812fx()) << "Request finished" << command << m_currentReply->requestId() << status;
            emit m_currentReply->finished();

            m_currentReply = nullptr;
            sendNextRequest();
        }
    } else {
        qCWarning(dcWs2812fx()) << "Received unhandled data" << data.toHex();
    }

}

