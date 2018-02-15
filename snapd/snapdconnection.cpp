/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017-2018 Simon Stürz <simon.stuerz@guh.io               *
 *                                                                         *
 *  This file is part of guh.                                              *
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


#include "snapdconnection.h"
#include "extern-plugininfo.h"

#include <QPointer>
#include <QJsonDocument>
#include <QJsonParseError>


SnapdConnection::SnapdConnection(QObject *parent) :
    QLocalSocket(parent)
{
    connect(this, &QLocalSocket::connected, this, &SnapdConnection::onConnected);
    connect(this, &QLocalSocket::disconnected, this, &SnapdConnection::onDisconnected);
    connect(this, &QLocalSocket::readyRead, this, &SnapdConnection::onReadyRead);
    connect(this, &QLocalSocket::stateChanged, this, &SnapdConnection::onStateChanged);
    connect(this, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(onError(QLocalSocket::LocalSocketError)));
}

SnapdConnection::~SnapdConnection()
{
    close();
}

SnapdReply *SnapdConnection::get(const QString &path, QObject *parent)
{
    SnapdReply *reply = new SnapdReply(parent);
    reply->setRequestPath(path);
    reply->setRequestMethod("GET");
    reply->setRequestRawMessage(createRequestHeader("GET", path));

    // Enqueue the new reply
    m_replyQueue.enqueue(reply);
    sendNextRequest();

    // Note: the caller owns the object now
    return reply;
}

SnapdReply *SnapdConnection::post(const QString &path, const QByteArray &payload, QObject *parent)
{
    SnapdReply *reply = new SnapdReply(parent);
    reply->setRequestPath(path);
    reply->setRequestMethod("POST");
    QByteArray header = createRequestHeader("POST", path, payload);
    reply->setRequestRawMessage(header.append(payload));

    // Enqueue the new reply
    m_replyQueue.enqueue(reply);
    sendNextRequest();

    // Note: the caller owns the object now
    return reply;
}

SnapdReply *SnapdConnection::put(const QString &path, const QByteArray &payload, QObject *parent)
{
    SnapdReply *reply = new SnapdReply(parent);
    reply->setRequestPath(path);
    reply->setRequestMethod("PUT");
    QByteArray header = createRequestHeader("PUT", path, payload);
    reply->setRequestRawMessage(header.append(payload));

    // Enqueue the new reply
    m_replyQueue.enqueue(reply);
    sendNextRequest();

    // Note: the caller owns the object now
    return reply;
}

bool SnapdConnection::isConnected() const
{
    return m_connected;
}

void SnapdConnection::setConnected(const bool &connected)
{
    if (m_connected == connected)
        return;

    m_connected = connected;
    emit connectedChanged(m_connected);

    // Clean up replies of disconnected
    if (!m_connected) {
        // Clean up current reply
        if (m_currentReply) {
            m_currentReply->setFinished(false);
            m_currentReply = nullptr;
        }

        // Clean up queue
        while (!m_replyQueue.isEmpty()) {
            QPointer<SnapdReply> reply = m_replyQueue.dequeue();
            if (!reply.isNull()) {
                reply->deleteLater();
            }
        }
    } else {
        // Start with a clean parsing
        m_payload.clear();
        m_header.clear();
        m_chuncked = false;
    }
}

QByteArray SnapdConnection::createRequestHeader(const QString &method, const QString &path, const QByteArray &payload)
{
    QByteArray request;
    request.append(QString("%1 %2 HTTP/1.1\r\n").arg(method).arg(path).toUtf8());
    request.append("Host: http\r\n");
    request.append("Accept: *\r\n");
    if (!payload.isEmpty()) {
        request.append("Content-Type: application/json\r\n");
        request.append(QString("Content-Length: %1\r\n").arg(payload.count()).toUtf8());
    }

    request.append("\r\n");
    return request;
}

QByteArray SnapdConnection::getChunckedPayload(const QByteArray &payload)
{
    // Read line by line
    QStringList payloadLines = QString::fromUtf8(payload).split(QRegExp("\r\n"));
    if (payloadLines.count() < 4) {
        qCWarning(dcSnapd()) << "Chuncked payload invalid linecount" << payloadLines.count();
        return QByteArray();
    }

    int payloadSize = payloadLines.at(2).toInt(0, 16);

    if (m_debug)
        qCDebug(dcSnapd()) << "Payload size" << payloadSize;

    if (payloadLines.at(3).toUtf8().size() != payloadSize) {
        qCWarning(dcSnapd()) << "Invalid payload size" << payloadLines.at(3).toUtf8().size() << "!=" << payloadSize;
        return QByteArray();
    }

    // Return just the payload
    return payloadLines.at(3).toUtf8();
}

