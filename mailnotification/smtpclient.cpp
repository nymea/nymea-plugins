// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "smtpclient.h"
#include "extern-plugininfo.h"

#include <QDateTime>

SmtpClient::SmtpClient(QObject *parent):
    QObject(parent)
{
    m_socket = new QSslSocket(this);

    connect(m_socket, &QSslSocket::connected, this, &SmtpClient::connected);
    connect(m_socket, &QSslSocket::readyRead, this, &SmtpClient::readData);
    connect(m_socket, &QSslSocket::disconnected, this, &SmtpClient::disconnected);
    connect(m_socket, &QSslSocket::encrypted, this, &SmtpClient::onEncrypted);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_socket, &QTcpSocket::errorOccurred, this, &SmtpClient::onSocketError);
#else
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
#endif
}

void SmtpClient::connectToHost()
{
    switch (m_encryptionType) {
    case EncryptionTypeNone:
    case EncryptionTypeTLS:
        // Note: the handshake will be done later, not from the beginning
        m_socket->connectToHost(m_host, m_port);
        break;
    case EncryptionTypeSSL:
        m_socket->connectToHostEncrypted(m_host, m_port);
        break;
    }
}

void SmtpClient::testLogin()
{
    qCDebug(dcMailNotification()) << "Starting test login";
    m_testLogin = true;
    setState(StateInitialize);
    m_socket->close();
    connectToHost();
}

void SmtpClient::connected()
{
    qCDebug(dcMailNotification()) << "Connected";
}

void SmtpClient::disconnected()
{
    qCDebug(dcMailNotification()) << "Disconnected";
    setState(StateIdle);
    sendNextMail();
}

void SmtpClient::onEncrypted()
{
    qCDebug(dcMailNotification()) << "Socket encrypted";
    send("EHLO localhost");
    setState(StateAuthentification);
}

void SmtpClient::readData()
{
    while(m_socket->canReadLine()) {
        QString responseLine;
        responseLine = m_socket->readLine();

        qCDebug(dcMailNotification()) << "<--" << responseLine;

        bool responseCodeParseSuccess = false;
        int responseCode = responseLine.left(3).toInt(&responseCodeParseSuccess);
        if (!responseCodeParseSuccess) {
            qCWarning(dcMailNotification()) << "Could not convert status code to a valid integer" << responseLine;
            if (m_state != StateIdle) {
                handleSmtpFailure();
                continue;
            }
        } else {
            processServerResponse(responseCode, responseLine);
        }
    }
}

int SmtpClient::sendMail(const QString &subject, const QString &body)
{
    static int ids = 0;
    Message message;
    message.subject = subject;
    message.body = body;
    message.id = ids++;

    m_messageQueue.enqueue(message);
    sendNextMail();
    return message.id;
}

void SmtpClient::setHost(const QString &host)
{
    m_host = host;
}

void SmtpClient::setPort(const quint16 &port)
{
    m_port = port;
}

void SmtpClient::setEncryptionType(const SmtpClient::EncryptionType &encryptionType)
{
    m_encryptionType = encryptionType;
}

void SmtpClient::setAuthenticationMethod(const SmtpClient::AuthenticationMethod &authenticationMethod)
{
    m_authenticationMethod = authenticationMethod;
}

void SmtpClient::setUser(const QString &user)
{
    m_user = user;
}

void SmtpClient::setPassword(const QString &password)
{
    m_password = password;
}

void SmtpClient::setSender(const QString &sender)
{
    m_sender = sender;
}

void SmtpClient::setRecipients(const QStringList &recipients)
{
    m_recipients = recipients;
}

QString SmtpClient::createDateString()
{
    return QDateTime::currentDateTime().toString(Qt::RFC2822Date);
}

void SmtpClient::setState(SmtpClient::State state)
{
    if (m_state == state)
        return;

    qCDebug(dcMailNotification()) << state;
    m_state = state;
}

