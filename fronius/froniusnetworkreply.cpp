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

#include "froniusnetworkreply.h"

FroniusNetworkReply::~FroniusNetworkReply()
{
    if (m_networkReply) {
        // We don't need the finished signal any more, object gets deleted
        disconnect(m_networkReply, &QNetworkReply::finished, this, &FroniusNetworkReply::handleFinished);

        if (m_networkReply->isRunning()) {
            // Abort the reply, we are not interested in it any more
            m_networkReply->abort();
        }

        m_networkReply->deleteLater();
    }
}

QUrl FroniusNetworkReply::requestUrl() const
{
    return m_request.url();
}

QNetworkRequest FroniusNetworkReply::request() const
{
    return m_request;
}

QNetworkReply *FroniusNetworkReply::networkReply() const
{
    return m_networkReply;
}

QByteArray FroniusNetworkReply::readAll()
{
    return m_networkReply ? m_networkReply->readAll() : QByteArray();
}

QNetworkReply::NetworkError FroniusNetworkReply::error() const
{
    return m_networkReply ? m_networkReply->error() : m_error;
}

QString FroniusNetworkReply::errorString() const
{
    return m_networkReply ? m_networkReply->errorString() : m_errorString;
}

QUrl FroniusNetworkReply::url() const
{
    return m_networkReply ? m_networkReply->url() : m_request.url();
}

FroniusNetworkReply::FroniusNetworkReply(const QNetworkRequest &request, QObject *parent) :
    QObject(parent),
    m_request(request)
{

}

void FroniusNetworkReply::setNetworkReply(QNetworkReply *networkReply)
{
    m_networkReply = networkReply;

    // The QNetworkReply will be deleted in the destructor if set
    connect(m_networkReply, &QNetworkReply::finished, this, &FroniusNetworkReply::handleFinished);
}

void FroniusNetworkReply::cancel()
{
    if (m_finished)
        return;

    m_canceled = true;

    if (m_networkReply) {
        m_networkReply->abort();
        if (!m_finished)
            handleFinished();
        return;
    }

    m_error = QNetworkReply::OperationCanceledError;
    m_errorString = tr("Request canceled because the device address changed.");
    m_finished = true;
    emit finished();
}

bool FroniusNetworkReply::wasCanceled() const
{
    return m_canceled;
}

void FroniusNetworkReply::handleFinished()
{
    if (m_finished)
        return;

    m_finished = true;
    emit finished();
}
