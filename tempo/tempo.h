// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
       QString key;
       int id;
       QString name;
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
      QDate startDate;
      QTime startTime;
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

    explicit Tempo(NetworkAccessManager *networkmanager, const QString &token, QObject *parent = nullptr);
    ~Tempo() override;
    QString token() const;

    void getTeams();
    void getAccounts();
    void getWorkloadByAccount(const QString &accountKey, QDate from, QDate to, int offset = 0, int limit = 50);
    void getWorkloadByTeam(int teamId, QDate from, QDate to, int offset = 0, int limit = 50);

private:
    QByteArray m_baseControlUrl = "https://api.tempo.io/core/3";
    QString m_token;

    NetworkAccessManager *m_networkManager = nullptr;

    void setAuthenticated(bool state);
    void setConnected(bool state);

    bool m_authenticated = false;
    bool m_connected = false;

    QList<Worklog> parseJsonForWorklog(const QVariantMap &data);
    bool checkStatusCode(QNetworkReply *reply, const QByteArray &rawData);

private slots:

signals:
    void authenticationStatusChanged(bool state);
    void connectionChanged(bool connected);

    void teamsReceived(const QList<Team> teams);
    void accountsReceived(const QList<Account> accounts);
    void accountWorklogsReceived(const QString &accountKey, QList<Worklog> worklogs, int limit, int offset);
    void teamWorklogsReceived(int teamId, QList<Worklog> worklogs, int limit, int offset);
};

#endif // TEMPO_H
