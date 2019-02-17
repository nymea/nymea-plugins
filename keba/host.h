/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stürz <simon.stuerz@guh.io>                   *
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

#ifndef HOST_H
#define HOST_H

#include <QString>
#include <QDebug>
#include <QDateTime>

class Host
{
public:
    Host();

    QString macAddress() const;
    void setMacAddress(const QString &macAddress);

    QString hostName() const;
    void setHostName(const QString &hostName);

    QString address() const;
    void setAddress(const QString &address);

    void seen();
    QDateTime lastSeenTime() const;

    bool reachable() const;
    void setReachable(bool reachable);

private:
    QString m_macAddress;
    QString m_hostName;
    QString m_address;
    QDateTime m_lastSeenTime;
    bool m_reachable;
};
Q_DECLARE_METATYPE(Host)

QDebug operator<<(QDebug dbg, const Host &host);

#endif // HOST_H