void SnapdConnection::processData()
{
    if (!m_currentReply) {
        qCWarning(dcSnapd()) << "Data received without current reply" << m_payload;
        return;
    }

    if (m_header.isEmpty()) {
        qCWarning(dcSnapd()) << "Could not process data. There is no header.";
        m_currentReply->setFinished();
        return;
    }

    // Get the raw payload
    QByteArray payloadData;
    if (m_chuncked) {
        payloadData = getChunckedPayload(m_payload);
    } else {
        payloadData = m_payload;
    }

    // Check if there are data to process
    if (m_payload.isEmpty()) {
        qCWarning(dcSnapd()) << "Could not process data. There is no payload to process.";
        return;
    }

    // Parse header
    QHash<QString, QString> parsedHeader;
    QStringList headerLines = QString::fromUtf8(m_header).split(QRegExp("\r\n"));

    // Read status line
    QString statusLine = headerLines.takeFirst();
    QStringList statusLineTokens = statusLine.split(QRegExp("[ \r\n][ \r\n]*"));
    if (statusLineTokens.count() < 3) {
        qCWarning(dcSnapd()) << "Could not parse HTTP status line:" << statusLine;
        return;
    }

    bool statusCodeOk = false;
    int statusCode = statusLineTokens.at(1).simplified().toInt(&statusCodeOk);
    if (!statusCodeOk) {
        qCWarning(dcSnapd()) << "Could not parse HTTP status code:" << statusLineTokens.at(1);
        return;
    }

    QString statusMessage;
    for (int i = 2; i < statusLineTokens.count(); i++) {
        statusMessage.append(statusLineTokens.at(i).simplified());
        if (i < statusLineTokens.count() -1) {
            statusMessage.append(" ");
        }
    }

    // Verify header formating
    foreach (const QString &line, headerLines) {
        if (!line.contains(":")) {
            qCWarning(dcSnapd()) << "Invalid HTTP header. Missing \":\" in line" << line;
            return;
        }

        int index = line.indexOf(":");
        QString key = line.left(index).toUtf8().simplified();
        QString value = line.right(line.count() - index - 1).toUtf8().simplified();
        //qCDebug(dcSnapd()) << "   Key:" << key << "Value:" << value;
        parsedHeader.insert(key, value);
    }

    // Parse payload
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(payloadData, &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcSnapd()) << "Got invalid JSON data from snapd:" << error.offset << error.errorString();
        qCWarning(dcSnapd()) << qUtf8Printable(payloadData);
        return;
    }

    if (m_debug)
        qCDebug(dcSnapd()) << "<--" << m_currentReply->requestPath() << statusCode << statusMessage;

    // Fill reply
    m_currentReply->setStatusCode(statusCode);
    m_currentReply->setStatusMessage(statusMessage);
    m_currentReply->setHeader(parsedHeader);
    m_currentReply->setDataMap(jsonDoc.toVariant().toMap());
    m_currentReply->setFinished();

    // Current data stream finished, reset for new messages
    m_payload.clear();
    m_header.clear();
    m_chuncked = false;

    // Ready for next reply
    m_currentReply = nullptr;
    sendNextRequest();
}

void SnapdConnection::sendNextRequest()
{
    // Check if nothing else to do
    if (m_replyQueue.isEmpty())
        return;

    // Check a reply is currently pending
    if (m_currentReply)
        return;

    // Dequeue and send next reply
    SnapdReply *reply = m_replyQueue.dequeue();
    m_currentReply = reply;

    if (m_debug)
        qCDebug(dcSnapd()) << "-->" << reply->requestMethod() << reply->requestPath();

    // Send current reply request. If write failes, the reply is finished invalid and the owner has to delete it
    qint64 bytesWritten = write(reply->requestRawMessage());
    if (bytesWritten < 0) {
        qCWarning(dcSnapd()) << "Could not write request data" << reply->requestMethod() << reply->requestMethod();
        m_currentReply->setFinished(false);
        m_currentReply = nullptr;
        sendNextRequest();
    }
}

void SnapdConnection::onConnected()
{
    setConnected(true);
}

void SnapdConnection::onDisconnected()
{
    setConnected(false);
}

void SnapdConnection::onError(const QLocalSocket::LocalSocketError &socketError)
{
    qCWarning(dcSnapd()) << "Socket error" << socketError << errorString();
}

void SnapdConnection::onStateChanged(const QLocalSocket::LocalSocketState &state)
{
    switch (state) {
    case QLocalSocket::UnconnectedState:
        qCDebug(dcSnapd()) << "Disconnected from snapd.";
        break;
    case QLocalSocket::ConnectingState:
        qCDebug(dcSnapd()) << "Connecting to snapd...";
        break;
    case QLocalSocket::ConnectedState:
        qCDebug(dcSnapd()) << "Connected to snapd.";
        break;
    case QLocalSocket::ClosingState:
        qCDebug(dcSnapd()) << "Closing connection to snapd.";
        break;
    default:
        break;
    }
}

void SnapdConnection::onReadyRead()
{
    QByteArray data = readAll();

    if (m_debug)
        qCDebug(dcSnapd()) << "Data received:" << data;

    // If we are not appending to a reply
    if (!m_chuncked) {

        // Parse header
        int headerIndex = data.indexOf("\r\n\r\n");
        if (headerIndex < 0) {
            qCWarning(dcSnapd()) << "Invalid response format. Could not find header/payload mark.";
            return;
        }

        m_header = data.left(headerIndex);

        if (m_debug)
            qCDebug(dcSnapd()) << "Header:" << m_header;

        QByteArray payload = data.right(data.length() - (headerIndex));

        if (m_debug)
            qCDebug(dcSnapd()) << "Payload" << payload;

        // Check if this message is chuncked
        if (m_header.contains("chunked")) {

            if (m_debug)
                qCDebug(dcSnapd()) << "Chuncked message receiving";

            m_chuncked = true;
            m_payload.append(payload);

            if (m_payload.endsWith("\r\n0\r\n\r\n")) {
                // Chuncked message finished
                processData();
            }
        } else {
            // Not chucked
            m_payload = payload;
            processData();
        }
    } else {
        m_payload.append(data);
        if (m_payload.endsWith("\r\n0\r\n\r\n")) {
            // Chuncked message finished
            processData();
        }
    }
}
