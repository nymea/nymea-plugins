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

#ifndef EVERESTJSONRPCREPLY_H
#define EVERESTJSONRPCREPLY_H

#include <QTimer>
#include <QObject>
#include <QVariantMap>

class EverestJsonRpcReply : public QObject
{
    Q_OBJECT

    friend class EverestJsonRpcClient;

public:
    enum Error {
        ErrorNoError = 0,
        ErrorTimeout,
        ErrorConnectionError,
        ErrorJsonRpcError
    };
    Q_ENUM(Error)

    Error error() const;

    // Request
    int commandId() const;
    QString method() const;
    QVariantMap params() const;
    QVariantMap requestMap();

    // Retry logic, as of now only for init requests and if
    // they return ResponseErrorErrorNoDataAvailable
    bool retry() const;
    int retryCount() const;
    int retryLimit() const;

    // Response
    QVariantMap response() const;

signals:
    void finished();

private:
    explicit EverestJsonRpcReply(int commandId, QString method, QVariantMap params = QVariantMap(), QObject *parent = nullptr);

    int m_commandId;
    QString m_method;
    QVariantMap m_params;
    QVariantMap m_response;

    QTimer m_timer;
    Error m_error = ErrorNoError;

    bool m_retry = false;
    int m_retryCount = 0;
    int m_retryLimit = 5;

    void setResponse(const QVariantMap &response);
    void startWaiting();
    void finishReply(Error error = ErrorNoError);
};

#endif // EVERESTJSONRPCREPLY_H
