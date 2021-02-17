/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef TEMPO_H
#define TEMPO_H

#include <QObject>
#include <QUrl>
#include <QTimer>

#include "network/networkaccessmanager.h"

class Tempo : public QObject
{
    Q_OBJECT
public:

    enum Status {
        Open,
        Closed,
        Archived
    };
    Q_ENUM(Status)
    struct Lead {
        QUrl self;
        QString accountId;
        QString displayName;
    };

    struct Contact {
        QUrl self;
        QString accountId;
        QString displayName;
        QString type;
    };

    struct Category {
       QUrl self;
       QString accountId;
       QString displayName;
    };

    struct Customer {
        QUrl self;
        QString key;
        int id;
        QString name;
    };

    struct Account {
        QUrl self;
        QString key;
        int id;
        QString name;
        Status status;
        bool global;
        int monthlyBudget;
        Lead lead;
        Contact contact;
        Category category;
        Customer customer;
    };

    explicit Tempo(NetworkAccessManager *networkmanager, const QByteArray &clientId, const QByteArray &clientSecret, QObject *parent = nullptr);
    QByteArray accessToken();
    QByteArray refreshToken();

    static QUrl getLoginUrl(const QUrl &redirectUrl, const QString &jiraCloudInstanceName, const QByteArray &clientId);

    void getAccessTokenFromRefreshToken(const QByteArray &refreshToken);
    void getAccessTokenFromAuthorizationCode(const QByteArray &authorizationCode);

    void getAccounts();
    void getWorkloadByAccount(const QString &accountKey, QDate from, QDate to);

private:

    QByteArray m_baseTokenUrl = "https://api.tempo.io/oauth/token/";
    QByteArray m_baseControlUrl = "https://api.tempo.io/core/3/";
    QByteArray m_clientId;
    QByteArray m_clientSecret;

    QByteArray m_accessToken;
    QByteArray m_refreshToken;
    QByteArray m_redirectUri  = "https://127.0.0.1:8888";

    NetworkAccessManager *m_networkManager = nullptr;
    QTimer *m_tokenRefreshTimer = nullptr;

    void setAuthenticated(bool state);
    void setConnected(bool state);

    bool m_authenticated = false;
    bool m_connected = false;

    bool checkStatusCode(QNetworkReply *reply, const QByteArray &rawData);
private slots:
    void onRefreshTimer();

signals:
    void authenticationStatusChanged(bool state);
    void connectionChanged(bool connected);
    void receivedRefreshToken(const QByteArray &refreshToken);
    void receivedAccessToken(const QByteArray &accessToken);
    void accountsReceived(const QList<Account> accounts);
};

#endif // TEMPO_H
