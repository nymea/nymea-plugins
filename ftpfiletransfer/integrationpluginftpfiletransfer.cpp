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

#include "integrationpluginftpfiletransfer.h"
#include "integrations/integrationplugin.h"
#include "plugininfo.h"

#include <QHostInfo>

IntegrationPluginFtpFileTransfer::IntegrationPluginFtpFileTransfer()
{

}

void IntegrationPluginFtpFileTransfer::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == ftpFileTransferThingClassId) {
        if (!m_fileSystem) {
            m_fileSystem = new FileSystem(this);
        }

        pluginStorage()->beginGroup(thing->id().toString());
        QString username = pluginStorage()->value("username").toString();
        QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();

        QHostAddress address = QHostAddress(thing->paramValue(ftpFileTransferThingIpParamTypeId).toString());
        if (address.isNull()) {
            qCWarning(dcFtpFileTransfer()) << "Invalid IP address";
            return info->finish(Thing::ThingErrorSetupFailed);
        }
        int port = thing->paramValue(ftpFileTransferThingPortParamTypeId).toInt();

        FtpUpload *ftpUpload = new FtpUpload(hardwareManager()->networkManager(), address, port, username, password, this);
        connect(ftpUpload, &FtpUpload::testFinished, this, &IntegrationPluginFtpFileTransfer::onFtpUploadConnectionTestFinished);
        connect(ftpUpload, &FtpUpload::uploadProgress, this, &IntegrationPluginFtpFileTransfer::onUploadFinished);
        connect(ftpUpload, &FtpUpload::uploadFinished, this, &IntegrationPluginFtpFileTransfer::onUploadProgress);

        m_ftpUploads.insert(thing, ftpUpload);
        m_asyncSetups.insert(ftpUpload, info);
        ftpUpload->testConnection();

        connect(info, &ThingSetupInfo::aborted, this, [thing, ftpUpload, this]{
            m_asyncSetups.remove(ftpUpload);

            if (m_ftpUploads.contains(thing)) {
                m_ftpUploads.take(thing)->deleteLater();
            }
        });
    } else {
        qCWarning(dcFtpFileTransfer()) << "setupThing; Thing class not found" << thing->thingClassId();
        info->finish(Thing::ThingErrorSetupFailed);
    }
}

void IntegrationPluginFtpFileTransfer::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == ftpFileTransferThingClassId) {
        FtpUpload *ftpUpload = m_ftpUploads.take(thing);
        if (ftpUpload)
            ftpUpload->deleteLater();
    }
    if (myThings().isEmpty()) {
        m_fileSystem->deleteLater();
        m_fileSystem = nullptr;
    }
}


void IntegrationPluginFtpFileTransfer::startPairing(ThingPairingInfo *info)
{
    if (info->thingClassId() == ftpFileTransferThingClassId) {
        info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter the user credentials"));
        return;
    }
}

void IntegrationPluginFtpFileTransfer::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    if (info->thingClassId() == ftpFileTransferThingClassId) {
        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", secret);
        pluginStorage()->endGroup();

        info->finish(Thing::ThingErrorNoError);
    } else {
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginFtpFileTransfer::browseThing(BrowseResult *result)
{
    qCDebug(dcFtpFileTransfer()) << "Browse device called" << result->itemId();
    m_fileSystem->browseThing(result);
}

void IntegrationPluginFtpFileTransfer::browserItem(BrowserItemResult *result)
{
    qCDebug(dcFtpFileTransfer()) << "Browse Item called" << result->itemId();
    m_fileSystem->browserItem(result);
}

void IntegrationPluginFtpFileTransfer::executeBrowserItemAction(BrowserItemActionInfo *info)
{
    qCDebug(dcFtpFileTransfer()) << "Execute browser Item action called" << info->browserItemAction().itemId();

    if (info->browserItemAction().actionTypeId() == ftpFileTransferUploadBrowserItemActionTypeId) {
        FtpUpload *ftpUpload = m_ftpUploads.value(info->thing());
        if (!ftpUpload) {
            qCWarning(dcFtpFileTransfer()) << "executeBrowseItemAction FtpUpload object not found";
            return info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
        ftpUpload->uploadFile(info->browserItemAction().itemId(), info->browserItemAction().itemId().split("/").last());
        info->finish(Thing::ThingErrorNoError);
    } else {
        qCWarning(dcFtpFileTransfer()) << "executeBrowseItemAction: actionTypeId not found" << info->browserItemAction().actionTypeId();
        info->finish(Thing::ThingErrorActionTypeNotFound);
    }
}

void IntegrationPluginFtpFileTransfer::onConnectionChanged(bool connected)
{
    FtpUpload *ftpUpload = static_cast<FtpUpload *>(sender());
    Thing *thing = m_ftpUploads.key(ftpUpload);
    if (!thing) {
        return;
    }
    thing->setStateValue(ftpFileTransferConnectedStateTypeId, connected);
}

void IntegrationPluginFtpFileTransfer::onUploadProgress(int percentage)
{
    FtpUpload *ftpUpload = static_cast<FtpUpload *>(sender());
    Thing *thing = m_ftpUploads.key(ftpUpload);
    if (!thing) {
        return;
    }
    thing->setStateValue(ftpFileTransferUploadProgressStateTypeId, percentage);
    thing->setStateValue(ftpFileTransferUploadInProgressStateTypeId, true);
}

void IntegrationPluginFtpFileTransfer::onUploadFinished(bool success)
{
    FtpUpload *ftpUpload = static_cast<FtpUpload *>(sender());
    Thing *thing = m_ftpUploads.key(ftpUpload);
    if (!thing) {
        return;
    }
    thing->setStateValue(ftpFileTransferUploadInProgressStateTypeId, false);
    emitEvent(Event(ftpFileTransferUploadFinishedEventTypeId, thing->id(), ParamList() << Param(ftpFileTransferUploadFinishedEventSuccessParamTypeId, success)));
}

void IntegrationPluginFtpFileTransfer::onFtpUploadConnectionTestFinished(bool success)
{
    FtpUpload *ftpUpload = static_cast<FtpUpload *>(sender());
    if (m_asyncSetups.contains(ftpUpload)) {
        ThingSetupInfo *info = m_asyncSetups.take(ftpUpload);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorSetupFailed);
            m_ftpUploads.remove(m_ftpUploads.key(ftpUpload));
            ftpUpload->deleteLater();
        }
    }
}
