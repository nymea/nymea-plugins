/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#ifndef SENECCONNECT_H
#define SENECCONNECT_H

#include <QUrl>
#include <QObject>
#include <QNetworkReply>

#include <network/networkaccessmanager.h>

// Official read-only SENEC cloud API (V4 systems only): https://developer.senec.com

class SenecConnect : public QObject
{
    Q_OBJECT
public:
    struct SystemData {
        QString systemId;
        QString serialNumber;
        QString model;
        float capacity = 0; // kWh
        float batteryPower = 0; // W, positive charging
        float batteryLevel = 0; // %
        float gridPower = 0; // W, positive import
        float production = 0; // W
        bool active = false;
    };

    explicit SenecConnect(NetworkAccessManager *networkManager, const QString &subscriptionKey, QObject *parent = nullptr);

    static QUrl deviceDataUrl();
    static QList<SystemData> parseDeviceData(const QByteArray &data, bool *ok = nullptr);

    QNetworkReply *getDeviceData();

private:
    NetworkAccessManager *m_networkManager = nullptr;
    QString m_subscriptionKey;

};

#endif // SENECCONNECT_H
