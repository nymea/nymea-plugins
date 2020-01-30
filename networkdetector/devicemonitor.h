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

#ifndef DEVICEMONITOR_H
#define DEVICEMONITOR_H

#include <QObject>
#include <QProcess>
#include <QDateTime>

class DeviceMonitor : public QObject
{
    Q_OBJECT
public:
    explicit DeviceMonitor(const QString &name, const QString &macAddress, const QString &ipAddress, bool initialState, QObject *parent = nullptr);

    ~DeviceMonitor();

    void setGracePeriod(int minutes);

    void update();

signals:
    void addressChanged(const QString &address);
    void reachableChanged(bool reachable);
    void seen();

private:
    void lookupArpCache();
    void arping();
    void ping();

    void log(const QString &message);
    void warn(const QString &message);

private slots:
    void arpLookupFinished(int exitCode);
    void arpingFinished(int exitCode);
    void pingFinished(int exitCode);

private:
    QString m_name;
    QString m_macAddress;
    QString m_ipAddress;
    QDateTime m_lastSeenTime;

    bool m_reachable = false;
    int m_gracePeriod = 5;

    QProcess *m_arpLookupProcess = nullptr;
    QProcess *m_arpingProcess = nullptr;
    QProcess *m_pingProcess = nullptr;
};

#endif // DEVICEMONITOR_H
