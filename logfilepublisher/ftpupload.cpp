#include "ftpupload.h"
#include "extern-plugininfo.h"

#include <QDateTime>

FtpUpload::FtpUpload(QNetworkAccessManager *networkManager, const QHostAddress &address, const QString &username, const QString &password, QObject *parent) :
    QObject(parent),
    m_networkAccessManager(networkManager),
    m_serverAddress(address),
    m_username(username),
    m_password(password)
{

}

void FtpUpload::uploadFile(const QString &fileName, const QString &targetName)
{
    QFile *file = new QFile(fileName);

    QFileInfo fileInfo(*file);
    QString targetFileName = QString::number(QDateTime::currentSecsSinceEpoch()) + "_" + targetName + "_" + fileInfo.fileName();
    QUrl url(m_serverAddress.toString() + targetFileName);
    url.setScheme("ftp");
    url.setUserName(m_username);
    url.setPassword(m_password);
    url.setPort(21);

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
