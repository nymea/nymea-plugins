#ifndef FEMSCONNECTION_H
#define FEMSCONNECTION_H

#include <QObject>

#include <QHostAddress>
#include <QQueue>

#include <network/networkaccessmanager.h>

#include "femsnetworkreply.h"

class FemsConnection : public QObject {
  Q_OBJECT

private:
  QString m_user;
  QString m_password;
  QString m_port;
  QString baseUrl;
  QHostAddress m_address;
  NetworkAccessManager *m_networkManager = nullptr;

  bool batteryAvailable = false;
  bool meterAvailable = false;
  bool m_available = false;

  FemsNetworkReply *m_currentReply = nullptr;
  QQueue<FemsNetworkReply *> m_request_Queue;
  FemsNetworkReply *getFemsReply(QString path);

  void sendNextRequest();

public:
  explicit FemsConnection(NetworkAccessManager *networkManager,
                          const QHostAddress &address,
                          QObject *parent = nullptr, QString user = "",
                          QString pwd = "",
                          QString port = "8084");

  QHostAddress address() const;

  bool available() const;

  bool busy() const;

  FemsNetworkReply *isAvailable();
  // Has to be called by integrationsPlugin for each datapoint that you want to
  // receive
  FemsNetworkReply *getFemsDataPoint(QString componentAndChannelId);

  enum StateOfFems { OK = 0, INFO = 1, WARNING = 2, FAULT = 3 };

signals:

  void availableChanged(bool available);
};

#endif // FEMSCONNECTION_H
