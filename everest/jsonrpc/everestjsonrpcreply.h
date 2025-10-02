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
