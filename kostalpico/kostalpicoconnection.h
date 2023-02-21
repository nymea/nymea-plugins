#ifndef KOSTALPICOCONNECTION_H
#define KOSTALPICOCONNECTION_H

#include <QObject>

#include <QQueue>
#include <QHostAddress>

#include <network/networkaccessmanager.h>

#include "kostalnetworkreply.h"

class KostalPicoConnection : public QObject
{
    Q_OBJECT
public:
    explicit KostalPicoConnection(NetworkAccessManager *networkManager, const QHostAddress &address, QObject *parent = nullptr);

    QHostAddress address() const;

    bool available() const;

    bool busy() const;
    //Has Consumption "GridConsumedPower"
    KostalNetworkReply *getMeasurement();

    KostalNetworkReply *getActiveDevices();
    //Has Production
    //Yields->Yield-> Type Produced -> In Watthours
    KostalNetworkReply *getYields();
    //TODO
    /*
    //yields -> look for Type -> If Inverter -> yes
    KostalNetworkReply *getType();
    KostalNetworkReply *getVersion();


    */
signals:
   void availableChanged(bool available);

private:

   NetworkAccessManager *m_networkManager = nullptr;
   QHostAddress m_address;

   bool m_available = false;

   // Request queue to prevent overloading the device with requests
   KostalNetworkReply *m_currentReply = nullptr;
   QQueue<KostalNetworkReply *> m_requestQueue;

   void sendNextRequest();

};

#endif // KOSTALPICOCONNECTION_H
