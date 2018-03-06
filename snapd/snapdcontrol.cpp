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

#include "snapdcontrol.h"
#include "extern-plugininfo.h"

#include <QFileInfo>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonParseError>

SnapdControl::SnapdControl(Device *device, QObject *parent) :
    QObject(parent),
    m_device(device),
    m_snapdSocketPath("/run/snapd.socket")
{
    // If a change is one of following kind, the plugin will recognize it as update running
    m_updateChangeKinds.append("install-snap");
    m_updateChangeKinds.append("remove-snap");
    m_updateChangeKinds.append("refresh-snap");
    m_updateChangeKinds.append("revert-snap");

    m_snapConnection = new SnapdConnection(this);
    connect(m_snapConnection, &SnapdConnection::connectedChanged, this, &SnapdControl::onConnectedChanged);
}

Device *SnapdControl::device()
{
    return m_device;
}

bool SnapdControl::available() const
{
    QFileInfo fileInfo(m_snapdSocketPath);
    if (!fileInfo.exists()) {
        qCWarning(dcSnapd()) << "The socket descriptor" << m_snapdSocketPath << "does not exist";
        return false;
    }

    if (!fileInfo.isReadable()) {
        qCWarning(dcSnapd()) << "The socket descriptor" << m_snapdSocketPath << "is not readable";
        return false;
    }

    if (!fileInfo.isWritable()) {
        qCWarning(dcSnapd()) << "The socket descriptor" << m_snapdSocketPath << "is not writable";
        return false;
    }

    return true;
}

bool SnapdControl::connected() const
{
    return m_snapConnection->isConnected();
}

bool SnapdControl::enabled() const
{
    return m_enabled;
}

bool SnapdControl::timerBasedSchedule() const
{
    return m_timerBasedSchedule;
}

void SnapdControl::loadSystemInfo()
{
    if (!m_snapConnection)
        return;

    if (!m_snapConnection->isConnected())
        return;

    SnapdReply *reply = m_snapConnection->get("/v2/system-info", this);
    connect(reply, &SnapdReply::finished, this, &SnapdControl::onLoadSystemInfoFinished);
}

void SnapdControl::loadSnapList()
{
    if (!m_snapConnection)
        return;

    if (!m_snapConnection->isConnected())
        return;

    SnapdReply *reply = m_snapConnection->get("/v2/snaps", this);
    connect(reply, &SnapdReply::finished, this, &SnapdControl::onLoadSnapListFinished);
}

void SnapdControl::loadRunningChanges()
{
    if (!m_snapConnection)
        return;

    if (!m_snapConnection->isConnected())
        return;

    SnapdReply *reply = m_snapConnection->get("/v2/changes", this);
    connect(reply, &SnapdReply::finished, this, &SnapdControl::onLoadRunningChangesFinished);
}

void SnapdControl::configureRefreshSchedule()
{
    if (!m_snapConnection)
        return;

    if (!m_snapConnection->isConnected())
        return;

    QVariantMap configuration; QVariantMap configMap;
    configMap.insert("timer", m_preferedRefreshSchedule);
    configMap.insert("schedule", m_preferedRefreshSchedule);
    configuration.insert("refresh", configMap);

    qCDebug(dcSnapd()) << "Configure refresh schedule from" << m_currentRefreshSchedule << "-->" << m_preferedRefreshSchedule;

    SnapdReply *reply = m_snapConnection->put(QString("/v2/snaps/core/conf"), QJsonDocument::fromVariant(configuration).toJson(QJsonDocument::Compact), this);
    connect(reply, &SnapdReply::finished, this, &SnapdControl::onConfigureRefreshScheduleFinished);
}

bool SnapdControl::validAsyncResponse(const QVariantMap &responseMap)
{
    if (!responseMap.contains("type"))
        return false;

    if (responseMap.value("type") == "error")
        return false;

    return true;
}

