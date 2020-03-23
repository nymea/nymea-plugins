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

#include "ftpupload.h"
#include "extern-plugininfo.h"

#include <QDateTime>

FtpUpload::FtpUpload(NetworkAccessManager *networkManager, const QHostAddress &address, int port, const QString &username, const QString &password, QObject *parent) :
    QObject(parent),
    m_networkAccessManager(networkManager),
    m_serverAddress(address),
    m_port(port),
    m_username(username),
    m_password(password)
{
    qCDebug(dcFtpFileTransfer()) << "FtpUpload Init" << address.toString() << port << username << password;
}

void FtpUpload::testConnection()
{
    emit testFinished(true);
}



void FtpUpload::uploadFile(const QString &fileName, const QString &targetName)
{
    QUrl url;
    url.setHost(m_serverAddress.toString());
    url.setPath("/"+targetName);
    url.setScheme("ftp");
    url.setUserName(m_username);
    url.setPassword(m_password);
    url.setPort(m_port);
    qCDebug(dcFtpFileTransfer()) << "uploadFile request" << targetName << fileName << url ;

    Q_UNUSED(fileName);
    /*QFile *file = new QFile(fileName);
    QFileInfo fileInfo(*file);
    if (file->open(QIODevice::ReadOnly)) {
        // Start upload
        QNetworkReply *reply = m_networkAccessManager->put(QNetworkRequest(url), file);
        m_fileUploads.insert(reply, file);
        // And connect to the progress upload signal
        connect(reply, &QNetworkReply::uploadProgress, this, &FtpUpload::onUploadProgress);
        connect(reply, &QNetworkReply::finished, this, &FtpUpload::onFinished);
    }*/
}

void FtpUpload::onFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    if (m_fileUploads.contains(reply)) {
        QFile *file = m_fileUploads.take(reply);
        if (!file) {
            qCWarning(dcFtpFileTransfer()) << "ftpUpload::onFinished - QFile object not found";
            return;
        }
        file->close();
        file->deleteLater();

        if (reply->error()) {
            emit uploadFinished(false);
            qCWarning(dcFtpFileTransfer()) << "Upload failed:" << reply->errorString();
        } else {
            emit uploadFinished(true);
        }
    }
}

void FtpUpload::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    // Display the progress of the upload
    emit uploadProgress(100 * bytesSent/bytesTotal);
}
