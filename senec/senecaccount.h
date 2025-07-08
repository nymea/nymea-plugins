/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#ifndef SENECACCOUNT_H
#define SENECACCOUNT_H

#include <QUrl>
#include <QObject>
#include <QNetworkReply>

#include <network/networkaccessmanager.h>

class SenecAccount : public QObject
{
    Q_OBJECT
public:
    explicit SenecAccount(NetworkAccessManager *networkManager, const QString &username, const QString &token, const QString &refreshToken, QObject *parent = nullptr);

    static QUrl baseUrl();
    static QUrl loginUrl();
    static QUrl systemsUrl();

    bool available() const;

    QNetworkReply *getSystems();
    QNetworkReply *getDashboard(const QString &id);
    QNetworkReply *getAbilities(const QString &id);
    QNetworkReply *getTechnicalData(const QString &id);

signals:
    bool availableChanged(bool available);

private:
    NetworkAccessManager *m_networkManager = nullptr;

    QString m_username;
    QString m_token;
    QString m_refreshToken;

    bool m_available = false;
};

#endif // SENECACCOUNT_H
