/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "smtpclient.h"
#include "extern-plugininfo.h"

#include <QDateTime>

Q_LOGGING_CATEGORY(dcSmtpClient, "SmtpClient")

SmtpClient::SmtpClient(QObject *parent):
    QObject(parent)
{
    m_socket = new QSslSocket(this);

    connect(m_socket, &QSslSocket::connected, this, &SmtpClient::connected);
    connect(m_socket, &QSslSocket::readyRead, this, &SmtpClient::readData);
    connect(m_socket, &QSslSocket::disconnected, this, &SmtpClient::disconnected);
    connect(m_socket, &QSslSocket::encrypted, this, &SmtpClient::onEncrypted);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
}

void SmtpClient::connectToHost()
{
    switch (m_encryptionType) {
    case EncryptionTypeNone:
    case EncryptionTypeTLS:
        m_socket->connectToHost(m_host, m_port);
        break;
    case EncryptionTypeSSL:
        m_socket->connectToHostEncrypted(m_host, m_port);
        break;
    }
}

void SmtpClient::testLogin()
{
    m_testLogin = true;
    setState(StateInitialize);
    m_socket->close();
    connectToHost();
}

void SmtpClient::connected()
{
    qCDebug(dcSmtpClient()) << "Connected";
}

void SmtpClient::disconnected()
{
    qCDebug(dcSmtpClient()) << "Disconnected";
    setState(StateIdle);
    sendNextMail();
}

void SmtpClient::onEncrypted()
{
    qCDebug(dcSmtpClient()) << "Socket encrypted";
    send("EHLO localhost");
    setState(StateAuthentification);
}

