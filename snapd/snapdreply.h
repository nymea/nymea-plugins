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