void SmtpClient::processServerResponse(int responseCode, const QString &response)
{
    qCDebug(dcMailNotification()) << "Server response:" << responseCode << response;
    switch (m_state) {
    case StateIdle:
        // Check if we have to send an other email, otherwise we are done and remain in idle
        sendNextMail();
        break;
    case StateInitialize:
        if (responseCode == 220) {
            send("EHLO localhost");

            if (m_encryptionType == EncryptionTypeNone) {
                setState(StateAuthentification);
                break;
            }

            if (m_encryptionType == EncryptionTypeSSL) {
                setState(StateHandShake);
                break;
            }

            if (m_encryptionType == EncryptionTypeTLS) {
                setState(StateStartTls);
                break;
            }
        }
        break;
    case StateHandShake:
        // Ignore server information messages
        if (responseCode == 250) {
            break;
        }

        // We need a 220 befor continue
        if (responseCode == 220) {
            if (!m_socket->isEncrypted() && m_encryptionType != EncryptionTypeNone) {
                qCDebug(dcMailNotification()) << "Start client encryption...";
                m_socket->startClientEncryption();
            }
        } else {
            handleUnexpectedSmtpCode(responseCode, response);
        }
        break;
    case StateStartTls:
        // Ignore server information messages until we get a '250 ...' instead of '250-....'
        if (responseCode == 250 && response.at(3) != ' ') {
            break;
        }
        if (responseCode == 250) {
            send("STARTTLS");
            setState(StateHandShake);
        } else {
            handleUnexpectedSmtpCode(responseCode, response);
        }
        break;
    case StateAuthentification:
        // Ignore server information messages of '250-....' (with dash), we need a clear "250 ..."
        if (responseCode == 250 && response.at(3) != ' ') {
            break;
        }

        if (responseCode == 250 || responseCode == 220) {
            if (m_authenticationMethod == AuthenticationMethodLogin) {
                send("AUTH LOGIN");
                setState(StateUser);
                break;
            }

            if (m_authenticationMethod == AuthenticationMethodPlain) {
                send("AUTH PLAIN " + QByteArray().append(static_cast<char>(0))
                     .append(m_user.toUtf8())
                     .append(static_cast<char>(0))
                     .append(m_password.toUtf8())
                     .toBase64());

                // If we just want to test the Login, we are almost done here
                if (!m_testLogin) {
                    setState(StateMail);
                } else {
                    setState(StateTestLoginFinished);
                }
                break;
            }
        } else {
            handleUnexpectedSmtpCode(responseCode, response);
        }
        break;
    case StateUser:
        if (responseCode == 334) {
            send(QByteArray().append(m_user.toUtf8()).toBase64());
            setState(StatePassword);
        } else {
            handleUnexpectedSmtpCode(responseCode, response);
        }
        break;
    case StatePassword:
        if (responseCode == 334) {
            send(QByteArray().append(m_password.toUtf8()).toBase64());
            // if we just want to test the Login, we are almost done here
            if (!m_testLogin) {
                setState(StateMail);
            } else {
                setState(StateTestLoginFinished);
            }
            break;
        } else {
            handleUnexpectedSmtpCode(responseCode, response);
        }
        break;
    case StateTestLoginFinished:
        // Ignore server information messages
        if (responseCode == 250) {
            break;
        }

        if (responseCode == 235) {
            emit testLoginFinished(true);
        } else {
            emit testLoginFinished(false);
        }
        m_socket->close();
        m_testLogin = false;
        break;
    case StateMail:
        // Ignore server information messages
        if (responseCode == 250) {
            break;
        }

        if (responseCode == 235) {
            send("MAIL FROM:<" + m_sender + ">");

            // Prepare queue for recipients
            m_recipientsQueue.clear();
            qCDebug(dcMailNotification()) << "Prepare recipients list" << m_recipients;
            foreach (const QString &recipient, m_recipients) {
                m_recipientsQueue.enqueue(recipient.trimmed());
            }
            setState(StateRcpt);
        } else {
            handleUnexpectedSmtpCode(responseCode, response);
        }
        break;
    case StateRcpt:
        // Ignore server information messages until we get a '250 ...' instead of '250-....'
        if (responseCode == 250 && response.at(3) != ' ') {
            break;
        }

        if (responseCode == 250) {
            send("RCPT TO:<" + m_recipientsQueue.dequeue() + ">");

            if (m_recipientsQueue.isEmpty()) {
                setState(StateData);
            }
        } else {
            handleUnexpectedSmtpCode(responseCode, response);
        }
        break;
    case StateData:
        // Ignore server information messages until we get a '250 ...' instead of '250-....'
        if (responseCode == 250 && response.at(3) != ' ') {
            break;
        }

        if (responseCode == 250) {
            send("DATA");
            setState(StateBody);
        } else {
            handleUnexpectedSmtpCode(responseCode, response);
        }
        break;
    case StateBody:
        if (responseCode == 354) {
            send(m_messageData);
            setState(StateQuit);
        } else {
            handleUnexpectedSmtpCode(responseCode, response);
        }
        break;
    case StateQuit:
        // Ignore server information messages until we get a '250 ...' instead of '250-....'
        if (responseCode == 250 && response.at(3) != ' ') {
            break;
        }

        if (responseCode == 250) {
            emit sendMailFinished(true, m_message.id);
            send("QUIT");
            setState(StateClose);
        } else {
            handleUnexpectedSmtpCode(responseCode, response);
        }
        break;
    case StateClose:
        if (responseCode == 221) {
            m_socket->close();
        } else {
            qCDebug(dcMailNotification()) << "The server does not handle the QUIT command. This is ok, we close the socket either way.";
        }

        // some mail server does not recognize the QUIT command...so close the connection either way
        m_socket->close();
        break;
    }
}