void SmtpClient::readData()
{
    QString response;
    QString responseLine;

    while(m_socket->canReadLine() && responseLine[3] != ' ') {
        responseLine = m_socket->readLine();
        response.append(responseLine);
    }
    responseLine.truncate(3);
    qCDebug(dcSmtpClient()) << "<--" << response;

    switch (m_state) {
    case StateIdle:
        sendNextMail();
        break;
    case StateInitialize:
        if (responseLine == "220") {
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
        if (responseLine == "250" || responseLine == "220") {
            if (!m_socket->isEncrypted() && m_encryptionType != EncryptionTypeNone) {
                qCDebug(dcSmtpClient()) << "Start client encryption...";
                m_socket->startClientEncryption();
            }
        }
        break;
    case StateStartTls:
        if (responseLine == "250") {
            send("STARTTLS");
            setState(StateHandShake);
        }
        break;
    case StateAuthentification:
        if (responseLine == "250") {
            if (m_authenticationMethod == AuthenticationMethodLogin) {
                send("AUTH LOGIN");
                setState(StateUser);
                break;
            }
            if (m_authenticationMethod == AuthenticationMethodPlain) {
                send("AUTH PLAIN " + QByteArray().append((char) 0).append(m_user).append((char) 0).append(m_password).toBase64());
                // if we just want to test the Login, we are almost done here
                if (!m_testLogin) {
                    setState(StateMail);
                } else {
                    setState(StateTestLoginFinished);
                }
                break;
            }
        }
        break;
    case StateUser:
        if (responseLine == "334") {
            send(QByteArray().append(m_user).toBase64());
            setState(StatePassword);
        }
        break;
    case StatePassword:
        if (responseLine == "334") {
            send(QByteArray().append(m_password).toBase64());
            // if we just want to test the Login, we are almost done here
            if (!m_testLogin) {
                setState(StateMail);
            } else {
                setState(StateTestLoginFinished);
            }
            break;
        }
        break;
    case StateTestLoginFinished:
        if (responseLine == "235") {
            emit testLoginFinished(true);
        } else {
            emit testLoginFinished(false);
        }
        m_socket->close();
        m_testLogin = false;
        break;
    case StateMail:
        if (responseLine == "235") {
            send("MAIL FROM:<" + m_sender + ">");
            setState(StateRcpt);
        }
        break;
    case StateRcpt:
        if (responseLine == "250") {
            send("RCPT TO:<" + m_rcpt + ">");
            setState(StateData);
        }
        break;
    case StateData:
        if (responseLine == "250") {
            send("DATA");
            setState(StateBody);
        }
        break;
    case StateBody:
        if (responseLine == "354") {
            send(m_messageData + "\r\n.\r\n");
            setState(StateQuit);
        }
        break;
    case StateQuit:
        if (responseLine == "250") {
            emit sendMailFinished(true, m_message.actionId);
            send("QUIT");
            setState(StateClose);
        }
        break;
    case StateClose:
        if (responseLine == "221") {
            m_socket->close();
        }
        // some mail server does not recognize the QUIT command...so close the connection either way
        m_socket->close();
        break;
    }
}

void SmtpClient::sendMail(const QString &subject, const QString &body, const ActionId &actionId)
{
    Message message;
    message.subject = subject;
    message.body = body;
    message.actionId = actionId;

    m_messageQueue.enqueue(message);
    sendNextMail();
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

void SmtpClient::setRecipient(const QString &rcpt)
{
    m_rcpt = rcpt;
}

QString SmtpClient::createDateString()
{
    return QDateTime::currentDateTime().toString(Qt::RFC2822Date);
}

void SmtpClient::setState(SmtpClient::State state)
{
    if (m_state == state)
        return;

    qCDebug(dcSmtpClient()) << state;
    m_state = state;
}

bool SmtpClient::verifyResponseCode(int responseCode)
{
    if (responseCode >= 500) {
        return false;
    }
    return true;
}

void SmtpClient::sendNextMail()
{
    // Check if there is a mail left to send
    if (m_messageQueue.isEmpty())
        return;

    // Check if busy
    if (m_state != StateIdle) {
        return;
    }

    sendEmailInternally(m_messageQueue.dequeue());
}

void SmtpClient::sendEmailInternally(const Message &message)
{
    qCDebug(dcMailNotification()) << "Start sending message" << message.subject << message.body;

    // Initialize data for sending
    m_message = message;
    m_messageData.clear();

    // Create plain message content
    m_messageData = "To: " + m_rcpt + "\r\n";
    m_messageData.append("From: " + m_sender + "\r\n");
    m_messageData.append("Subject: " + message.subject + "\r\n");
    m_messageData.append("Date: " + createDateString() + "\r\n");
    m_messageData.append("Content-Type: text/plain; charset=\"UTF-8\"\r\n");
    m_messageData.append("Content-Transfer-Encoding: quoted-printable\r\n");
    m_messageData.append("MIME-Version: 1.0\r\n");
    m_messageData.append("X-Mailer: nymea;\r\n");
    m_messageData.append("\r\n");
    m_messageData.append(message.body);
    //m_messageData.replace(QString::fromLatin1("\n"), QString::fromLatin1("\r\n"));
    //m_messageData.replace(QString::fromLatin1("\r\n.\r\n"), QString::fromLatin1("\r\n..\r\n"));
    m_messageData.append("\r\n.\r\n");

    setState(StateInitialize);

    // Make sure the connection starts from the beginning
    m_socket->close();
    connectToHost();
}

void SmtpClient::onSocketError(QAbstractSocket::SocketError error)
{
    qCWarning(dcMailNotification()) << "Mail socket error" << error << m_socket->errorString();
    if (m_state != StateIdle) {
        if (m_testLogin) {
            emit testLoginFinished(false);
        } else {
            emit sendMailFinished(false, m_message.actionId);
        }
        m_socket->close();
        setState(StateIdle);
    }
}

void SmtpClient::send(const QString &data)
{
    qCDebug(dcSmtpClient()) << "-->" << data;
    m_socket->write(data.toUtf8() + "\r\n");
    m_socket->flush();
}


