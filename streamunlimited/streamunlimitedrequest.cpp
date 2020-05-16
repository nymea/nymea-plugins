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

#include "streamunlimitedrequest.h"
#include "extern-plugininfo.h"

#include <network/networkaccessmanager.h>

#include <QNetworkReply>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QUrlQuery>

StreamUnlimitedGetRequest::StreamUnlimitedGetRequest(NetworkAccessManager *nam, const QHostAddress &hostAddress, int port, const QString &path, const QStringList &roles, QObject *parent):
    QObject(parent)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(hostAddress.toString());
    url.setPort(port);
    url.setPath("/api/getData");

    QUrlQuery query;
    query.addQueryItem("path", path);
    query.addQueryItem("roles", roles.join(','));
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkReply *reply = nam->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [=](){
        deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcStreamUnlimited()) << "Request to" << hostAddress.toString() << "failed:" << reply->errorString();
            emit error(reply->error());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qCWarning(dcStreamUnlimited()) << "Json parse error in reply from" << hostAddress.toString() << ":" << parseError.errorString();
            emit error(QNetworkReply::UnknownContentError);
            return;
        }

        QVariantList resultList = jsonDoc.toVariant().toList();

        if (resultList.count() != roles.count()) {
            qCWarning(dcStreamUnlimited()) << "Unexpected result length!";
            emit error(QNetworkReply::UnknownContentError);
            return;
        }

//        qCDebug(dcStreamUnlimited()) << "Result list" << resultList;
        QVariantMap resultMap;
        for (int i = 0; i < roles.length(); i++) {
            resultMap.insert(roles.at(i), resultList.at(i));
        }
        emit finished(resultMap);
    });
}

StreamUnlimitedSetRequest::StreamUnlimitedSetRequest(NetworkAccessManager *nam, const QHostAddress &hostAddress, int port, const QString &path, const QString &role, const QVariantMap &value, QObject *parent):
    QObject(parent)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(hostAddress.toString());
    url.setPort(port);
    url.setPath("/api/setData");

    QUrlQuery query;
    query.addQueryItem("path", path);
    query.addQueryItem("role", role);
    query.addQueryItem("value", QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact));
    url.setQuery(query);

    QNetworkRequest request(url);

    qCDebug(dcStreamUnlimited()) << "Set data request:" << url.toString();

    QNetworkReply *reply = nam->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [=](){
        deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcStreamUnlimited()) << "Request to" << hostAddress.toString() << "failed:" << reply->errorString();
            emit error(reply->error());
            return;
        }

        QByteArray data = reply->readAll();
        emit finished(data);
    });
}

StreamUnlimitedBrowseRequest::StreamUnlimitedBrowseRequest(NetworkAccessManager *nam, const QHostAddress &hostAddress, int port, const QString &path, const QStringList &roles, QObject *parent):
    QObject(parent)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(hostAddress.toString());
    url.setPort(port);
    url.setPath("/api/getRows");

    QUrlQuery query;
    query.addQueryItem("path", path);
    query.addQueryItem("roles", roles.join(','));
    query.addQueryItem("from", "0");
    query.addQueryItem("to", "1000");
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkReply *reply = nam->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [=](){
        deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcStreamUnlimited()) << "Request to" << hostAddress.toString() << "failed:" << reply->errorString();
            emit error(reply->error());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qCWarning(dcStreamUnlimited()) << "Json parse error in reply from" << hostAddress.toString() << ":" << parseError.errorString();
            emit error(QNetworkReply::UnknownContentError);
            return;
        }

        emit finished(jsonDoc.toVariant().toMap());
    });
}

