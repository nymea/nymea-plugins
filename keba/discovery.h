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

#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <QObject>
#include <QProcess>
#include <QHostInfo>
#include <QTimer>

#include "host.h"

class Discovery : public QObject
{
    Q_OBJECT
public:
    explicit Discovery(QObject *parent = nullptr);

    void discoverHosts(int timeout);
    void abort();
    bool isRunning() const;

signals:
    void finished(const QList<Host> &hosts);

private:
    QStringList getDefaultTargets();

    void finishDiscovery();

private slots:
    void discoveryFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void hostLookupDone(const QHostInfo &info);
    void arpLookupDone(int exitCode, QProcess::ExitStatus exitStatus);
    void onTimeout();

private:
    QList<QProcess*> m_discoveryProcesses;
    QTimer m_timeoutTimer;

    QHash<QProcess*, Host*> m_pendingArpLookups;
    QHash<QString, Host*> m_pendingNameLookups;
    QList<Host*> m_scanResults;
};

#endif // DISCOVERY_H

