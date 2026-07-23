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

#ifndef FRONIUSNETWORKREPLY_H
#define FRONIUSNETWORKREPLY_H

#include <QObject>
#include <QNetworkReply>

class FroniusNetworkReply : public QObject
{
    Q_OBJECT

    friend class FroniusSolarConnection;

public:
    ~FroniusNetworkReply();

    QUrl requestUrl() const;

    QNetworkRequest request() const;
    QNetworkReply *networkReply() const;
    QByteArray readAll();
    QNetworkReply::NetworkError error() const;
    QString errorString() const;
    QUrl url() const;

signals:
    void finished();

private:
    explicit FroniusNetworkReply(const QNetworkRequest &request, QObject *parent = nullptr);

    QNetworkRequest m_request;
    QNetworkReply *m_networkReply = nullptr;
    QNetworkReply::NetworkError m_error = QNetworkReply::NoError;
    QString m_errorString;
    bool m_finished = false;
    bool m_canceled = false;

    void setNetworkReply(QNetworkReply *networkReply);
    void cancel();
    bool wasCanceled() const;
    void handleFinished();
};

#endif // FRONIUSNETWORKREPLY_H
