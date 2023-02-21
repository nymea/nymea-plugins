#include "kostalnetworkreply.h"

KostalNetworkReply::KostalNetworkReply(const QNetworkRequest &request,
                                       QObject *parent)
    : QObject(parent), m_request(request) {}
KostalNetworkReply::~KostalNetworkReply() {
  if (m_networkReply) {
    // We don't need the finished signal any more, object gets deleted
    disconnect(m_networkReply, &QNetworkReply::finished, this,
               &KostalNetworkReply::finished);

    if (m_networkReply->isRunning()) {
      // Abort the reply, we are not interested in it any more
      m_networkReply->abort();
    }

    m_networkReply->deleteLater();
  }
}
QUrl KostalNetworkReply::requestUrl() const { return m_request.url(); }
QNetworkRequest KostalNetworkReply::request() const { return m_request; }

QNetworkReply *KostalNetworkReply::networkReply() const {
  return m_networkReply;
}


void KostalNetworkReply::setNetworkReply(QNetworkReply *networkReply)
{
    m_networkReply = networkReply;

    // The QNetworkReply will be deleted in the destructor if set
    connect(m_networkReply, &QNetworkReply::finished, this, &KostalNetworkReply::finished);
}
