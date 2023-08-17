/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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

#ifndef FRONIUSSOLARCONNECTION_H
#define FRONIUSSOLARCONNECTION_H

#include <QObject>
#include <QQueue>
#include <QHostAddress>

#include <network/networkaccessmanager.h>

#include "froniusnetworkreply.h"

class FroniusSolarConnection : public QObject
{
    Q_OBJECT
public:
    explicit FroniusSolarConnection(NetworkAccessManager *networkManager, const QHostAddress &address, QObject *parent = nullptr);

    QHostAddress address() const;
    void setAddress(const QHostAddress &address);

    bool available() const;

    bool busy() const;

    FroniusNetworkReply *getVersion();
    FroniusNetworkReply *getActiveDevices();
    FroniusNetworkReply *getPowerFlowRealtimeData();

    FroniusNetworkReply *getInverterRealtimeData(int inverterId);
    FroniusNetworkReply *getMeterRealtimeData(int meterId);
    FroniusNetworkReply *getStorageRealtimeData(int meterId);

signals:
    void availableChanged(bool available);

private:
    NetworkAccessManager *m_networkManager = nullptr;
    QHostAddress m_address;

    bool m_available = false;

    // Request queue to prevent overloading the device with requests
    FroniusNetworkReply *m_currentReply = nullptr;
    QQueue<FroniusNetworkReply *> m_requestQueue;

    void sendNextRequest();

};

#endif // FRONIUSSOLARCONNECTION_H