void SmtpClient::sendNextMail()
{
    // Check if there is a mail left to send
    if (m_messageQueue.isEmpty())
        return;

    // Check if busy
    if (m_state != StateIdle)
        return;

    sendEmailInternally(m_messageQueue.dequeue());
}

void SmtpClient::sendEmailInternally(const Message &message)
{
    qCDebug(dcMailNotification()) << "Start sending message" << message.subject << message.body;

    // Initialize data for sending
    m_message = message;
    m_messageData.clear();

    // Create plain message content
    m_messageData = "To: " + m_recipients.join(",") + "\r\n";
    m_messageData.append("From: " + m_sender + "\r\n");
    m_messageData.append("Subject: " + message.subject + "\r\n");
    m_messageData.append("Date: " + createDateString() + "\r\n");
    m_messageData.append("Content-Type: text/plain; charset=\"UTF-8\"\r\n");
    m_messageData.append("Content-Transfer-Encoding: quoted-printable\r\n");
    m_messageData.append("MIME-Version: 1.0\r\n");
    m_messageData.append("X-Mailer: nymea;\r\n");
    m_messageData.append("\r\n");
    m_messageData.append(message.body);
    m_messageData.append("\r\n.\r\n");

    setState(StateInitialize);

    // Make sure the connection starts from the beginning
    m_socket->close();
    connectToHost();
}

void SmtpClient::handleSmtpFailure()
{
    if (m_testLogin) {
        emit testLoginFinished(false);
    } else {
        emit sendMailFinished(false, m_message.id);
    }

    // Clean up
    m_socket->close();
    m_messageData.clear();
    m_testLogin = false;

    // Set idle state (handles the queue)
    setState(StateIdle);
}

void SmtpClient::handleUnexpectedSmtpCode(int responseCode, const QString &serverMessage)
{
    qCWarning(dcMailNotification()) << "Received unexpected error code from smtp server" << responseCode << serverMessage;
    handleSmtpFailure();
}

void SmtpClient::onSocketError(QAbstractSocket::SocketError error)
{
    qCWarning(dcMailNotification()) << "Mail socket error" << error << m_socket->errorString();
    if (m_state != StateIdle) {
        handleSmtpFailure();
    }
}

void SmtpClient::send(const QString &data)
{
    qCDebug(dcMailNotification()) << "-->" << data;
    m_socket->write(data.toUtf8() + "\r\n");
    m_socket->flush();
}


