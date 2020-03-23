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

#include "filesystem.h"
#include "extern-plugininfo.h"

#include <QDir>
#include <QFile>
#include <QUuid>
#include <QDateTime>

FileSystem::FileSystem(QObject *parent) : QObject(parent)
{

}

void FileSystem::browseThing(BrowseResult *result)
{
    QDir directory;
    if (!result->itemId().isEmpty()) {
        directory.setPath(result->itemId());
    } else {
        directory.setPath("/");
    }
    QFileInfoList list = directory.entryInfoList();

    foreach (QFileInfo item, list) {
        BrowserItem browserItem;
        if (item.isHidden()) {
            continue;
        }
        if (item.isDir()) {
            qCDebug(dcFtpFileTransfer()) << "Found dir" << item.filePath();
            browserItem.setId(item.filePath());
            browserItem.setIcon(BrowserItem::BrowserIconFolder);
            browserItem.setBrowsable(true);
        } else {
            qCDebug(dcFtpFileTransfer()) << "Found file" << item.filePath();
             browserItem.setId(item.filePath());
            browserItem.setIcon(BrowserItem::BrowserIconFile);
            browserItem.setBrowsable(false);
            browserItem.setExecutable(item.isReadable());
            browserItem.setActionTypeIds(QList<ActionTypeId>() <<ftpFileTransferUploadBrowserItemActionTypeId);
        }
        browserItem.setDescription("Date modified: " + item.lastModified().toString() + " Size: " + QString::number(item.size()/1000) + " kB");
        browserItem.setDisplayName(item.fileName());
        result->addItem(browserItem);
    }
    return result->finish(Thing::ThingErrorNoError);
}

void FileSystem::browserItem(BrowserItemResult *result)
{
    qCDebug(dcFtpFileTransfer) << "Getting details for" << result->itemId();

    QFileInfo info(result->itemId());
    if (info.isReadable()) {
        result->finish(Thing::ThingErrorNoError);
    } else {
        qCWarning(dcFtpFileTransfer()) << "browseItem: itemId not found" << result->itemId();
        result->finish(Thing::ThingErrorItemNotFound);
    }
}
