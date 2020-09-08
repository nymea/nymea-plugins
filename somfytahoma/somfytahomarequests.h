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

#ifndef SOMFYTAHOMAREQUESTS_H
#define SOMFYTAHOMAREQUESTS_H

#include <QNetworkReply>

class NetworkAccessManager;

class SomfyTahomaPostRequest : public QObject
{
    Q_OBJECT

public:
    SomfyTahomaPostRequest(NetworkAccessManager *networkManager, const QString &path, const QString &contentType, const QByteArray &body, QObject *parent);

signals:
    void error(QNetworkReply::NetworkError error);
    void finished(const QVariant &results);
};

class SomfyTahomaGetRequest : public QObject
{
    Q_OBJECT

public:
    SomfyTahomaGetRequest(NetworkAccessManager *networkManager, const QString &path, QObject *parent);

signals:
    void error(QNetworkReply::NetworkError error);
    void finished(const QVariant &results);
};

class SomfyTahomaLoginRequest : public SomfyTahomaPostRequest
{
    Q_OBJECT

public:
    SomfyTahomaLoginRequest(NetworkAccessManager *networkManager, const QString &username, const QString &password, QObject *parent);
};

class SomfyTahomaEventFetchRequest : public SomfyTahomaPostRequest
{
    Q_OBJECT

public:
    SomfyTahomaEventFetchRequest(NetworkAccessManager *networkManager, const QString &eventListenerId, QObject *parent);
};

#endif // SOMFYTAHOMAREQUESTS_H
