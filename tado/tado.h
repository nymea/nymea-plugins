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

#ifndef TADO_H
#define TADO_H

#include "network/networkaccessmanager.h"
#include "integrations/thing.h"

#include <QObject>
#include <QTimer>
#include <QUuid>

class Tado : public QObject
{
    Q_OBJECT
public:

    struct Token {
        QString accesToken;
        QString tokenType;
        QString refreshToken;
        int expires;
        QString scope;
        QString jti;
    };

    struct Zone {
        QString id;
        QString name;
        QString type;
    };

    struct Overlay {
      bool power;
      double temperature;
      QString zoneType;
      QString terminationType;
      QString tadoMode;
    };

    struct ZoneState {
        bool connected = false;
        bool power = false;
        QString tadoMode;
        QString settingType;
        double settingTemperature = false;
        bool settingPower = false;
        double temperature = false;
        double humidity = false;
        bool windowOpenDetected = false;
        double heatingPowerPercentage = false;
        QString heatingPowerType;
        bool overlayIsSet = false;
        bool overlaySettingPower = false;
        double overlaySettingTemperature = false;
        QString overlayType;
    };

    struct Home {
        QString id;
        QString name;
    };

    explicit Tado(NetworkAccessManager *networkManager, const QString &username, QObject *parent = nullptr);

    void setUsername(const QString &username);
    QString username();
    bool apiAvailable();
    bool authenticated();
    bool connected();

    void getApiCredentials(const QString &url = "https://app.tado.com/env.js");
    void getToken(const QString &password);
    void getHomes();
    void getZones(const QString &homeId);
    void getZoneState(const QString &homeId, const QString &zoneId);

    QUuid setOverlay(const QString &homeId, const QString &zoneId, bool power, double targetTemperature);
    QUuid deleteOverlay(const QString &homeId, const QString &zoneId);

private:
    bool m_apiAvailable = false;
    QString m_baseAuthorizationUrl;
    QString m_baseControlUrl;
    QString m_clientSecret;
    QString m_clientId;

    NetworkAccessManager *m_networkManager = nullptr;
    QString m_username;
    QString m_accessToken;
    QString m_refreshToken;
    QTimer *m_refreshTimer = nullptr;

    bool m_authenticationStatus = false;
    bool m_connectionStatus = false;
    void setAuthenticationStatus(bool status);
    void setConnectionStatus(bool status);

signals:
    void connectionChanged(bool connected);
    void apiCredentialsReceived(bool success);
    void authenticationStatusChanged(bool authenticated);
    void requestExecuted(QUuid requestId, bool success);

    void tokenReceived(Token token);
    void homesReceived(QList<Home> homes);
    void zonesReceived(const QString &homeId, QList<Zone> zones);
    void zoneStateReceived(const QString &homeId,const QString &zoneId, ZoneState sate);
    void overlayReceived(const QString &homeId, const QString &zoneId, const Overlay &overlay);
    void connectionError(QNetworkReply::NetworkError error);

private slots:
    void onRefreshTimer();

};

#endif // TADO_H
