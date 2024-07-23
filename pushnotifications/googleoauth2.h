/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2024, nymea GmbH
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