void SnapdControl::onConnectedChanged(const bool &connected)
{
    if (connected) {
        device()->setStateValue(snapdControlSnapdAvailableStateTypeId, true);
        update();
    } else {
        device()->setStateValue(snapdControlSnapdAvailableStateTypeId, false);
    }
}

void SnapdControl::onLoadSystemInfoFinished()
{
    SnapdReply *reply = static_cast<SnapdReply *>(sender());
    if (!reply->isValid()) {
        qCDebug(dcSnapd()) << "Load system info request finished with error" << reply->requestPath();
        reply->deleteLater();
        return;
    }

    QVariantMap result = reply->dataMap().value("result").toMap();
    QDateTime lastRefreshTime = QDateTime::fromString(result.value("refresh").toMap().value("last").toString(), Qt::ISODate);
    QDateTime nextRefreshTime = QDateTime::fromString(result.value("refresh").toMap().value("next").toString(), Qt::ISODate);

    // Set update time information
    device()->setStateValue(snapdControlLastUpdateTimeStateTypeId, lastRefreshTime.toTime_t());
    device()->setStateValue(snapdControlNextUpdateTimeStateTypeId, nextRefreshTime.toTime_t());

    // Check if we are working on refresh timer or refresh schedule
    if (result.value("refresh").toMap().contains("schedule")) {
        // Schedule based core snap
        m_timerBasedSchedule = false;
        m_currentRefreshSchedule = result.value("refresh").toMap().value("schedule").toString();
    } else if (result.value("refresh").toMap().contains("timer")) {
        // Timer based core snap: snapd >= 2.31
        m_timerBasedSchedule = true;
        m_currentRefreshSchedule = result.value("refresh").toMap().value("timer").toString();
    }

    reply->deleteLater();

    // Check if the refresh schedule should be updated
    if (m_currentRefreshSchedule != m_preferedRefreshSchedule) {
        configureRefreshSchedule();
    }
}

void SnapdControl::onLoadSnapListFinished()
{
    SnapdReply *reply = static_cast<SnapdReply *>(sender());
    if (!reply->isValid()) {
        qCDebug(dcSnapd()) << "Load system info request finished with error" << reply->requestPath();
        reply->deleteLater();
        return;
    }

    emit snapListUpdated(reply->dataMap().value("result").toList());
    reply->deleteLater();
}

void SnapdControl::onLoadRunningChangesFinished()
{
    SnapdReply *reply = static_cast<SnapdReply *>(sender());
    if (!reply->isValid()) {
        qCDebug(dcSnapd()) << "Load running changes request finished with error" << reply->requestPath();
        reply->deleteLater();
        return;
    }

    // Load changes list
    QVariantList changes = reply->dataMap().value("result").toList();
    reply->deleteLater();

    // If there are no running changes, update is not running
    if (changes.isEmpty()) {
        // Update not running any more
        device()->setStateValue(snapdControlUpdateRunningStateTypeId, false);
        device()->setStateValue(snapdControlStatusStateTypeId, "-");
        return;
    }

    bool updateRunning = false;
    QString updateStatus = "-";

    // Verifiy if a change is running and which one is currently doing something
    foreach (const QVariant &changeVariant, changes) {
        QVariantMap changeMap = changeVariant.toMap();

        int changeId = changeMap.value("id").toInt();
        bool changeReady = changeMap.value("ready").toBool();
        QString changeKind = changeMap.value("kind").toString();
        QString changeStatus = changeMap.value("status").toString();
        QString changeSummary = changeMap.value("summary").toString();

        // If there is a change kind "doing" or "Do"
        if ( (changeStatus == "Doing" || changeStatus == "Do") && m_updateChangeKinds.contains(changeKind)) {
            // Set the status of the current running change
            updateRunning = true;
            if (changeStatus == "Doing") {
                updateStatus = changeSummary;
                qCDebug(dcSnapd()).noquote() << "Current change:" << changeId << (changeReady ? "ready" : "not ready") << changeStatus << changeKind << changeSummary;

            }
        }
    }

    device()->setStateValue(snapdControlUpdateRunningStateTypeId, updateRunning);
    device()->setStateValue(snapdControlStatusStateTypeId, updateStatus);
}

