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

#ifndef NYMEALIGHT_H
#define NYMEALIGHT_H

#include <QObject>
#include <QColor>
#include <QQueue>

#include "nymealightinterface.h"

class NymeaLight : public QObject
{
    Q_OBJECT
public:
    explicit NymeaLight(NymeaLightInterface *interface, QObject *parent = nullptr);

    // Set the color. If fade duration is 0, the color will be set immediatly,
    // otherwise it will fade to the color with the given fade duration
    NymeaLightInterfaceReply *setPower(bool power, quint16 fadeDuration = 0);
    NymeaLightInterfaceReply *setColor(const QColor &color, quint16 fadeDuration = 0);
    NymeaLightInterfaceReply *setBrightness(quint8 brightness, quint16 fadeDuration = 0);
    NymeaLightInterfaceReply *setSpeed(quint16 speed, quint16 fadeDuration = 0);
    NymeaLightInterfaceReply *setEffect(quint8 effect);

    bool available() const;

public slots:
    void enable();
    void disable();

private:
    NymeaLightInterface *m_interface = nullptr;
    quint8 m_requestId = 0;
    bool m_interfaceAvailable = false;
    bool m_ready = false;
    int m_pollStatusRetryCount = 0;
    int m_pollStatusRetryLimit = 5;

    NymeaLightInterfaceReply *m_currentReply = nullptr;
    QQueue<NymeaLightInterfaceReply *> m_pendingRequests;

    NymeaLightInterfaceReply *createReply(const QByteArray &requestData);
    void sendNextRequest();

    void pollStatus();
    NymeaLightInterfaceReply *getStatus();

private slots:
    void onDataReceived(const QByteArray &data);

signals:
    void availableChanged(bool available);

};
#endif // NYMEALIGHT_H
