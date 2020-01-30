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

#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H

#include <QQueue>
#include <QDebug>
#include <QObject>
#include <QTcpSocket>
#include <QSslSocket>
#include <QStringList>
#include <QLoggingCategory>

//#include "devices/deviceplugin.h"

Q_DECLARE_LOGGING_CATEGORY(dcSmtpClient)

struct Message {
    QString subject;
    QString body;
    int id;
};


class SmtpClient : public QObject
{
    Q_OBJECT
public:

    enum AuthenticationMethod{
        AuthenticationMethodPlain,
        AuthenticationMethodLogin
    };
    Q_ENUM(AuthenticationMethod)

    enum State{
        StateIdle,
        StateInitialize,
        StateHandShake,
        StateAuthentification,
        StateStartTls,
        StateUser,
        StatePassword,
        StateTestLoginFinished,
        StateMail,
        StateRcpt,
        StateData,
        StateBody,
        StateQuit,
        StateClose
    };
    Q_ENUM(State)

    enum EncryptionType{
        EncryptionTypeNone,
        EncryptionTypeSSL,
        EncryptionTypeTLS
    };
    Q_ENUM(EncryptionType)

    explicit SmtpClient(QObject *parent = nullptr);

    void connectToHost();
    void testLogin();
    int sendMail(const QString &subject, const QString &body);

    void setHost(const QString &host);
    void setPort(const quint16 &port);
    void setEncryptionType(const EncryptionType &encryptionType);
    void setAuthenticationMethod(const AuthenticationMethod &authenticationMethod);
    void setUser(const QString &user);
    void setPassword(const QString &password);
    void setSender(const QString &sender);
    void setRecipients(const QStringList &recipients);

private:
    QSslSocket *m_socket = nullptr;
    State m_state = StateIdle;
    QString m_host = "127.0.0.1";
    quint16 m_port = 25;

    QString m_user;
    QString m_password;
    QString m_sender;
    AuthenticationMethod m_authenticationMethod;
    EncryptionType m_encryptionType;
    QStringList m_recipients;
    QQueue<QString> m_recipientsQueue;

    // Created for each message
    Message m_message;
    QString m_messageData;

    QQueue<Message> m_messageQueue;

    bool m_testLogin = false;

    QString createDateString();
    void setState(State state);

    void processServerResponse(int responseCode, const QString &response);

    void sendNextMail();
    void sendEmailInternally(const Message &message);

    void handleSmtpFailure();
    void handleUnexpectedSmtpCode(int responseCode, const QString &serverMessage);

signals:
    void sendMailFinished(bool success, int &id);
    void testLoginFinished(bool success);

private slots:

    void onSocketError(QAbstractSocket::SocketError error);
    void connected();
    void disconnected();
    void onEncrypted();
    void readData();
    void send(const QString &data);
};

#endif // SMTPCLIENT_H
