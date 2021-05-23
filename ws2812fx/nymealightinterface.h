/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
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

#ifndef NYMEALIGHTINTERFACE_H
#define NYMEALIGHTINTERFACE_H

#include <QObject>
#include <QTimer>

class NymeaLightInterface : public QObject
{
    Q_OBJECT
public:
    enum Command {
        CommandGetStatus = 0x00,
        CommandSetPower = 0x01,
        CommandSetColor = 0x02,
        CommandSetBrightness = 0x03,
        CommandSetSpeed = 0x04,
        CommandSetEffect = 0x05,
        CommandCustom = 0xEF
    };
    Q_ENUM(Command)

    enum Notification {
        NotificationReady = 0xF0,
        NotificationDebugMessage = 0xF1
    };
    Q_ENUM(Notification)

    enum Status {
        StatusSuccess = 0x00,
        StatusInvalidProtocol = 0x01,
        StatusInvalidCommand = 0x02,
        StatusInvalidPlayload = 0x03,
        StatusTimeout = 0xfe,
        StatusUnknownError = 0xff
    };
    Q_ENUM(Status)

    enum Mode {
        ModeDirect = 0x00,
        ModeFade = 0x01
    };
    Q_ENUM(Mode)

    inline explicit NymeaLightInterface(QObject *parent = nullptr) : QObject(parent) { };
    virtual ~NymeaLightInterface() = default;

    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool available() = 0;

    virtual void sendData(const QByteArray &data) = 0;

signals:
    void availableChanged(bool available);
    void dataReceived(const QByteArray &data);

};


class NymeaLightInterfaceReply : public QObject
{
    Q_OBJECT

    friend class NymeaLight;

public:
    QByteArray requestData() const { return m_requestData; };
    NymeaLightInterface::Command command() const { return m_command; };
    quint8 requestId() const { return m_requestId; };
    NymeaLightInterface::Status status() const { return m_status; };

signals:
    void finished();

private:
    explicit NymeaLightInterfaceReply(const QByteArray &requestData, QObject *parent = nullptr) : QObject(parent), m_requestData(requestData) {

        Q_ASSERT(m_requestData.length() >= 2);
        m_command = static_cast<NymeaLightInterface::Command>(m_requestData.at(0));
        m_requestId = static_cast<quint8>(m_requestData.at(1));

        m_timer = new QTimer(this);
        m_timer->setInterval(2000);
        m_timer->setSingleShot(true);
        connect(m_timer, &QTimer::timeout, this, [=](){
            m_status = NymeaLightInterface::StatusTimeout;
            emit finished();
        });
    }

    QTimer *m_timer = nullptr;
    QByteArray m_requestData;
    NymeaLightInterface::Command m_command;
    quint8 m_requestId = 0;
    NymeaLightInterface::Status m_status = NymeaLightInterface::StatusUnknownError;
};

#endif // NYMEALIGHTINTERFACE_H
