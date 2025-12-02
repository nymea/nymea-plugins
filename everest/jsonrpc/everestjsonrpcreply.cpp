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

#include "everestjsonrpcreply.h"

EverestJsonRpcReply::EverestJsonRpcReply(int commandId, QString method, QVariantMap params, QObject *parent)
    : QObject{parent},
    m_commandId{commandId},
    m_method{method},
    m_params{params}
{
    m_timer.setInterval(30000);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [this](){
        m_error = ErrorTimeout;
        emit finished();
    });
}

EverestJsonRpcReply::Error EverestJsonRpcReply::error() const
{
    return m_error;
}

int EverestJsonRpcReply::commandId() const
{
    return m_commandId;
}

QString EverestJsonRpcReply::method() const
{
    return m_method;
}

QVariantMap EverestJsonRpcReply::params() const
{
    return m_params;
}

QVariantMap EverestJsonRpcReply::requestMap()
{
    QVariantMap request;
    request.insert("id", commandId());
    request.insert("jsonrpc", "2.0");
    request.insert("method", method());
    if (!m_params.isEmpty())
        request.insert("params", params());

    return request;
}

bool EverestJsonRpcReply::retry() const
{
    return m_retry;
}

int EverestJsonRpcReply::retryCount() const
{
    return m_retryCount;
}

int EverestJsonRpcReply::retryLimit() const
{
    return m_retryLimit;
}

QVariantMap EverestJsonRpcReply::response() const
{
    return m_response;
}

void EverestJsonRpcReply::setResponse(const QVariantMap &response)
{
    m_response = response;
}

void EverestJsonRpcReply::startWaiting()
{
    m_timer.start();
}

void EverestJsonRpcReply::finishReply(Error error)
{
    m_timer.stop();
    m_error = error;
    emit finished();
}
