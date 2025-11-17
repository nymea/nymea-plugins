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

#ifndef GOOGLEOAUTH2_H
#define GOOGLEOAUTH2_H

#include <QUrl>
#include <QObject>
#include <QTimer>

#include <network/apikeys/apikey.h>
#include <network/networkaccessmanager.h>

// https://developers.google.com/identity/protocols/oauth2/service-account

class GoogleOAuth2 : public QObject
{
    Q_OBJECT
public:
    explicit GoogleOAuth2(NetworkAccessManager *networkManager, const ApiKey &apiKey, QObject *parent = nullptr);

    QString accessToken() const;
    bool authenticated() const;

public slots:
    void authorize();

signals:
    void authenticatedChanged(bool authenticated);
    void accessTokenChanged(QString accessToken);

private:
    NetworkAccessManager *m_networkManager;
    ApiKey m_apiKey;

    QTimer m_refreshTimer;
    bool m_authenticated = false;
    QString m_accessToken;

    void setAuthenticated(bool authenticated);

    QByteArray signData(const QByteArray &data, const QByteArray &key);
};

#endif // GOOGLEOAUTH2_H
