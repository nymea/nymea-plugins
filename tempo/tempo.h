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

    struct Worklog {
      QUrl self;
      int tempoWorklogId;
      int jiraWorklogId;
      QString issue;
      int timeSpentSeconds;
      QDateTime startedAt;
      QString description;
      QDateTime createdAt;
      QDateTime updatedAt;
      QString authorAccountId;
      QString authorDisplayName;
    };

    struct Team {
        QUrl self;
        int id;
        QString name;
        QString summary;
        Lead lead;
    };

    explicit Tempo(NetworkAccessManager *networkmanager, const QString &jiraCloudInstanceName, const QString &token, QObject *parent = nullptr);
    ~Tempo() override;
    QString token() const;

    void getTeams();
    void getAccounts();
    void getWorkloadByAccount(const QString &accountKey, QDate from, QDate to);

private:
    QByteArray m_baseTokenUrl = "https://api.tempo.io/oauth/token/";
    QByteArray m_baseControlUrl = "https://api.tempo.io/core/3/";
    QString m_token;
    QString m_jiraCloudInstanceName;

    NetworkAccessManager *m_networkManager = nullptr;

    void setAuthenticated(bool state);
    void setConnected(bool state);

    bool m_authenticated = false;
    bool m_connected = false;

    bool checkStatusCode(QNetworkReply *reply, const QByteArray &rawData);

private slots:

signals:
    void authenticationStatusChanged(bool state);
    void connectionChanged(bool connected);

    void teamsReceived(const QList<Team> teams);
    void accountsReceived(const QList<Account> accounts);
    void accountWorklogsReceived(const QString &accountKey, QList<Worklog> worklogs);
};

#endif // TEMPO_H
