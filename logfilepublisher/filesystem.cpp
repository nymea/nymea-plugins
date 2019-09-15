#include "filesystem.h"
#include "extern-plugininfo.h"

#include <QDir>
#include <QFile>
#include <QUuid>
#include <QDateTime>

FileSystem::FileSystem(QObject *parent) : QObject(parent)
{

}

Device::BrowseResult FileSystem::browse(const QString &itemId, Device::BrowseResult &result)
{
    QDir directory;
    if (!itemId.isEmpty()) {
        directory.setPath(itemId);
    } else {
        directory.setPath("/");
    }
    QFileInfoList list = directory.entryInfoList();

    foreach (QFileInfo item, list) {
        qCDebug(dcLogfilePublisher) << "Found item" << item;
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
        }
        browserItem.setDescription("Date modified: " + item.lastModified().toString() + " Size: " + QString::number(item.size()/1000) + " kB");
        browserItem.setDisplayName(item.fileName());
        result.items.append(browserItem);
    }

    result.status = Device::DeviceErrorNoError;

    return result;
}

Device::BrowserItemResult FileSystem::browserItem(const QString &itemId, Device::BrowserItemResult &result)
{
    qCDebug(dcLogfilePublisher) << "Getting details for" << itemId;
    QString idString = itemId;
    QString method;
    QVariantMap params;

    QDir directory(itemId);
    QStringList list = directory.entryList();

    foreach (QString item, list) {
        qCDebug(dcLogfilePublisher) << "Found item" << item;
    }

    result.status = Device::DeviceErrorNoError;
    return result;
}

Device::DeviceError FileSystem::launchBrowserItem(const QString &itemId)
{
    Q_UNUSED(itemId)
    return Device::DeviceErrorNoError;
}

int FileSystem::executeBrowserItemAction(const QString &itemId, const ActionTypeId &actionTypeId)
{
    Q_UNUSED(itemId)
    Q_UNUSED(actionTypeId)

    return 0;
}