void SnapdControl::onConfigureRefreshScheduleFinished()
{
    SnapdReply *reply = static_cast<SnapdReply *>(sender());
    if (!reply->isValid()) {
        qCDebug(dcSnapd()) << "Set refresh schedule request finished with error" << reply->requestPath();
        reply->deleteLater();
        return;
    }

    if (!validAsyncResponse(reply->dataMap())) {
        qCWarning(dcSnapd()) << "Async refresh configuration request finished with error" << reply->dataMap().value("status").toString() << reply->dataMap().value("status-code").toInt();
        reply->deleteLater();
        return;
    }

    qCDebug(dcSnapd()) << "Configure refresh schedule finished successfully";
    reply->deleteLater();
}

void SnapdControl::onSnapRefreshFinished()
{
    SnapdReply *reply = static_cast<SnapdReply *>(sender());
    if (!reply->isValid()) {
        qCDebug(dcSnapd()) << "Snap refresh request finished with error" << reply->requestPath();
        reply->deleteLater();
        return;
    }

    if (!validAsyncResponse(reply->dataMap())) {
        qCWarning(dcSnapd()) << "Async refresh request finished with error" << reply->dataMap().value("status").toString() << reply->dataMap().value("status-code").toInt();
    } else {
        loadRunningChanges();
    }

    reply->deleteLater();
}

void SnapdControl::onSnapRevertFinished()
{
    SnapdReply *reply = static_cast<SnapdReply *>(sender());
    if (!reply->isValid()) {
        qCDebug(dcSnapd()) << "Snap revert request finished with error" << reply->requestPath();
        reply->deleteLater();
        return;
    }

    if (!validAsyncResponse(reply->dataMap())) {
        qCWarning(dcSnapd()) << "Async change request finished with error" << reply->dataMap().value("status").toString() << reply->dataMap().value("status-code").toInt();;
    } else {
        loadRunningChanges();
    }

    reply->deleteLater();
}

void SnapdControl::onCheckForUpdatesFinished()
{
    SnapdReply *reply = static_cast<SnapdReply *>(sender());
    if (!reply->isValid()) {
        qCDebug(dcSnapd()) << "Check for snap updates request finished with error" << reply->requestPath();
        reply->deleteLater();
        return;
    }

    qCDebug(dcSnapd()) << "Check for available snap updates finished.";
    if (reply->dataMap().value("result").toList().isEmpty()) {
        qCDebug(dcSnapd()) << "There are no snap updates available.";
        device()->setStateValue(snapdControlUpdateAvailableStateTypeId, false);
    } else {
        // Print available snap updates
        qCDebug(dcSnapd()) << "Following snaps can be updated:";
        foreach (const QVariant &resultVariant, reply->dataMap().value("result").toList()) {
            QVariantMap resultMap = resultVariant.toMap();
            qCDebug(dcSnapd()) << "    -->" << resultMap.value("name").toString() << resultMap.value("version").toString();
        }

        device()->setStateValue(snapdControlUpdateAvailableStateTypeId, true);
    }

    reply->deleteLater();
}

void SnapdControl::onChangeSnapChannelFinished()
{
    SnapdReply *reply = static_cast<SnapdReply *>(sender());
    if (!reply->isValid()) {
        qCDebug(dcSnapd()) << "Change snap channel request finished with error" << reply->requestPath();
        reply->deleteLater();
        return;
    }

    if (!validAsyncResponse(reply->dataMap())) {
        qCWarning(dcSnapd()) << "Async change request finished with error" << reply->dataMap().value("status").toString() << reply->dataMap().value("status-code").toInt();
    } else {
        loadRunningChanges();
    }

    reply->deleteLater();
}

