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

#ifndef SNAPDCONTROL_H
#define SNAPDCONTROL_H

#include <QObject>
#include <QLocalSocket>

#include "integrations/thing.h"
#include "snapdconnection.h"

class SnapdControl : public QObject
{
    Q_OBJECT
public:
    explicit SnapdControl(Thing *thing, QObject *parent = nullptr);

    Thing *thing();

    bool available() const;
    bool connected() const;
    bool enabled() const;
    bool timerBasedSchedule() const;

private:
    Thing *m_device = nullptr;
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
