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

#include "ftpupload.h"
#include "extern-plugininfo.h"

#include <QDateTime>

FtpUpload::FtpUpload(const QHostAddress &address, int port, const QString &username, const QString &password, QObject *parent) :
    QObject(parent),
    m_serverAddress(address),
    m_port(port),
    m_username(username),
    m_password(password)
{
    m_networkAccessManager = new QNetworkAccessManager(this);
}

void FtpUpload::uploadFile(const QString &fileName, const QString &targetName)
{
    QFile *file = new QFile(fileName);

    QFileInfo fileInfo(*file);
    QUrl url(m_serverAddress.toString() + targetName);
    url.setScheme("ftp");
    url.setUserName(m_username);
    url.setPassword(m_password);
    url.setPort(m_port);

    if (file->open(QIODevice::ReadOnly)) {
        // Start upload
        QNetworkReply *reply = m_networkAccessManager->put(QNetworkRequest(url), file);
        // And connect to the progress upload signal
        connect(reply, &QNetworkReply::uploadProgress, this, &FtpUpload::onUploadProgress);
        connect(reply, &QNetworkReply::finished, this, &FtpUpload::onFinished);
    }
}

void FtpUpload::onFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    QFile *file = m_fileUploads.take(reply);
    file->close();
    file->deleteLater();

    if (reply->error()) {
        emit uploadFinished(false);
        qWarning(dcLogfilePublisher()) << "Upload failed:" << reply->errorString();
    } else {
        emit uploadFinished(true);
    }
}

void FtpUpload::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    // Display the progress of the upload
    emit uploadProgress(100 * bytesSent/bytesTotal);
}
