/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017-2018 Simon Stürz <simon.stuerz@guh.io               *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "snapdreply.h"

QString SnapdReply::requestPath() const
{
    return m_requestPath;
}

QString SnapdReply::requestMethod() const
{
    return m_requestMethod;
}

QByteArray SnapdReply::requestRawMessage() const
{
    return m_requestRawMessage;
}

int SnapdReply::statusCode() const
{
    return m_statusCode;
}

QString SnapdReply::statusMessage() const
{
    return m_statusMessage;
}

QHash<QString, QString> SnapdReply::header() const
{
    return m_header;
}

QVariantMap SnapdReply::dataMap() const
{
    return m_dataMap;
}

bool SnapdReply::isFinished() const
{
    return m_isFinished;
}

bool SnapdReply::isValid() const
{
    return m_valid;
}

SnapdReply::SnapdReply(QObject *parent) :
    QObject(parent)
{

}

void SnapdReply::setRequestPath(const QString &requestPath)
{
    m_requestPath = requestPath;
}

void SnapdReply::setRequestMethod(const QString &requestMethod)
{
    m_requestMethod = requestMethod;
}

void SnapdReply::setRequestRawMessage(const QByteArray &rawMessage)
{
    m_requestRawMessage = rawMessage;
}

void SnapdReply::setStatusCode(const int &statusCode)
{
    m_statusCode = statusCode;
}

void SnapdReply::setStatusMessage(const QString &statusMessage)
{
    m_statusMessage = statusMessage;
}

void SnapdReply::setHeader(const QHash<QString, QString> header)
{
    m_header = header;
}

void SnapdReply::setDataMap(const QVariantMap &dataMap)
{
    m_dataMap = dataMap;
}

void SnapdReply::setFinished(const bool &valid)
{
    m_isFinished = true;
    m_valid = valid;

    emit finished();
}
