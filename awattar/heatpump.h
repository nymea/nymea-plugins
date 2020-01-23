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

#ifndef HEATPUMP_H
#define HEATPUMP_H

#include <QObject>
#include <QHostAddress>


#include "coap/coap.h"
#include "coap/coapreply.h"
#include "coap/coaprequest.h"

class HeatPump : public QObject
{
    Q_OBJECT
public:
    explicit HeatPump(QHostAddress address, QObject *parent = 0);

    QHostAddress address() const;
    bool reachable() const;
    void setSgMode(const int &sgMode);

private:
    QHostAddress m_address;
    bool m_reachable;
    int m_sgMode;

    Coap *m_coap;

    QList<CoapReply *> m_discoverReplies;
    QList<CoapReply *> m_sgModeReplies;

    void setReachable(const bool &reachable);

private slots:
    void onReplyFinished(CoapReply *reply);

signals:
    void reachableChanged();


};

#endif // HEATPUMP_H
