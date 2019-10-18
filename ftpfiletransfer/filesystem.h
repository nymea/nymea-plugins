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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "types/browseritem.h"
#include "types/browseritemaction.h"
#include "devices/device.h"

#include "devices/browseractioninfo.h"
#include "devices/browseresult.h"
#include "devices/browseritemactioninfo.h"
#include "devices/browseritemresult.h"

#include <QObject>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QVector>

class FileSystem : public QObject
{
    Q_OBJECT
public:
    explicit FileSystem(QObject *parent = nullptr);

    void browseDevice(BrowseResult *result);
    void browserItem(BrowserItemResult *result);
    void executeBrowserItem(BrowserActionInfo *info);
    void executeBrowserItemAction(BrowserItemActionInfo *info);

signals:

public slots:
};

#endif // FILESYSTEM_H
