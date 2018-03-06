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

#ifndef SNAPDREPLY_H
#define SNAPDREPLY_H

#include <QHash>
#include <QObject>
#include <QVariantMap>

class SnapdReply : public QObject
{
    Q_OBJECT

    friend class SnapdConnection;

public:
    // Request
    QString requestPath() const;
    QString requestMethod() const;
    QByteArray requestRawMessage() const;

    // Response
    int statusCode() const;
    QString statusMessage() const;
    QHash<QString, QString> header() const;
    QVariantMap dataMap() const;

    bool isFinished() const;
    bool isValid() const;

private:
    explicit SnapdReply(QObject *parent = nullptr);

    QString m_requestPath;
    QString m_requestMethod;
    QByteArray m_requestRawMessage;

    int m_statusCode;
    QString m_statusMessage;
    QHash<QString, QString> m_header;
    QVariantMap m_dataMap;

    bool m_isFinished = false;
    bool m_valid = false;

    // Methods for SnapdConnection
    void setRequestPath(const QString &requestPath);
    void setRequestMethod(const QString &requestMethod);
    void setRequestRawMessage(const QByteArray &rawMessage);

    void setStatusCode(const int &statusCode);
    void setStatusMessage(const QString &statusMessage);
    void setHeader(const QHash<QString, QString> header);
    void setDataMap(const QVariantMap &dataMap);
    void setFinished(const bool &valid = true);

signals:
    void finished();

};

#endif // SNAPDREPLY_H
