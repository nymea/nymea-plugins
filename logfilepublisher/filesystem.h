#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "types/browseritem.h"
#include "types/browseritemaction.h"
#include "devices/device.h"

#include <QObject>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QVector>

class FileSystem : public QObject
{
    Q_OBJECT
public:
    explicit FileSystem(QObject *parent = nullptr);

    Device::BrowseResult browse(const QString &itemId, Device::BrowseResult &result);
    Device::BrowserItemResult browserItem(const QString &itemId, Device::BrowserItemResult &result);
    Device::DeviceError launchBrowserItem(const QString &itemId);
    int executeBrowserItemAction(const QString &itemId, const ActionTypeId &actionTypeId);

signals:

public slots:
};

#endif // FILESYSTEM_H
