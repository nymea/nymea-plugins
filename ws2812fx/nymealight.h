#ifndef NYMEALIGHT_H
#define NYMEALIGHT_H

#include <QObject>
#include <QColor>
#include <QQueue>

#include "nymealightinterface.h"

class NymeaLight : public QObject
{
    Q_OBJECT
public:
    explicit NymeaLight(NymeaLightInterface *interface, QObject *parent = nullptr);

    // Set the color. If fade duration is 0, the color will be set immediatly,
    // otherwise it will fade to the color with the given fade duration
    NymeaLightInterfaceReply *setColor(const QColor &color, quint16 fadeDuration = 0);

    bool available() const;

private:
    NymeaLightInterface *m_interface = nullptr;
    quint8 m_requestId = 0;

    NymeaLightInterfaceReply *m_currentReply = nullptr;
    QQueue<NymeaLightInterfaceReply *> m_pendingRequests;

    NymeaLightInterfaceReply *createReply(const QByteArray &requestData);
    void sendNextRequest();

private slots:
    void onDataReceived(const QByteArray &data);

signals:
    void availableChanged(bool available);

};


#endif // NYMEALIGHT_H
