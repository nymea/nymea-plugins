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

#ifndef FTPUPLOAD_H
#define FTPUPLOAD_H

#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHostAddress>


class FtpUpload : public QObject
{
    Q_OBJECT
public:
    explicit FtpUpload(const QHostAddress &address, int port, const QString &username, const QString &password, QObject *parent = nullptr);

    void setLoginCredentials(const QString &username, const QString &password);
    void setFtpServer(const QHostAddress &address);
    void uploadFile(const QString &fileName, const QString &targetName);

private:
    QNetworkAccessManager *m_networkAccessManager;
    QHash<QNetworkReply *, QFile *> m_fileUploads;
    QHostAddress m_serverAddress;
    int m_port;
    QString m_username;
    QString m_password;

signals:
    void uploadProgress(int percentage);
    void uploadFinished(bool success);

private slots:
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void onFinished();
};

#endif // FTPUPLOAD_H
