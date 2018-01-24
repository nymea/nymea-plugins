#ifndef SNAPDREPLY_H
#define SNAPDREPLY_H

#include <QHash>
#include <QObject>
#include <QVariantMap>

class SnapdReply : public QObject
{
    Q_OBJECT

    friend class SnapdConnection;

public:
    // Request
    QString requestPath() const;
    QString requestMethod() const;
    QByteArray requestRawMessage() const;

    // Response
    int statusCode() const;
    QString statusMessage() const;
    QHash<QString, QString> header() const;
    QVariantMap dataMap() const;

    bool isFinished() const;
    bool isValid() const;

private:
    explicit SnapdReply(QObject *parent = nullptr);

    QString m_requestPath;
    QString m_requestMethod;
    QByteArray m_requestRawMessage;

    int m_statusCode;
    QString m_statusMessage;
    QHash<QString, QString> m_header;
    QVariantMap m_dataMap;

    bool m_isFinished = false;
    bool m_valid = false;

    // Methods for SnapdConnection
    void setRequestPath(const QString &requestPath);
    void setRequestMethod(const QString &requestMethod);
    void setRequestRawMessage(const QByteArray &rawMessage);

    void setStatusCode(const int &statusCode);
    void setStatusMessage(const QString &statusMessage);
    void setHeader(const QHash<QString, QString> header);
    void setDataMap(const QVariantMap &dataMap);
    void setFinished(const bool &valid = true);

signals:
    void finished();

};

#endif // SNAPDREPLY_H
