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
