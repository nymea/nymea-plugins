#ifndef OAUTH_H
#define OAUTH_H


#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class OAuth : public QObject
{
    Q_OBJECT

public:
    explicit OAuth(QString clientId, QObject *parent = nullptr);

    QUrl url() const;
    void setUrl(const QUrl &url);

    QUrlQuery query() const;
    void setQuery(const QUrlQuery &query);

    QString clientId() const;
    void setClientId(const QString &clientId);

    QString scope() const;
    void setScope(const QString &scope);

    QByteArray authorizationCode() const;
    QByteArray bearerToken() const;

    bool authenticated() const;

    void startAuthentication();

private:

    QNetworkAccessManager *m_networkManager;
    QTimer *m_timer;
    QList<QNetworkReply *> m_tokenRequests;
    QList<QNetworkReply *> m_refreshTokenRequests;

    QUrl m_url;
    QUrlQuery m_query;
    QString m_clientId;
    QString m_scope;
    QString m_state;
    QString m_redirectUri;
    QString m_responseType;

    QString m_token;
    QString m_refreshToken;

    bool m_authenticated;

    void setAuthenticated(const bool &authenticated);
    void setToken(const QString &token);

private slots:
    void replyFinished(QNetworkReply *reply);
    void refreshTimeout();

signals:
    void authenticationChanged();
    void tokenChanged();
};

#endif // OAUTH_H
