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

#ifndef SNAPDCONTROL_H
#define SNAPDCONTROL_H

#include <QObject>
#include <QLocalSocket>

#include "devices/device.h"
#include "snapdconnection.h"

class SnapdControl : public QObject
{
    Q_OBJECT
public:
    explicit SnapdControl(Device *device, QObject *parent = nullptr);

    Device *device();

    bool available() const;
    bool connected() const;
    bool enabled() const;
    bool timerBasedSchedule() const;

private:
    Device *m_device = nullptr;
    SnapdConnection *m_snapConnection = nullptr;

    QString m_snapdSocketPath;
    bool m_enabled = true;

    QStringList m_updateChangeKinds;

    bool m_timerBasedSchedule = false;
    QString m_currentRefreshSchedule;
    QString m_preferredRefreshSchedule;

    // Update calls
    void loadSystemInfo();
    void loadSnapList();
    void loadRunningChanges();

    void configureRefreshSchedule();

    bool validAsyncResponse(const QVariantMap &responseMap);

private slots:
    void onConnectedChanged(const bool &connected);

    // Response handler for different requests
    void onLoadSystemInfoFinished();
    void onLoadSnapListFinished();
    void onLoadRunningChangesFinished();
    void onConfigureRefreshScheduleFinished();

    void onSnapRefreshFinished();
    void onSnapRevertFinished();
    void onCheckForUpdatesFinished();
    void onChangeSnapChannelFinished();

signals:
    void snapListUpdated(const QVariantList &snapList);

public slots:
    void enable();
    void disable();

    // Snapd request calls
    void update();    
    void snapRefresh();
    void checkForUpdates();

    void setPreferredRefreshTime(int startTime);

    void snapRevert(const QString &snapName);
    void changeSnapChannel(const QString &snapName, const QString &channel);
};

#endif // SNAPDCONTROL_H
