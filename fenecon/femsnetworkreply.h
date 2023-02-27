#ifndef FEMSNETWORKREPLY_H
#define FEMSNETWORKREPLY_H

#include <QNetworkReply>
#include <QObject>

class FemsNetworkReply : public QObject {
  Q_OBJECT

  friend class FemsConnection;

public:
  ~FemsNetworkReply();

  QUrl requestUrl() const;

  QNetworkRequest request() const;
  QNetworkReply *networkReply() const;

signals:
  void finished();

private:
  explicit FemsNetworkReply(const QNetworkRequest &request,
                            QObject *parent = nullptr, QString usr = "",
                            QString pwd = "");

  QNetworkRequest m_request;
  QNetworkReply *m_networkReply = nullptr;

  void setNetworkReply(QNetworkReply *networkReply);
};

#endif // FEMSNETWORKREPLY_H
