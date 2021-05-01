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
    NymeaLightInterfaceReply *setPower(bool power, quint16 fadeDuration = 0);
    NymeaLightInterfaceReply *setColor(const QColor &color, quint16 fadeDuration = 0);
    NymeaLightInterfaceReply *setBrightness(quint8 brightness, quint16 fadeDuration = 0);
    NymeaLightInterfaceReply *setSpeed(quint16 speed, quint16 fadeDuration = 0);
    NymeaLightInterfaceReply *setEffect(quint8 effect);

    bool available() const;

public slots:
    void enable();
    void disable();

private:
    NymeaLightInterface *m_interface = nullptr;
    quint8 m_requestId = 0;
    bool m_interfaceAvailable = false;
    bool m_ready = false;
    int m_pollStatusRetryCount = 0;
    int m_pollStatusRetryLimit = 5;

    NymeaLightInterfaceReply *m_currentReply = nullptr;
    QQueue<NymeaLightInterfaceReply *> m_pendingRequests;

    NymeaLightInterfaceReply *createReply(const QByteArray &requestData);
    void sendNextRequest();


    void pollStatus();
    NymeaLightInterfaceReply *getStatus();


private slots:
    void onDataReceived(const QByteArray &data);

signals:
    void availableChanged(bool available);

};


#endif // NYMEALIGHT_H
