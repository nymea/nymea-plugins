/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017-2018 Simon Stürz <simon.stuerz@guh.io               *
 *                                                                         *
 *  This file is part of guh.                                              *
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

void SnapdControl::loadChange(const int &change)
{
    if (!m_snapConnection)
        return;

    if (!m_snapConnection->isConnected())
        return;

    SnapdReply *reply = m_snapConnection->get(QString("/v2/changes/%1").arg(QString::number(change)), this);
    connect(reply, &SnapdReply::finished, this, &SnapdControl::onLoadChangeFinished);
}

void SnapdControl::processChange(const QVariantMap &changeMap)
{
    int changeId = changeMap.value("id").toInt();
    bool changeReady = changeMap.value("ready").toBool();
    QString changeKind = changeMap.value("kind").toString();
    QString changeStatus = changeMap.value("status").toString();
    QString changeSummary = changeMap.value("summary").toString();

    qCDebug(dcSnapd()) << changeStatus << changeKind << changeSummary;

    // Add this change if not already finishished or added
    if (!m_watchingChanges.contains(changeId) && !changeReady)
        m_watchingChanges.append(changeId);

    // If change is on Doing, update the status
    if (changeStatus == "Doing") {
        device()->setStateValue(snapdControlStatusStateTypeId, changeSummary);
    }

    // If this change is on ready, we can remove it from our watch list
    if (changeReady) {
        qCDebug(dcSnapd()).noquote() << changeKind << (changeReady ? "finished." : "running.") << changeSummary;
        m_watchingChanges.removeAll(changeId);
    }
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

    reply->deleteLater();
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

    foreach (const QVariant &changeVariant, reply->dataMap().value("result").toList()) {
        processChange(changeVariant.toMap());
    }

    // Check if there are still changes around
    if (reply->dataMap().value("result").toList().isEmpty()) {
        // If there are no running changes, we can forget old ones
        m_watchingChanges.clear();

        // Update not running any more
        device()->setStateValue(snapdControlUpdateRunningStateTypeId, false);
        device()->setStateValue(snapdControlStatusStateTypeId, "-");
    } else {
        // Update running
        device()->setStateValue(snapdControlUpdateRunningStateTypeId, true);
    }

    reply->deleteLater();
}

void SnapdControl::onLoadChangeFinished()
{
    SnapdReply *reply = static_cast<SnapdReply *>(sender());
    if (!reply->isValid()) {
        qCDebug(dcSnapd()) << "Load change request finished with error" << reply->requestPath();
        reply->deleteLater();
        return;
    }

    processChange(reply->dataMap().value("result").toMap());

    if (m_watchingChanges.isEmpty()) {
        device()->setStateValue(snapdControlUpdateRunningStateTypeId, false);
        device()->setStateValue(snapdControlStatusStateTypeId, "-");
    }

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

    //qCDebug(dcSnapd()) << qUtf8Printable(QJsonDocument::fromVariant(reply->dataMap()).toJson(QJsonDocument::Indented));
    if (!validAsyncResponse(reply->dataMap())) {
        qCWarning(dcSnapd()) << "Async change request finished with error" << reply->dataMap().value("status").toString() << reply->dataMap().value("").toString();
    } else {
        loadChange(reply->dataMap().value("change").toInt());
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

    //qCDebug(dcSnapd()) << qUtf8Printable(QJsonDocument::fromVariant(reply->dataMap()).toJson(QJsonDocument::Indented));
    if (!validAsyncResponse(reply->dataMap())) {
        qCWarning(dcSnapd()) << "Async change request finished with error" << reply->dataMap().value("status").toString() << reply->dataMap().value("").toString();
    } else {
        loadChange(reply->dataMap().value("change").toInt());
    }
    reply->deleteLater();
}

void SnapdControl::onCheckForUpdatesFinished()
{
    SnapdReply *reply = static_cast<SnapdReply *>(sender());
    if (!reply->isValid()) {
        qCDebug(dcSnapd()) << "Snap check for updates request finished with error" << reply->requestPath();
        reply->deleteLater();
        return;
    }

    //qCDebug(dcSnapd()) << qUtf8Printable(QJsonDocument::fromVariant(reply->dataMap()).toJson(QJsonDocument::Indented));
    device()->setStateValue(snapdControlUpdateAvailableStateTypeId, !reply->dataMap().value("result").toList().isEmpty());
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
        qCWarning(dcSnapd()) << "Async change request finished with error" << reply->dataMap().value("status").toString() << reply->dataMap().value("").toString();
    } else {
        loadChange(reply->dataMap().value("change").toInt());
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
    if (!m_watchingChanges.isEmpty()) {
        // We are watching currently changes
        foreach (const int &change, m_watchingChanges) {
            loadChange(change);
        }
        loadRunningChanges();
    } else {
        // Normal refresh
        loadSystemInfo();
        loadSnapList();
        checkForUpdates();
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

    SnapdReply *reply = m_snapConnection->get("/v2/find?select=refresh", this);
    connect(reply, &SnapdReply::finished, this, &SnapdControl::onCheckForUpdatesFinished);
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
