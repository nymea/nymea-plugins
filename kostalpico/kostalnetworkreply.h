#ifndef KOSTALNETWORKREPLY_H
#define KOSTALNETWORKREPLY_H

#include <QNetworkReply>
#include <QObject>

class KostalNetworkReply : public QObject {
  Q_OBJECT

  friend class KostalPicoConnection;

public:

  ~KostalNetworkReply();
  QUrl requestUrl() const;

  QNetworkRequest request() const;
  QNetworkReply *networkReply() const;

signals:
  void finished();


private:
  explicit KostalNetworkReply(const QNetworkRequest &request, QObject *parent = nullptr);

  QNetworkRequest m_request;
  QNetworkReply *m_networkReply = nullptr;

  void setNetworkReply(QNetworkReply *networkReply);

};



#endif // KOSTALNETWORKREPLY_H
