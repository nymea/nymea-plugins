#include "oauth.h"
#include "extern-plugininfo.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QUuid>

OAuth::OAuth(QString clientId, QObject *parent) :
    QObject(parent),
    m_clientId(clientId),
    m_authenticated(false)
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &OAuth::replyFinished);

    m_timer = new QTimer(this);
    m_timer->setSingleShot(false);

    connect(m_timer, &QTimer::timeout, this, &OAuth::refreshTimeout);
}

QUrl OAuth::url() const
{
    return m_url;
}

void OAuth::setUrl(const QUrl &url)
{
    m_url = url;
}

QUrlQuery OAuth::query() const
{
    return m_query;
}

void OAuth::setQuery(const QUrlQuery &query)
{
    m_query = query;
}

QString OAuth::clientId() const
{
    return m_clientId;
}

void OAuth::setClientId(const QString &clientId)
{
    m_clientId = clientId;
}

QString OAuth::scope() const
{
    return m_scope;
}

void OAuth::setScope(const QString &scope)
{
    m_scope = scope;
}

QString OAuth::authorizationCode() const
{
    return m_token;
}

QString OAuth::bearerToken() const
{
    return m_token;
}

bool OAuth::authenticated() const
{
    return m_authenticated;
}

void OAuth::startAuthentication()
{
    qCDebug(dcSonos) << "Start authentication";

    QUrlQuery query;
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("redirect_uri", m_redirectUri);
    query.addQueryItem("response_type", "code");
    query.addQueryItem("scope", m_scope);
    m_state = QUuid().toByteArray();
    query.addQueryItem("state", m_state);
    setQuery(query);

    QNetworkRequest request(m_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    m_tokenRequests.append(m_networkManager->post(request, m_query.toString().toUtf8()));
}

void OAuth::setAuthenticated(const bool &authenticated)
{
    if (authenticated) {
        qCDebug(dcSonos()) << "Authenticated successfully";
    } else {
        m_timer->stop();
        qCWarning(dcSonos) << "Authentication failed";
    }
    m_authenticated = authenticated;
    emit authenticationChanged();
}

void OAuth::setToken(const QString &token)
{
    m_token = token;
    emit tokenChanged();
}

void OAuth::replyFinished(QNetworkReply *reply)
{
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();
    // token request
    if (m_tokenRequests.contains(reply)) {

        QByteArray data = reply->readAll();
        m_tokenRequests.removeAll(reply);

        // check HTTP status code
        if (status != 200) {
            qCWarning(dcSonos) << "Request token reply HTTP error:" << status << reply->errorString();
            qCWarning(dcSonos) << data;
            setAuthenticated(false);
            return;
        }

        // check JSON
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcSonos) << "Request token reply JSON error:" << error.errorString();
            setAuthenticated(false);
            return;
        }

        if (!jsonDoc.toVariant().toMap().contains("code")) {
            qCWarning(dcSonos) << "Could not get code" << jsonDoc.toJson();
            setAuthenticated(false);
            return;
        }

        if (!jsonDoc.toVariant().toMap().contains("state")) {
            qCWarning(dcSonos) << "Could not get state" << jsonDoc.toJson();
            return;
        }

        if (jsonDoc.toVariant().toMap().value("state").toString() != m_state) {
            qCWarning(dcSonos) << "State doesn't match. Expected:" << m_state << "Received:" << jsonDoc.toVariant().toMap().value("state").toString();
        }

        setToken(jsonDoc.toVariant().toMap().value("code").toString());
        setAuthenticated(true);

        if (jsonDoc.toVariant().toMap().contains("expires_in") && jsonDoc.toVariant().toMap().contains("refresh_token")) {
            int expireTime = jsonDoc.toVariant().toMap().value("expires_in").toInt();
            m_refreshToken = jsonDoc.toVariant().toMap().value("refresh_token").toString();
            qCDebug(dcSonos) << "Token will be refreshed in" << expireTime << "[s]";
            m_timer->start((expireTime - 20) * 1000);
        }

    } else if (m_refreshTokenRequests.contains(reply)) {

        QByteArray data = reply->readAll();
        m_refreshTokenRequests.removeAll(reply);

        // check HTTP status code
        if (status != 200) {
            qCWarning(dcSonos) << "Refresh token reply HTTP error:" << status << reply->errorString();
            qCWarning(dcSonos) << data;
            setAuthenticated(false);
            return;
        }

        // check JSON
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcSonos) << "Refresh token reply JSON error:" << error.errorString();
            setAuthenticated(false);
            return;
        }

        if (!jsonDoc.toVariant().toMap().contains("access_token")) {
            qCWarning(dcSonos) << "Could not get access token after refresh" << jsonDoc.toJson();
            setAuthenticated(false);
            return;
        }


        setToken(jsonDoc.toVariant().toMap().value("access_token").toString());
        qCDebug(dcSonos) << "Token refreshed successfully";

        if (jsonDoc.toVariant().toMap().contains("expires_in") && jsonDoc.toVariant().toMap().contains("refresh_token")) {
            int expireTime = jsonDoc.toVariant().toMap().value("expires_in").toInt();
            m_refreshToken = jsonDoc.toVariant().toMap().value("refresh_token").toString();
            qCDebug(dcSonos) << "Token will be refreshed in" << expireTime << "[s]";
            m_timer->start((expireTime - 20) * 1000);
        }

        if (!authenticated())
            setAuthenticated(true);
    }
}

void OAuth::refreshTimeout()
{
    qCDebug(dcSonos) << "Refresh authentication token for";

    QUrlQuery query;
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", m_refreshToken);
    query.addQueryItem("client_id", m_clientId);

    QNetworkRequest request(m_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    m_refreshTokenRequests.append(m_networkManager->post(request, query.toString().toUtf8()));
}