void SnapdControl::enable()
{
    m_enabled = true;
    update();
}

void SnapdControl::disable()
{
    m_enabled = false;

    if (m_snapConnection) {
        m_snapConnection->close();
    }
}

void SnapdControl::update()
{
    if (!m_snapConnection)
        return;

    if (!available())
        return;

    if (!enabled())
        return;

    // Try to reconnect if unconnected
    if (m_snapConnection->state() == QLocalSocket::UnconnectedState) {
        m_snapConnection->connectToServer(m_snapdSocketPath, QLocalSocket::ReadWrite);
        return;
    }

    // Note: this makes sure the state is realy connected (including connection initialisation stuff)
    if (!m_snapConnection->isConnected())
        return;

    // Update information
    if (device()->stateValue(snapdControlUpdateRunningStateTypeId).toBool()) {
        // Note: if an update is running, just load the changes to save system resources
        loadRunningChanges();
    } else {
        // Normal update
        loadSystemInfo();
        loadSnapList();
        loadRunningChanges();
    }
}

void SnapdControl::snapRefresh()
{
    if (!m_snapConnection)
        return;

    if (!m_snapConnection->isConnected())
        return;

    QVariantMap request;
    request.insert("action", "refresh");

    qCDebug(dcSnapd()) << "Refresh all snaps";
    SnapdReply *reply = m_snapConnection->post("/v2/snaps", QJsonDocument::fromVariant(request).toJson(QJsonDocument::Compact), this);
    connect(reply, &SnapdReply::finished, this, &SnapdControl::onSnapRefreshFinished);
}

void SnapdControl::changeSnapChannel(const QString &snapName, const QString &channel)
{
    if (!m_snapConnection)
        return;

    if (!m_snapConnection->isConnected())
        return;

    QVariantMap request;
    request.insert("action", "refresh");
    request.insert("channel", channel);

    qCDebug(dcSnapd()) << "Refresh snap" << snapName << "to channel" << channel;
    SnapdReply *reply = m_snapConnection->post(QString("/v2/snaps/%1").arg(snapName), QJsonDocument::fromVariant(request).toJson(QJsonDocument::Compact), this);
    connect(reply, &SnapdReply::finished, this, &SnapdControl::onChangeSnapChannelFinished);
}

void SnapdControl::checkForUpdates()
{
    if (!m_snapConnection)
        return;

    if (!m_snapConnection->isConnected())
        return;

    qCDebug(dcSnapd()) << "Checking for available snap updates";
    SnapdReply *reply = m_snapConnection->get("/v2/find?select=refresh", this);
    connect(reply, &SnapdReply::finished, this, &SnapdControl::onCheckForUpdatesFinished);
}

void SnapdControl::setPreferedRefreshTime(int startTime)
{
    // Schedule the refresh between startTime and startTime + 59 minutes
    QTime start(startTime, 0, 0);
    QTime end = start.addSecs(3540);
    m_preferedRefreshSchedule  = QString("%1-%2").arg(start.toString("h:mm")).arg(end.toString("h:mm"));
    qCDebug(dcSnapd()) << "Set prefered refresh schedule to " << m_preferedRefreshSchedule;
}

void SnapdControl::snapRevert(const QString &snapName)
{
    if (!m_snapConnection)
        return;

    if (!m_snapConnection->isConnected())
        return;

    QVariantMap request;
    request.insert("action", "revert");

    qCDebug(dcSnapd()) << "Revert snap" << snapName;
    SnapdReply *reply = m_snapConnection->post(QString("/v2/snaps/%1").arg(snapName), QJsonDocument::fromVariant(request).toJson(QJsonDocument::Compact), this);
    connect(reply, &SnapdReply::finished, this, &SnapdControl::onSnapRevertFinished);
}
