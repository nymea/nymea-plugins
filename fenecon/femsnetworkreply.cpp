#include "femsnetworkreply.h"

FemsNetworkReply::~FemsNetworkReply() {
  if (m_networkReply) {
    // We don't need the finished signal any more, object gets deleted
    disconnect(m_networkReply, &QNetworkReply::finished, this,
               &FemsNetworkReply::finished);

    if (m_networkReply->isRunning()) {
      // Abort the reply, we are not interested in it any more
      m_networkReply->abort();
    }

    m_networkReply->deleteLater();
  }
}

QUrl FemsNetworkReply::requestUrl() const { return m_request.url(); }

QNetworkRequest FemsNetworkReply::request() const { return m_request; }

QNetworkReply *FemsNetworkReply::networkReply() const { return m_networkReply; }

FemsNetworkReply::FemsNetworkReply(const QNetworkRequest &request,
                                   QObject *parent, QString usr, QString pwd, bool useEdge)
    : QObject(parent) {
  m_request = request;
  //qInfo() << useEdge;
  if (useEdge) {

      //qInfo() <<    "Basic " + QString("%1:%2").arg(usr).arg(pwd).toUtf8().toBase64();

    this->m_request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(usr).arg(pwd).toUtf8().toBase64());
  }
}

void FemsNetworkReply::setNetworkReply(QNetworkReply *networkReply) {
  m_networkReply = networkReply;
  // The QNetworkReply will be deleted in the destructor if set
  connect(m_networkReply, &QNetworkReply::finished, this,
          &FemsNetworkReply::finished);
}
