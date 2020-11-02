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

#ifndef HUEBRIDGE_H
#define HUEBRIDGE_H

#include <QObject>
#include <QHostAddress>
#include <QNetworkRequest>

class HueBridge : public QObject
{
    Q_OBJECT
public:
    explicit HueBridge(QObject *parent = nullptr);

    QString name() const;
    void setName(const QString &name);

    QString id() const;
    void setId(const QString &id);

    QString apiKey() const;
    void setApiKey(const QString &apiKey);

    QHostAddress hostAddress() const;
    void setHostAddress(const QHostAddress &hostAddress);

    QString macAddress() const;
    void setMacAddress(const QString &macAddress);

    QString apiVersion() const;
    void setApiVersion(const QString &apiVersion);

    QString softwareVersion() const;
    void setSoftwareVersion(const QString &softwareVersion);

    int zigbeeChannel() const;
    void setZigbeeChannel(const int &zigbeeChannel);

    QPair<QNetworkRequest, QByteArray> createDiscoverLightsRequest();
    QPair<QNetworkRequest, QByteArray> createSearchLightsRequest(const QString &deviceId);
    QPair<QNetworkRequest, QByteArray> createSearchSensorsRequest();
    QPair<QNetworkRequest, QByteArray> createCheckUpdatesRequest();
    QPair<QNetworkRequest, QByteArray> createUpgradeRequest();

private:
    QString m_id;
    QString m_apiKey;
    QHostAddress m_hostAddress;
    QString m_name;
    QString m_macAddress;
    QString m_apiVersion;
    QString m_softwareVersion;
    int m_zigbeeChannel;
};

#endif // HUEBRIDGE_H
