#include "snapdreply.h"

QString SnapdReply::requestPath() const
{
    return m_requestPath;
}

QString SnapdReply::requestMethod() const
{
    return m_requestMethod;
}

QByteArray SnapdReply::requestRawMessage() const
{
    return m_requestRawMessage;
}

int SnapdReply::statusCode() const
{
    return m_statusCode;
}

QString SnapdReply::statusMessage() const
{
    return m_statusMessage;
}

QHash<QString, QString> SnapdReply::header() const
{
    return m_header;
}

QVariantMap SnapdReply::dataMap() const
{
    return m_dataMap;
}

bool SnapdReply::isFinished() const
{
    return m_isFinished;
}

bool SnapdReply::isValid() const
{
    return m_valid;
}

SnapdReply::SnapdReply(QObject *parent) :
    QObject(parent)
{

}

void SnapdReply::setRequestPath(const QString &requestPath)
{
    m_requestPath = requestPath;
}

void SnapdReply::setRequestMethod(const QString &requestMethod)
{
    m_requestMethod = requestMethod;
}

void SnapdReply::setRequestRawMessage(const QByteArray &rawMessage)
{
    m_requestRawMessage = rawMessage;
}

void SnapdReply::setStatusCode(const int &statusCode)
{
    m_statusCode = statusCode;
}

void SnapdReply::setStatusMessage(const QString &statusMessage)
{
    m_statusMessage = statusMessage;
}

void SnapdReply::setHeader(const QHash<QString, QString> header)
{
    m_header = header;
}

void SnapdReply::setDataMap(const QVariantMap &dataMap)
{
    m_dataMap = dataMap;
}

void SnapdReply::setFinished(const bool &valid)
{
    m_isFinished = true;
    m_valid = valid;

    emit finished();
}
