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

#ifndef FTPUPLOAD_H
#define FTPUPLOAD_H

#include "network/networkaccessmanager.h"

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
    explicit FtpUpload(NetworkAccessManager *networkmanager, const QHostAddress &address, int port, const QString &username, const QString &password, QObject *parent = nullptr);

    void setLoginCredentials(const QString &username, const QString &password);
    void testConnection();
    void setFtpServer(const QHostAddress &address);
    void uploadFile(const QString &fileName, const QString &targetName);

private:
    NetworkAccessManager *m_networkAccessManager;
    QHash<QNetworkReply *, QFile *> m_fileUploads;
    QHostAddress m_serverAddress;
    int m_port;
    QString m_username;
    QString m_password;

signals:
    void testFinished(bool success);
    void connectionChanged(bool connected);
    void uploadProgress(int percentage);
    void uploadFinished(bool success);

private slots:
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void onFinished();
};

#endif // FTPUPLOAD_H
