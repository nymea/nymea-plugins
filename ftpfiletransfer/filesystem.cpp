/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#include "filesystem.h"
#include "extern-plugininfo.h"

#include <QDir>
#include <QFile>
#include <QUuid>
#include <QDateTime>

FileSystem::FileSystem(QObject *parent) : QObject(parent)
{

}

void FileSystem::browseDevice(BrowseResult *result)
{
    QDir directory;
    if (!result->itemId().isEmpty()) {
        directory.setPath(result->itemId());
    } else {
        directory.setPath("/");
    }
    QFileInfoList list = directory.entryInfoList();

    foreach (QFileInfo item, list) {
        qCDebug(dcFtpFileTransfer) << "Found item" << item;
        BrowserItem browserItem;
        browserItem.setId(item.filePath());
        if (item.isHidden()) {
            continue;
        }
        if (item.isDir()) {
            browserItem.setIcon(BrowserItem::BrowserIconFolder);
            browserItem.setBrowsable(true);
        } else {
            browserItem.setIcon(BrowserItem::BrowserIconFile);
            browserItem.setBrowsable(false);
            browserItem.setExecutable(item.isReadable());
            browserItem.setActionTypeIds(QList<ActionTypeId>() <<ftpFileTransferUploadBrowserItemActionTypeId);
        }
        browserItem.setDescription("Date modified: " + item.lastModified().toString() + " Size: " + QString::number(item.size()/1000) + " kB");
        browserItem.setDisplayName(item.fileName());
        result->addItem(browserItem);
    }

    result->finish(Device::DeviceErrorNoError);
    return;
}

void FileSystem::browserItem(BrowserItemResult *result)
{
    qCDebug(dcFtpFileTransfer) << "Getting details for" << result->itemId();
    QString method;
    QVariantMap params;

    QDir directory(result->itemId());
    QStringList list = directory.entryList();

    foreach (QString item, list) {
        qCDebug(dcFtpFileTransfer) << "Found item" << item;
    }

    result->finish(Device::DeviceErrorNoError);
    return;
}

void FileSystem::executeBrowserItem(BrowserActionInfo *info)
{
    Q_UNUSED(info)
    return;
}

void FileSystem::executeBrowserItemAction(BrowserItemActionInfo *info)
{
    Q_UNUSED(info)

    return;
}
