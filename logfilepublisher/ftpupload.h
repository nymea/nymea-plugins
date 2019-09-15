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
    explicit FtpUpload(QNetworkAccessManager *networkManager, const QHostAddress &address, const QString &username, const QString &password, QObject *parent = nullptr);
    void setLoginCredentials(const QString &username, const QString &password);
    void setFtpServer(const QHostAddress &address);

    void uploadFile(const QString &fileName, const QString &targetName);

private:
    QNetworkAccessManager *m_networkAccessManager;
    QHash<QNetworkReply *, QFile *> m_fileUploads;
    QHostAddress m_serverAddress;
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
